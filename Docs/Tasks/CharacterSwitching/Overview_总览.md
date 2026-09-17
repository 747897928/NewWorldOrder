# 角色切换总览

版本：2.8
最后更新：2026-03-20
状态：切换后端已落地；菜单长按骨架已落地；世界入口当前退回 PressToInteract 占位方案；另一位主角 NPC 入口的 C++ 骨架已落地，后续仍需替换成最终入口体验

## 当前结论
- 角色切换真正稳定的部分已经落在 C++：
  - `AShootPlayerState::SwitchToCharacter`
  - `AShootPlayerController::TrySwitchCharacter`
  - 快照保存 / 恢复 / 落盘
- 当前入口层代码存在一个原型链，但不能再把它当成最终主线：
  - `UShootGA_Interact`
  - `UShootGA_WorldCharacterSwitchRequest`
  - `AShootCharacterSwitchStation`
- 这条原型链的问题不是“能不能跑”，而是职责仍是原型分层：
  - 世界内交互入口、菜单入口、长按确认、角色切换业务没有拆层
  - `UShootGA_Interact` 被误用成角色切换长按宿主
  - 世界入口桥接能力虽然已经改名收口，但仍只是世界入口原型桥接层，不是最终业务承载层
- 男女主数据分层已经明确：
  - 共享：账号资源、共享仓库、等级/经验等账号层数据
  - 独立：`MaleProtagonist` / `FemaleProtagonist` 的外观与 QuickBar
  - 当前主角：`UShootSaveGame::LastActiveGender`
- 文档与代码的权威关系：
  - 正史需求以 `Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md` 为准
  - 落地实现以 `Source/NewWorldOrder` 当前代码为准
- 输入与交互授予链现状已经补齐关键闭环：
  - `DA_ShootInputConfig` 已加入 `IA_Interact -> InputTag.Ability.Interact`
  - `UShootGA_Interact` 已在 C++ 构造中补齐 `StartupInputTag = InputTag.Ability.Interact`
  - `AShootCharacter::GrantCoreInteractionAbilitiesIfNeeded` 会在服务器 `PossessedBy -> LoadProgress` 之后只兜底授予 `UShootGA_Interact`
  - `Collect / Revive / WorldCharacterSwitchRequest` 等执行能力由附近目标通过 `GrantNearbyInteraction` 动态授予
  - 这样输入分发链与执行能力授予链都已闭合，并避免重复 AbilitySpec
- 交互相关类的定位也要收紧：
  - `UShootGA_Interact` 是交互样例 / 通用交互参考，不是角色切换最终主线
  - 旧蓝图 `GA_Interact` 作为人工迁移的 Lyra 参考资产保留；运行时主线只授予 C++ `UShootGA_Interact`
  - `DA_ShootInputConfig` 只解决输入映射，不决定角色切换入口分层

## 最终体验目标
- 参考《刺客信条：枭雄 / 影》的双主角模式，不是把“切换站”当成最终体验。
- Hub / 家园内：
  - 当前操控主角
  - 另一位主角以 NPC / 待机同伴形式存在
  - 靠近另一位主角时出现交互提示并切换
- 系统层：
  - `ESC` / CommonUI 主菜单内也可提供“切换角色”入口
  - 菜单入口也推荐做成长按，不再额外弹确认框
  - 入口显示与否仍受 `IsInHubOrSafeArea()` 约束
- 因此最终应有两个前端入口：
  - 世界内交互入口
  - 菜单入口
- 它们共用同一个 C++ 后端切换流程

## 当前不发散，但为未来留接口
- 当前阶段只完成双主角切换主线，不把同伴互动系统一起做进来。
- 但需求上已经明确预留未来扩展：
  - 另一位主角 NPC 未来不应只是“切换按钮”
  - 还可能承载对话、邀请跳舞、送礼、约会、队友互动等选项
- 因此当前更合理的理解是：
  - `AShootCharacterSwitchNPCBase` 是通往“同伴交互对象”的过渡骨架
  - 当前只先实现其中一个交互选项：切换角色

## 当前项目架构（必须遵守）
- PlayerState：
  - `UShootInventoryManagerComponent`（有身份物品，ItemInstance+StatTags，支持 Persistent/RuntimeOnly）
  - `UResourceInventoryComponent`（数量型资源）
  - `UAbilitySystemComponent`（ASC 挂 PlayerState）
- Character：
  - `UCombatComponent`（QuickBar 槽位：ItemInstanceId+Lifetime；通过 GetOrCreateWeaponInstanceFromInventory 生成 `UShootWeaponInstance/UShootRangedWeaponInstance`；SpawnedActors 仅视觉壳 `AShootWeaponActor`）
- 静态配置：
  - `UShootInventoryItemDefinition_Weapon` + Fragments（WeaponBasic/Ranged/Projectile/Equippable 等）
- 动态状态：
  - `UShootInventoryItemInstance` 的 StatTags（含 `Inventory_Ammo_Magazine/Reserve` 等）
- GA/HUD：
  - SourceObject = WeaponInstance（UObject），无对外 Actor 依赖
  - Simulated Client 不执行 GA，表现依赖复制/GameplayCue

这部分虽然来自旧版总览，但截至当前代码版本仍然成立，后续修改角色切换时必须继续遵守。

## 当前真实架构

### 真正应保留的角色切换后端
- `AShootPlayerController::TrySwitchCharacter`
  - 做入口层统一校验与结果回包
- `AShootPlayerState::SwitchToCharacter`
  - 做切换业务本体
- `CommitCurrentGenderLoadoutToSave`
  - 刷新当前主角快照并写回存档

### 当前原型链（待重构，不再继续扩展）
- `AShootCharacterSwitchEntryActorBase`
  - 新增的世界入口 Actor 基类
  - 收口“入口提示、触发过滤、事件数据包装”这类共性逻辑
- `AShootCharacterSwitchNPCBase`
  - 新增的另一位主角 NPC 入口骨架
  - 适合蓝图子类直接配置 `RepresentedGender`、外观和待机表现
- `AShootCharacterSwitchStation`
  - 当前只是开发期占位入口
  - 现在继承 `AShootCharacterSwitchEntryActorBase`
  - 现已实现 `IShootCharacterSwitchEntry`
- `UShootGA_Interact`
  - 现在已收口回通用交互样例/参考
  - 不再承载角色切换专属长按门禁
- `UShootGA_WorldCharacterSwitchRequest`
  - 当前相当于“世界入口桥接到切换后端”的原型能力
  - 当前通过 `IShootCharacterSwitchEntry` 取目标性别，不再把能力写死到 `AShootCharacterSwitchStation`
  - 本轮已经从旧的 `UShootGA_Interaction_CharacterSwitch` 收口到当前名字
  - 现在它的定位就是“世界入口请求桥接类”，后续若再拆，也只会继续缩小桥接职责，不会再回到旧命名

### 旧确认框链路状态
- 世界内角色切换的旧确认框 C++ 主线已删除，不再保留 `RequestCharacterSwitchConfirmation` / `SubmitCharacterSwitchDecision` / `UShootCharacterSwitchWidgetBase::ConfirmSwitch` 这套入口。
- 2026-08：`UShootCharacterSwitchWidgetBase`（菜单 UI 模板类）整体删除；衣柜菜单切换由 `UShootWardrobeViewModel` 直连 PlayerController，资产 `IA_SwitchCharacter` / `IMC_CharacterSwitchMenu` 保留。
- 当前统一主线是“入口提出请求 -> PlayerController 收口 -> PlayerState 执行切换 -> 结果事件回包”。
- 菜单入口使用长按进度事件；世界入口当前只是普通交互占位。
- 后续蓝图和 UI 接线应全部改到长按进度事件，不再继续扩展确认框方案。

### 目标入口模型（2026-03-18 收紧）
- 入口 A：世界内入口
  - 另一位主角 NPC 或专用切换点提供提示
  - 玩家按下交互键后发起“请求切换角色”
  - 世界入口对象只需要实现 `IShootCharacterSwitchEntry`
  - 如果入口本体本来就是独立 Actor，可直接继承 `AShootCharacterSwitchEntryActorBase`
  - 如果入口本体已经继承了别的父类，例如未来的 `ACharacter` 版另一位主角 NPC，则直接实现 `IShootCharacterSwitchEntry`
  - 若需要长按，这个长按只是世界入口自己的确认策略
- 入口 B：CommonUI / ESC 菜单入口
  - 使用 CommonUI + 增强输入的长按动作
  - 需要支持手柄
  - 不依赖任何 `IInteractableTarget` 或世界内交互对象
- 两个入口共用：
  - `TrySwitchCharacter`
  - `SwitchToCharacter`
  - `IsInHubOrSafeArea`
  - 快照保存 / 恢复 / 落盘

中文结论：
- 菜单入口不是交互。
- 世界入口也只是入口，不是角色切换业务本体。
- 长按应属于入口确认层，而不是强耦合在 `UShootGA_Interact` 里。

### 这轮审计确认并已修复的风险点
- `BP_ShootCharacter::StartupAbilities` 已只保留有效的 `GA_Hero_Jump`
- `StartupPassiveAbilities` 保留负责 GameplayEvent 转 GameplayEffect 的 `GA_ListenForEvent`
- `GA_Skill1~4` 已从当前启动数组移除但保留为后续扩展资产；正式技能由 `AShootPlayerState::ApplyGenderAbilityKit` 授予
- `UShootGA_Interact` 由 `GrantCoreInteractionAbilitiesIfNeeded` 常驻授予，执行交互能力由附近目标动态授予
- `UShootAbilitySystemComponent::AddCharacterAbilitiesIfMissing` 负责查重，避免 PlayerState ASC 在重复附身时叠加重复 AbilitySpec

### 存档分层
- `UShootSaveGame::SavedInventoryItems`
  - 账号共享 Persistent 物品池
- `UShootSaveGame::MaleProtagonist`
  - 男主独立 QuickBar 与外观标签
- `UShootSaveGame::FemaleProtagonist`
  - 女主独立 QuickBar 与外观标签
- `UShootSaveGame::LastActiveGender`
  - 下次读档恢复当前主角

### Hub/安全区判定职责
- `AShootGameModeBase`
  - 服务器权威决策地图是否属于 Hub/安全区
  - 配置项：`bHubOrSafeAreaMap`
- `AShootGameStateBase`
  - 只负责复制 `bIsHubOrSafeArea`
  - 不负责规则决策
- `AShootPlayerState`
  - 读取 `SafeAreaVolume` 本地状态
  - 读取 `GameState` 的全局 Hub 标记
  - 统一通过 `IsInHubOrSafeArea()` 给角色切换等系统使用

这条边界是当前正确实现，不是“把逻辑放到 GameState”，而是“GameMode 决策，GameState 复制结果”。

## 当前代码状态
- 已完成：
  - 双主角切换
  - 独立外观/QuickBar 存档
  - 当前主角存档恢复
  - 角色切换后端 C++ 链路
  - 菜单长按 C++ 骨架（Enhanced Input + CommonUI Widget）
  - 玩家常驻交互能力与执行交互能力的 C++ 显式授予链
  - 世界内占位入口已从 `UShootGA_Interact` 的专属长按逻辑中拆出，当前为 PressToInteract 占位
  - “另一位主角 NPC 入口”的最小 C++ 骨架（`AShootCharacterSwitchNPCBase`）
- 未完成：
  - 长按进度条蓝图最终接线
  - “另一位主角 NPC”入口替代当前 `SwitchStation`
  - CommonUI / ESC 菜单长按入口的蓝图接线与验收
  - 世界入口桥接层是否还要继续拆成更窄的请求类 / 展示类

## 当前阅读建议

### 你现在只需要看这 3 份
1. `Docs/Tasks/CharacterSwitching/Overview_总览.md`
   - 看目标、边界、当前结论。
2. `Docs/Tasks/CharacterSwitching/CallFlow_调用链路.md`
   - 看世界内角色切换的数据流与调用顺序。
3. `Docs/Tasks/CharacterSwitching/STATUS.md`
   - 看当前完成度、未完成项、下一步。

### 需要深挖时再看
- `Docs/Tasks/CharacterSwitching/Implementation_实现指南.md`
  - 看实现拆解、架构原则、过渡层规划。
- `Docs/Tasks/CharacterSwitching/BlueprintIntegration_蓝图接线指南.md`
  - 看蓝图接线与节点级连线顺序。
- `Docs/Tasks/CharacterSwitching/LyraInteraction_研究笔记.md`
  - 看 Lyra 对照资产、当前授予/输入/执行调用链，以及 C++ 与蓝图启动能力的职责边界。
- `Docs/Tasks/CharacterSwitching/Checklist_检查清单.md`
  - 看验收步骤。

## 维护规则
- 如果代码变了，优先更新本目录文档，不要把过时逻辑留在正文里。
- 如果职责边界变化，必须明确写出：
  - 谁负责决策
  - 谁负责复制
  - 谁负责表现
- 如果一个方案已经退休，必须明确写成“兼容旧资产保留”，不能继续写成主线。
- 如果后续把 Hub 判定、切换入口、存档结构再改动，先更新本页，再更新细分文档。

## 相关必读
- `Docs/Tasks/InventorySystem/` 下的 Requirements / DesignSpec / Implementation / STATUS
- `Docs/QuickReference/SessionChecklist.md`
- `AGENTS.md`
- `Docs/GASDocumentation_Chinese/README.md`
