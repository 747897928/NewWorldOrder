# 角色切换实现指南
版本：1.6
最后更新：2026-03-20

## 开始前
- 必读：本目录下 `Requirements_需求.md`、`Overview_总览.md`、`STATUS.md`，以及 `Docs/Tasks/InventorySystem/` 全套文档。
- 必读：`Docs/GASDocumentation_Chinese/README.md`（特别是 4.6.1 Ability 定义、4.8 GameplayCue、4.10 预测：Simulated Client 不执行 GA）。
- 复盘现有架构：PlayerState 挂 ASC/InventoryManager/ResourceInventory；Character 挂 CombatComponent（QuickBar+WeaponInstance）；WeaponActor 仅视觉。

## 推荐实现步骤
1) 入口与快照
   - 在 AShootPlayerState 添加 `SwitchToCharacter(ECharacterGender TargetGender)`（服务器权威调用）。
   - 保存当前角色 FCharacterSnapshot（库存/StatTags、ASC 状态、外观、QuickBar）。
   - 失败即回滚。

2) 恢复目标角色
   - InventoryManager：重建 ItemInstances + StatTags；RuntimeOnly 清理策略与副本一致。
   - CombatComponent：用 ItemInstanceId/Lifetime 回填 QuickBar 槽位，调用 GetOrCreateWeaponInstanceFromInventory 生成 WeaponInstance；SpawnedActors 仅视觉。
   - ASC：按 Ability.Character.Male/Female 授予/移除 GA/GE/AttributeSet 初始化。
   - 外观：调用 UMutableAppearanceComponent::SwitchGender 或从快照还原。
   - 技能/被动/等级：按 GameDesign 配置授予 7 主动技能、4 通用被动、2 专属被动；Lv5/10/15/25 解锁槽位逻辑需在切换后生效。敏捷/被动对换弹/射击惩罚的加成需刷新到武器/GA 状态。

3) 同步与验证
   - HUD/Reticle：通过 QuickBar/Ammo 消息刷新；无需 Actor 直连。
   - 资源：ResourceInventory 为共享账户层，不在切换中分拆。
   - 日志：关键路径标注 [Server]/[Client] + SnapshotId/ItemInstanceId，便于排查。

4) 测试用例（至少覆盖）
   - 单机：男女主互切，验证外观/技能/属性/装备/库存独立，Shared 数据保持。
   - Listen Server：切换在服务器生效，客户端表现同步；开火/换弹/拾取/丢弃/弹药/投射物无回归。
   - 存档：保存男主→切换女主→保存→重载→验证两份快照与共享数据。
   - 入口：菜单长按读条完成才切换；松开按键进度回落；世界入口当前按交互键后应走同一个后端；非法区域直接失败。
   - 技能/被动：按 Lv5/10/15/25 槽位解锁验证；换弹/射击惩罚/移速加成按角色与被动生效。

## 提交前检查
- 仅依赖 WeaponInstance/ItemInstance/Fragments/StatTags；无对外 Actor 引用。
- 不销毁 Controller/PlayerState/ASC/InventoryManager/ResourceInventory/CombatComponent。
- 函数行数/复杂度符合项目规范（行数≤80、嵌套≤3 优先）。
- 文档同步：更新 STATUS 进度、说明改动入口与测试结果。

## 架构原则（从旧 DesignSpec 收口）
- 控制与存储都在 `PlayerState`：
  - ASC
  - InventoryManager
  - ResourceInventory
  - SaveGame 快照
- 角色表现都在 `Character`：
  - `CombatComponent` 维护 QuickBar
  - `MutableAppearanceComponent` 维护外观表现
  - `AShootWeaponActor` 只做视觉壳
- 物品与武器数据层次：
```text
ItemDefinition + Fragments
    -> ItemInstance (Guid + Lifetime + StatTags)
    -> WeaponInstance (UObject，GA/HUD 只依赖它)
    -> WeaponActor (纯视觉壳)
```
- 切换角色时不允许销毁这些核心对象：
  - Controller
  - PlayerState
  - ASC
  - InventoryManager
  - ResourceInventory
  - CombatComponent

## 角色切换分层原则
- 角色切换必须拆成 4 层：
  - 世界内入口层
  - CommonUI 菜单入口层
  - 长按确认层
  - 角色切换后端
- 角色切换后端不属于交互系统：
  - 它应该只关心“能不能切、切到谁、怎么保存/恢复”
  - 不关心玩家是靠近 NPC、站在某个点、还是在菜单里长按
- 长按也不属于交互样例类职责：
  - 长按只是确认策略
  - 世界内入口可以有长按
  - 菜单入口也可以有长按
  - 但二者最终都应调用同一个切换请求入口，再进入同一个服务器切换后端

中文结论：
- 角色切换业务和交互系统不能混写。
- 入口可以复用输入和 UI 组件，但后端必须独立。

## 交互能力落地原则
- 交互逻辑权威实现放在 C++：
  - `UShootGA_Interact`
  - `UShootGA_Interaction_Collect`
  - `UShootGA_Interaction_Revive`
  - `UShootGA_WorldCharacterSwitchRequest`
- 蓝图若继续存在，只允许做两类事情：
  - 资产配置
  - 继承原生 C++ 基类后的薄包装
- 旧 `GA_Interact` 的推荐处理方式：
  - 备份一份旧图表逻辑
  - 后续改造成继承 `UShootGA_Interact` 的薄包装资产
  - 新逻辑不再继续堆在蓝图事件图里

### 交互能力应拆成两层
- 主交互能力：
  - `UShootGA_Interact`
  - 职责是扫描、聚焦、提示、输入监听、事件分发
  - 它是交互样例 / 通用交互参考，不应再被描述成角色切换最终宿主
- 执行交互能力：
  - `UShootGA_Interaction_Collect`
  - `UShootGA_Interaction_Revive`
  - `UShootGA_WorldCharacterSwitchRequest`
  - 职责是处理具体交互业务

### 当前推荐授予策略
- 如果项目后续真的启用通用玩家交互系统：
  - `UShootGA_Interact` 应作为玩家预设常驻能力存在
  - 当前已经由 `AShootCharacter::GrantCoreInteractionAbilitiesIfNeeded` 在 C++ 中显式兜底授予
  - 但这件事和角色切换后端是两回事
  - 不能再因为角色切换需求，倒逼把切换专属逻辑塞进 `UShootGA_Interact`
- 核心执行交互能力
  - 当前项目已经改成显式授予给玩家
  - 原因是当前线扫交互链会在玩家 ASC 上查 `FindAbilitySpecFromClass(...)`
  - 如果玩家身上没有对应 `AbilitySpec`，交互选项就无法通过
  - 当前由 `AShootCharacter::CoreInteractionAbilities` 统一列出：
    - `UShootGA_Interact`
    - `UShootGA_Interaction_Collect`
    - `UShootGA_Interaction_Revive`
    - `UShootGA_WorldCharacterSwitchRequest`
  - 当前由 `UShootAbilitySystemComponent::AddCharacterAbilitiesIfMissing` 负责查重后授予
- `GrantNearbyInteraction`
  - 继续保留
  - 但只作为稀有、强上下文特殊执行能力的可选机制
  - 不再作为整个项目主交互能力的默认授予方式

### 当前仓库还差的主要步骤
- `DA_ShootInputConfig` 现在已经有了 `IA_Interact -> InputTag.Ability.Interact`
- `UShootGA_Interact` 也已经在 C++ 构造里补齐 `StartupInputTag = InputTag.Ability.Interact`
- 当前玩家交互能力授予链已经在 C++ 中闭合：
  - `AShootCharacter::PossessedBy`
  - `AShootCharacter::LoadProgress`
  - `AShootCharacter::GrantCoreInteractionAbilitiesIfNeeded`
  - `UShootAbilitySystemComponent::AddCharacterAbilitiesIfMissing`
- 所以当前不再需要依赖蓝图把交互能力偷偷塞进 `StartupAbilities`
- 但输入映射补齐与交互能力显式授予，并不等于角色切换入口架构已经完全收尾
- 当前已经完成：
  - 把角色切换专属长按逻辑从交互样例类里拆出
  - 定义菜单入口与世界入口共用的切换请求接口
  - 把旧 `UShootGA_Interaction_CharacterSwitch` 重命名为 `UShootGA_WorldCharacterSwitchRequest`
  - 新增 `AShootCharacterSwitchEntryActorBase` 收口世界入口 Actor 共性骨架
  - 用 C++ 显式补齐玩家交互主能力与执行交互能力的授予链
- 下一步是：
  - 完成菜单入口蓝图接线与验收
  - 用 `AShootCharacterSwitchNPCBase` 蓝图子类替换当前 `SwitchStation`
  - 做一轮菜单入口与世界入口的完整功能验收

中文结论：
- 交互系统未来要扩展，最稳的结构不是“每个交互物临时给玩家整套交互主能力”。
- 最稳的是：
  - 玩家永远有主交互 GA
  - 交互物只出选项与数据
  - 执行能力按通用度选择“预授予”或“动态授予”

中文结论：
- 这符合当前协作现实。
- 你看不了蓝图，我也读不了蓝图，所以主逻辑必须尽量上收到 C++，蓝图只保留最小配置面。

### 双主角切换与存档结构（概览）

- 存档结构（UShootSaveGame）：
  - SavedInventoryItems：存档级共享仓库（男女主共用武器池）
  - FProtagonistSaveData MaleProtagonist / FemaleProtagonist：
    - QuickbarSlots：各自独立的 QuickBar 配置（主武器/副武器/近战/道具等）
    - AppearanceTags：各自的外观标签
  - LastActiveGender：上次离开游戏时使用的主角性别
- 运行时 PlayerState（AShootPlayerState）：
  - CharacterGender：当前主角性别（可复制）
  - FCharacterRuntimeSnapshot MaleSnapshot / FemaleSnapshot：
    - InventoryItems：运行时物品快照（切换角色用）
    - QuickbarSlots：运行时 QuickBar 配置快照
    - AppearanceTags：各自外观
    - AttributeSnapshot：基础属性（Health/Shield/Ultimate，未覆盖高级效果）
- 切换与存档流程：
  - 读档：SaveGameSubsystem::RestorePlayerInventoryState → 恢复共享仓库 + LastActiveGender 的 QuickBar/外观 → ApplyGenderAbilityKit。
  - 切换：AShootPlayerState::SwitchToCharacter → 保存当前性别快照 → 加载目标性别快照或存档默认 → 更新 CharacterGender + ApplyGenderAbilityKit。
  - Hub Loadout：Hub UI 通过 CombatComponent 的 QuickBar 接口（Server_SetQuickbarSlot/Clear/Swap）修改当前主角 QuickBar → 调用 AShootPlayerState::CommitCurrentGenderLoadoutToSave 刷新快照并写入存档，如需立即写盘由 UI 调 SaveGameSubsystem 的异步保存。
- 男主 Q：TacticalAssault Phase 1 现状：
  - 具备 LaunchCharacter 冲刺 + 服务器胶囊 Sweep 命中流程，可配置 DashDamageEffect/DashVulnerableEffect（推荐配置 ShootEffect_Vulnerable）。
  - 未实现 i-frame/硬控/击退，后续 Phase 2 补充；未配置效果时命中只记录日志，不崩溃。
- 女主 Phase 1 现状（最小可玩）：
  - Q TacticalScan：服务器范围扫描，敌方目标添加 Status.Marked/Status.Vulnerable，并应用 ShootEffect_Vulnerable（Duration=技能 Duration）；到期统一清理目标状态标签。
  - E RescueCloak：施放期间添加 Status.Cloaked，并应用 MoveSpeed Buff（Duration=CloakDuration）；到期/取消清理 Status。
  - C RapidCharge：施放后进入 Buff 窗口（MoveSpeed/ReloadSpeed）；窗口内仅自身击杀触发刷新 Buff 与持续时间（通过 GameMode OnCharacterKilled）。
  - X MedicalStation：服务器 Spawn 医疗站 Actor，按 TeamId 过滤友军并周期性治疗（UShootEffect_HealInstant + SetByCaller.Heal）。
  - 被动 MedicalExpertise/SmartAssist：授予后激活一次，对自身应用长期 GE；GE 挂 Abilities.Kit.Protagonist.Female，性别切换时可统一移除。

### 当前世界内入口（当前原型，待重构）

- 交互站点：`AShootCharacterSwitchStation`
  - 路径：`Source/NewWorldOrder/Public/Interaction/ShootCharacterSwitchStation.h`
  - 定位：当前开发期占位入口；最终应替换成“另一位主角 NPC”或更自然的场景入口。
  - 当前继承 `AShootCharacterSwitchEntryActorBase`
  - 当前已实现 `IShootCharacterSwitchEntry`，后续 NPC 入口应沿用同一个接口。
- 世界入口 Actor 基类：`AShootCharacterSwitchEntryActorBase`
  - 路径：`Source/NewWorldOrder/Public/Interaction/ShootCharacterSwitchEntryActorBase.h`
  - 只收口世界入口 Actor 的共性逻辑：
    - 交互提示
    - 玩家/AI 过滤
    - 事件数据包装
  - 不负责决定目标性别，目标性别仍由 `IShootCharacterSwitchEntry` 的实现者决定。
- 世界入口 NPC 基类：`AShootCharacterSwitchNPCBase`
  - 路径：`Source/NewWorldOrder/Public/Interaction/ShootCharacterSwitchNPCBase.h`
  - 适合未来“另一位主角 NPC”直接做蓝图子类
  - 默认只需要在蓝图里设置：
    - `RepresentedGender`
    - Mesh / Mutable / 动画蓝图 / 待机表现
  - 默认语义是“切到我代表的主角”
  - 当前“如果已经是同一主角则不暴露切换选项”属于防呆兜底，不是核心玩法规则
  - 这样做是为了防：
    - 关卡摆错 NPC
    - 蓝图把 `RepresentedGender` 配错
    - 后续多人协作把入口配置改坏
  - 它不代表项目允许“两个男主 / 两个女主同时存在”
- 交互主能力：`UShootGA_Interact`
  - 路径：`Source/NewWorldOrder/Public/Interaction/Abilities/ShootGA_Interact.h`
  - 已收口回通用交互样例 / 参考能力。
  - 当前不再承担角色切换专属长按，也不再把角色切换目标锁定、读条、松手回落写在这个类里。
- 原型能力：`UShootGA_WorldCharacterSwitchRequest`
  - 路径：`Source/NewWorldOrder/Public/Interaction/Abilities/ShootGA_WorldCharacterSwitchRequest.h`
  - 当前作用是“世界入口桥接到共享切换后端”。
  - 当前通过 `IShootCharacterSwitchEntry` 读取目标性别，不再把能力写死到具体站点类。
  - 当前世界入口先退回 `PressToInteract` 占位方案，后续若恢复世界内长按，也必须放在入口层自己实现。
- 控制器：`AShootPlayerController`
  - `RequestSwitchCharacter`：菜单入口与世界入口统一调用的请求入口。
  - `ServerRequestSwitchCharacter`：把本地菜单请求转到服务器。
  - `TrySwitchCharacter`：服务器权威校验并执行 `AShootPlayerState::SwitchToCharacter`。
  - `BeginCharacterSwitchHold / CancelCharacterSwitchHold / CompleteCharacterSwitchHold` 这组接口当前主要服务菜单入口；世界入口已不再通过 `UShootGA_Interact` 驱动它们。

中文结论：
- 这条链现在只能算原型。
- 不要继续在 `UShootGA_Interact` 里堆角色切换专属逻辑。
- 世界入口当前只是开发期占位链路，先保证共享后端稳定，再决定是否回到“另一位主角 NPC + 独立确认层”。

### 旧确认框主线已经退休
- 已删除的旧 C++ 入口：
  - `RequestCharacterSwitchConfirmation`
  - `SubmitCharacterSwitchDecision`
  - `ServerSubmitCharacterSwitchDecision`
  - `ClientReceiveCharacterSwitchConfirmRequest`
  - `UShootCharacterSwitchWidgetBase::ConfirmSwitch`
  - `UShootCharacterSwitchWidgetBase::CancelSwitch`
- 当前角色切换统一主线是：
  - 世界入口或菜单入口提出切换请求
  - `AShootPlayerController::RequestSwitchCharacter` 负责把请求收口到统一入口
  - `AShootPlayerController::TrySwitchCharacter` 负责服务器权威校验
  - `AShootPlayerState::SwitchToCharacter` 负责快照切换与落盘
  - `OnCharacterSwitchResult`：切换成功/失败后统一回包给 UI
- 当前世界入口只是 `PressToInteract` 占位，不再宣称“唯一主线就是 `UShootGA_Interact` 长按”。

### UI 蓝图接线（当前原型层）

- `UShootCharacterSwitchWidgetBase` 已于 2026-08 删除（全仓零引用）；衣柜内切换由 `UShootWardrobeViewModel` 直连 `AShootPlayerController` 完成。
- 新主线事件（未来重建独立菜单 Widget 时直接绑定 Controller 委托）：
  - `OnCharacterSwitchHoldProgressChanged`：更新长按面板、人物名字、进度条百分比。
  - `OnCharacterSwitchResult`：显示成功/失败提示。
- 旧确认框事件已经从 C++ 主线删除。
- 后续新蓝图不要再围绕确认弹窗继续搭建。

### 最终入口规划（以需求为准）

- 入口 A：世界内交互
  - 目标体验：当前主角靠近 Hub 内另一位主角，出现交互提示并触发切换请求。
  - 当前实现：`AShootCharacterSwitchStation` 占位。
  - 实现建议：
    - 入口本体若是独立 Actor，可继承 `AShootCharacterSwitchEntryActorBase`
    - 入口本体若已经继承了别的父类，例如未来另一位主角 NPC 继承自 `ACharacter`，可直接继承 `AShootCharacterSwitchNPCBase`
    - 如果 NPC 以后还要挂到别的复杂继承链上，再退回手写实现 `IInteractableTarget` 与 `IShootCharacterSwitchEntry`
  - 当前临时方案是 `PressToInteract`。
  - 若后续恢复长按，这个长按只是世界入口自己的确认方式，不是角色切换后端的一部分。
  - 中长期扩展方向：
    - 这个入口本质上应升级成“同伴交互对象”
    - 切换角色只是其中一个交互选项
    - 对话、邀请跳舞、送礼、约会、队友互动等未来能力应继续挂在同一个同伴交互对象上
    - 但这些内容不属于本轮实现范围
- 入口 B：CommonUI / ESC 菜单
  - 目标体验：菜单内显示另一位主角卡片，玩家长按切换。
  - 明确要求：使用 CommonUI 长按，需兼容手柄。
  - 建议：由 CommonUI + 增强输入单独实现菜单长按确认，不依赖世界内交互物或 `UShootGA_Interact`。
  - 输入上下文建议：
    - 基础前端导航继续保留 `IMC_FrontEnd`
    - 角色切换面板激活时，叠加 `IMC_Confirm_Select_Character`
    - 优先使用 `UCommonActivatableWidget::InputMapping / InputMappingPriority` 自动切入和移除
    - 只有当输入上下文不跟 Widget 生命周期绑定时，才手动调 `UEnhancedInputLocalPlayerSubsystem::AddMappingContext / RemoveMappingContext`
  - C++ 实现优先级：
    - `ULyraActivatableWidget` 中 `RegisterUIActionBinding(FBindUIActionArgs(InputAction, ...))`
    - 或 `UCommonButtonBase` 的 `TriggeringEnhancedInputAction + bRequiresHold + HoldData`
  - 动作资产建议：
    - `RegisterUIActionBinding(FBindUIActionArgs)` 只是 Widget 对动作的监听入口，不是 `UInputAction` 的替代品
    - 动作本身仍然需要 `UInputAction`
    - 动作所在的键位分配仍然需要 `Input Mapping Context`
    - 动作图标与文案仍然需要 `DT_PMM_InputAction` 之类的数据表支持
    - 当前方案已切到独立键位模式：
      - 新增 `IA_SwitchCharacter`
      - 新增 `IMC_CharacterSwitchMenu`
      - `DT_PMM_InputAction` 新增 `Input_SwitchCharacter`
    - 建议键位：
      - 手柄：X
      - 键盘：E
  - Trigger 建议：
    - 当前不要再给 `IA_SwitchCharacter` 配 `Pressed + Released` 这种短按组合
    - 第一版直接改成 `Hold`
    - `HoldTimeThreshold` 当前资产已改为 3.0 秒
    - `bIsOneShot = true`
    - `bAffectedByTimeDilation = false`
    - 当前不建议用 `Hold And Release`
    - 原因：我们要的是“读条满立即切换”，不是“读条满后松手再切换”
  - Generic Action 元数据建议：
    - 当前这条独立切换键长按方案，不再把 `RegisterUIActionBinding` 当作核心监听器
    - CommonUI 仍可继续负责焦点、显示、ActionWidget 提示
  - 菜单长按完成后应调用 `AShootPlayerController::RequestSwitchCharacter`
  - 不应直接在菜单 Widget 中越过控制器去碰 `AShootPlayerState`
  - 当前用户已确认的前端资产事实：
    - `IA_Confirm` 已映射手柄 A
    - `DT_PMM_InputAction` 已提供确认输入的跨设备图标数据
    - `IMC_FrontEnd` 已在前端地图流程中使用
  - 当前用户已确认的菜单输入方向：
    - 角色切换菜单使用独立键位
    - 后续主菜单相关输入都走 `IMC_CharacterSwitchMenu`
  - 因此当前建议变为：
    - `IMC_FrontEnd` 负责前端基础输入
    - 角色切换相关界面或流程叠加 `IMC_CharacterSwitchMenu`
    - `IA_SwitchCharacter` 负责独立切换动作
    - `DT_PMM_InputAction` 的 `Input_SwitchCharacter` 只负责图标与提示
    - 增强输入 + C++ 自己负责 hold 进度与松手回落
  - 监听与回落建议：
    - 菜单输入接收者使用 Enhanced Input 直接绑定 `IA_SwitchCharacter`
    - 绑定事件：
      - `Started`
      - `Ongoing`
      - `Triggered`
      - `Canceled`
      - `Completed` 可选，仅做成功后的收尾
    - `Ongoing` 中使用 `FInputActionInstance::GetElapsedTime()` / `HoldTimeThreshold` 计算进度
    - `Triggered` 中调用 `RequestSwitchCharacter`
    - `Canceled` 中启动 UI 回落
    - 回落时间由 C++ 常量或配置控制，第一版建议 0.25 秒
  - 当前 C++ 落地约定（2026-08 更新：旧 `UShootCharacterSwitchWidgetBase` 已删除）：
    - 菜单 Widget 使用 `ULyraActivatableWidget`（`UCommonActivatableWidget`）作为父类
    - 蓝图子类在父类 Details 中把 `UCommonActivatableWidget::InputMapping` 设成 `IMC_CharacterSwitchMenu`
    - `IA_SwitchCharacter` 与 `IMC_CharacterSwitchMenu` 资产保留，供独立菜单长按入口复用
    - C++ 在 Widget 自己的增强输入绑定里监听：
      - `Started`
      - `Ongoing`
      - `Triggered`
      - `Canceled`
      - `Completed` 仅保留给调试/收尾，不作为主逻辑入口
    - 纯 C++ Widget 没有蓝图输入事件时，可能不会自动生成输入组件，因此 C++ 需要在激活时兜底创建 `UEnhancedInputComponent`
    - 目标性别推导逻辑已随 `UShootCharacterSwitchWidgetBase` 删除；菜单 UI 需要自己决定目标性别再调用 `RequestSwitchCharacter`
  - 关于 `RegisterUIActionBinding` 的更新结论：
    - 它不是全局监听
    - 它依然适合普通 UI 动作
    - 但当前菜单独立切换键长按，不再把它作为唯一核心监听器
- 两个入口必须共享：
  - `IsInHubOrSafeArea()` 校验
  - 快照保存/恢复
  - 存档写入 `LastActiveGender`
  - 男女主独立 QuickBar / 外观恢复

### 过渡黑屏 / 过渡场景的定位

- 参考《刺客信条：影》的小型过渡空间，我们后续可以在当前长按链路外再包一层“过渡层”。
- 这层的职责应该是：
  - 黑屏 / 过渡动画 / 过渡相机
  - 暂停输入与 HUD
  - 等待 `SwitchToCharacter` 完成
  - 再恢复正式场景
- 这层不应该直接改 Inventory / ASC / QuickBar / Mutable 数据。
- 过渡层如果后续要做，技术上至少要覆盖：
  - 锁移动、开火、换弹、交互输入
  - 停止旧角色的 Montage、Loop 音效、枪口特效、瞄准状态
  - 保存当前主角快照并更新 `LastActiveGender`
  - 等待目标角色的外观、QuickBar、HUD、镜头状态稳定
  - 最后再亮屏并恢复控制权

### 过渡层分阶段建议（从旧过渡方案收口）
- Phase 1：
  - 先把当前世界内长按切换跑稳
  - 验证独立外观、QuickBar、存档恢复
- Phase 2：
  - 在 `TrySwitchCharacter` 外侧包一层黑屏过渡
  - 只负责遮挡和恢复，不改数据本体
- Phase 3：
  - 如果体验需要，再做《刺客信条：影》式的小型过渡空间
  - 不建议当前直接做重型独立关卡切换

### Hub/安全区判定归属说明

- `AShootPlayerState::IsInHubOrSafeArea` 的判定优先级：
  - SafeAreaVolume（`AShootSafeAreaVolume` 重叠计数）
  - `AShootGameStateBase::bIsHubOrSafeArea`（全局标记，复制到客户端）
- 为什么标记放在 `GameState`：
  - `GameMode` 负责服务器规则决策。
  - `GameState` 负责把结果复制给客户端 UI/交互。
- 统一来源关系（单一路径）：
  - 地图级开关配置在 `AShootGameModeBase::bHubOrSafeAreaMap`（蓝图可配）。
  - 服务器 `AShootGameModeBase::BeginPlay` 写入 `AShootGameStateBase::SetHubOrSafeArea(...)`。
  - `AShootPlayerState` 与 `CombatComponent` 只读取 PlayerState/ GameState，不再依赖 WorldSettings Tag 兜底。
