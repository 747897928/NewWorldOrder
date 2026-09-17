# 角色切换调用链路

版本：1.6
最后更新：2026-03-20
状态：当前文档同时记录“现有原型链”和“目标重构链”，避免继续沿错误职责扩展

## 当前世界内原型链（待重构，不再继续扩展）
1. 玩家靠近切换入口。
2. `UShootGA_Interact` 扫描到角色切换目标。
3. 玩家按下交互键后，`UShootGA_Interact` 直接触发当前聚焦选项。
4. `UShootGA_WorldCharacterSwitchRequest` 作为世界入口桥接类，通过 `IShootCharacterSwitchEntry` 解析目标性别，再调用 `AShootPlayerController::RequestSwitchCharacter`。
5. 若请求来自本地客户端：
   - `AShootPlayerController::ServerRequestSwitchCharacter`
6. `AShootPlayerController::TrySwitchCharacter`
   - 校验 PlayerState 是否可用
   - 校验目标性别是否合法
   - 校验是否位于 Hub / 安全区
   - 调用 `AShootPlayerState::SwitchToCharacter`
7. `AShootPlayerState::SwitchToCharacter`
   - 保存当前角色快照
   - 恢复目标角色快照或默认配置
   - 更新当前性别
   - 应用外观 / QuickBar / 技能套件
8. 切换成功后：
   - `CommitCurrentGenderLoadoutToSave`
   - `ClientReceiveCharacterSwitchResult`
9. `AShootPlayerController`
   - 广播 `OnCharacterSwitchResult`
10. UI 结果处理（当前衣柜路径为 `UShootWardrobeViewModel::HandleCharacterSwitchResult`）
   - 如果本次请求来自菜单长按，继续关闭进度或显示结果
   - 旧 `UShootCharacterSwitchWidgetBase` 已于 2026-08 删除

中文结论：
- 这条链现在已经退回“世界入口占位原型”。
- 当前世界入口不再把角色切换专属长按写在 `UShootGA_Interact` 里。
- 问题从“职责混乱”收口为“入口体验仍是占位方案，后续需要替换成最终 UX”。

## 目标调用链

### 入口 A：世界内入口
1. 玩家靠近另一位主角 NPC 或切换点。
2. 世界入口对象提供“可切换到目标角色”的上下文。
3. 玩家按下交互键。
4. 若世界入口要求长按，则由世界入口自己的确认层完成长按。
5. 确认完成后，入口层调用统一切换请求接口。
6. `AShootPlayerController::RequestSwitchCharacter`
7. 如当前不在服务器，转发到 `ServerRequestSwitchCharacter`
8. `AShootPlayerController::TrySwitchCharacter`
9. `AShootPlayerState::SwitchToCharacter`

### 入口 B：CommonUI 菜单入口
1. 玩家打开 CommonUI 菜单。
2. 菜单展示另一位主角卡片。
3. 玩家使用 CommonUI + 增强输入长按切换动作。
4. 菜单长按完成后，直接调用统一切换请求接口。
5. `AShootPlayerController::RequestSwitchCharacter`
6. `AShootPlayerController::ServerRequestSwitchCharacter`
7. `AShootPlayerController::TrySwitchCharacter`
8. `AShootPlayerState::SwitchToCharacter`

中文结论：
- 世界入口和菜单入口前端不同。
- 后端应该只有一套。
- 长按属于入口确认层，不属于 `UShootGA_Interact`。

## 文件到职责映射
- `Source/NewWorldOrder/Private/Interaction/ShootCharacterSwitchEntryActorBase.cpp`
  - 当前世界入口 Actor 的共性骨架。
  - 负责交互提示、触发过滤、事件数据包装。
- `Source/NewWorldOrder/Private/Interaction/ShootCharacterSwitchNPCBase.cpp`
  - 当前另一位主角 NPC 入口的最小 C++ 骨架。
  - 负责“我代表哪个主角”和“什么时候不该再显示切换提示”。
- `Source/NewWorldOrder/Private/Interaction/Abilities/ShootGA_Interact.cpp`
  - 当前已回归通用交互样例/参考。
  - 负责扫描、聚焦、触发，不再承担角色切换专属长按。
- `Source/NewWorldOrder/Private/Interaction/Abilities/ShootGA_WorldCharacterSwitchRequest.cpp`
  - 当前是“世界入口桥接到角色切换后端”的原型类。
  - 当前通过 `IShootCharacterSwitchEntry` 取目标角色，后续 NPC 入口可直接复用。
  - 本轮已经完成旧类名收口；后续只需要继续评估桥接层是否还要再拆。
- `Source/NewWorldOrder/Public/Interaction/IShootCharacterSwitchEntry.h`
  - 世界入口最小接口。
  - 只负责回答“当前应该切到谁”。
- `Source/NewWorldOrder/Private/Player/ShootPlayerController.cpp`
  - 菜单长按 UI 状态、统一切换请求入口、Server RPC、中转到服务器权威切换。
- `Source/NewWorldOrder/Private/Player/ShootPlayerState.cpp`
  - 角色切换、快照保存、快照恢复、存档同步。
- `Source/NewWorldOrder/Private/UI/ViewModel/WardrobeViewModel.cpp`（`SwitchCharacter` / `HandleCharacterSwitchResult`）
  - 当前承接衣柜菜单的角色切换请求与结果事件（旧 `ShootCharacterSwitchWidgetBase.cpp` 已删除）。
  - 后续若世界入口也需要独立长按展示，直接绑定 `AShootPlayerController` 的 `OnCharacterSwitchHoldProgressChanged` / `OnCharacterSwitchResult`。

## 交互能力授予策略
- 主交互能力和执行交互能力不能混成一个概念：
  - 主交互能力：`UShootGA_Interact`
  - 执行交互能力：`UShootGA_Interaction_Collect / Revive / CharacterSwitch`
- 当前项目推荐做法：
  - 角色切换后端不要再依赖 `UShootGA_Interact`
  - `UShootGA_Interact` 现已作为玩家预设常驻交互能力由 C++ 显式兜底授予
  - 交互物只通过 `IInteractableTarget` 提供选项、提示文案和事件数据
  - 常见执行能力当前也由玩家常驻能力链显式授予
- 当前 C++ 授予链：
  - `AShootCharacter::PossessedBy`
  - `AShootCharacter::LoadProgress`
  - `AShootCharacter::GrantCoreInteractionAbilitiesIfNeeded`
  - `UShootAbilitySystemComponent::AddCharacterAbilitiesIfMissing`
- 当前默认授予能力：
  - `UShootGA_Interact`
  - `UShootGA_Interaction_Collect`
  - `UShootGA_Interaction_Revive`
  - `UShootGA_WorldCharacterSwitchRequest`
- 这个结论来自当前代码事实：
  - `UShootInputComponent::BindAbilityActions(...)` 只会把 `InputTag` 分发给玩家 ASC 上已存在的 `AbilitySpec`
  - `UAbilityTask_WaitForInteractableTargets::UpdateInteractableOptions(...)` 在 `InteractionAbilityToGrant` 分支里，会对玩家 ASC 调 `FindAbilitySpecFromClass(...)`
  - 也就是说，若玩家身上没有对应 `AbilitySpec`，当前线扫交互分支就过不了
- `GrantNearbyInteraction` 这条 Lyra 基座仍然保留，但定位应调整为：
  - 稀有、上下文强的特殊执行能力可继续使用动态授予
  - 它不是整个项目的主交互扫描与输入门禁主线

## 主线与退休边界（从旧代码审计收口）

### 当前主线保留
- `AShootPlayerState::SwitchToCharacter`
  - 服务器权威角色切换入口
- `AShootPlayerState::CommitCurrentGenderLoadoutToSave`
  - 切换后把当前主角 QuickBar / 外观刷新回存档
- `AShootPlayerController`
  - 统一切换请求入口
  - 结果回包

### 当前原型层暂留
- `UShootGA_Interact`
  - 当前已收口回通用交互样例
  - 不再继续扩展角色切换专属逻辑
- `UShootGA_WorldCharacterSwitchRequest`
  - 当前原型中承担了世界入口桥接
  - 当前命名已经收口为“世界入口请求桥接类”
  - 后续若继续拆分，只是为了让桥接层更窄，不再是为了解决错误命名
- `UShootWardrobeViewModel`（`SwitchCharacter` / `HandleCharacterSwitchResult`）
  - 当前菜单切换 UI 承接链；旧 `UShootCharacterSwitchWidgetBase` 已删除

### 已退休的旧确认框主线
- `RequestCharacterSwitchConfirmation`
- `SubmitCharacterSwitchDecision`
- `ServerSubmitCharacterSwitchDecision`
- `ClientReceiveCharacterSwitchConfirmRequest`
- `HasPendingCharacterSwitchConfirm`
- `UShootCharacterSwitchWidgetBase::ConfirmSwitch`
- `UShootCharacterSwitchWidgetBase::CancelSwitch`
- `BP_UpdateConfirmPanel`

中文结论：
- 世界内角色切换已经不再走“二次确认框”。
- “当前唯一主线就是 `UShootGA_Interact` 长按”这句话已经失效。
- 当前真正保留的一致主线是“入口发请求 -> PlayerController 收口 -> PlayerState 切换”。

### 当前暂留但不要误删
- `AShootCharacterSwitchStation`
  - 这是开发期占位入口，不是最终体验
  - 后续应替换成“另一位主角 NPC 可交互”
- `AShootPlayerController::TrySwitchCharacter(ECharacterGender, AActor*)`
  - `SourceActor` 现在还没用满
  - 但后续做 NPC 距离/朝向校验时会用到，不是冗余参数

## 当前蓝图应该接什么
- 必接（直接绑定 `AShootPlayerController` 委托，旧 Widget 基类已删除）：
  - `OnCharacterSwitchHoldProgressChanged`
  - `OnCharacterSwitchResult`
- 不要再接：
  - `BP_UpdateConfirmPanel`
  - `ConfirmSwitch`
  - `CancelSwitch`

## 当前最小验收链
- 进入 Hub / 安全区。
- 靠近切换入口或打开菜单。
- 世界入口当前按交互键可直接走切换请求。
- 菜单入口长按时进度条正常出现。
- 菜单松手后进度回落。
- 两个入口最终都走同一个切换后端。
- 收到结果提示。
- 重进场景后按 `LastActiveGender` 恢复当前主角。

## 与过渡黑屏的关系
- 过渡黑屏不是切换逻辑本体，而是外层包装。
- 当前调用链已经足够支撑最小可玩版本。
- 如果后续要加黑屏 / 过渡场景，应包在：
  - 长按完成之后
  - `TrySwitchCharacter` 前后
  - `ClientReceiveCharacterSwitchResult` 收尾之前

中文结论：
- 先稳住这条主线，再往外包过渡表现。
- 不要把黑屏、镜头、过渡空间直接塞进 `PlayerState` 的切换逻辑里。
