---
doc_id: TASK-WPN-QUICKBAR-001
title: QuickBar 架构决策
version: 1.8
status: Active
last_updated: 2026-07-26
author: Codex
related_docs:
  - STATUS.md
  - Audit_现状审计.md
  - MigrationStrategy_迁移策略.md
  - ../../DevelopmentNotes/LyraShooterCore_武器迁移笔记.md
---

# QuickBar 架构决策

# 决策

项目采用 Lyra QuickBar 的职责划分，但不直接复制其库存归属。

- `AShootPlayerState::UShootInventoryManagerComponent` 是有身份物品的唯一仓库。账号购买/解锁的武器实例为 `Persistent`；副本内拾取、交换或借用的武器实例为 `RuntimeOnly`，只在该次副本有效。
- PlayerState 保存每个性别的 `Persistent` 出战配置。它是账号背包到副本 QuickBar 的唯一持久化来源，必须参与 SaveGame 与角色快照。
- 进入战斗时，Persistent 出战配置只作为来源，不直接成为战斗对象；系统为每个选中的账号武器物化一个 `RuntimeOnly` 战斗 ItemInstance。弹药、局内换枪和丢枪只作用于这个临时实例。
- `UShootQuickBarComponent` 挂在 `AShootPlayerController`，只管理当前会话的 RuntimeOnly 槽位绑定、激活索引、切槽和装备调度。它不拥有账号物品，也绝不直接进入 SaveGame。
- 当前 Pawn 的 `UShootEquipmentManagerComponent`、`UShootWeaponInstance` 与 `AShootWeaponActor` 继续负责当前 Avatar 的装备逻辑和表现。
- `UCombatComponent` 不再是 QuickBar 的最终归属。迁移期保留它作为兼容入口，待存档、角色切换、衣柜和 UI 调用点全部切换后删除其 QuickBar 职责。

# Lyra 对照事实

Lyra 的 `ULyraQuickBarComponent` 是 `UControllerComponent`，只保存槽位物品、`ActiveSlotIndex` 和单个 `EquippedItem`。切换时通过当前 Controller 的 Pawn 找到 EquipmentManager，装备产生的 EquipmentInstance 保存 `Instigator = SlotItem`。库存、装备和表现各自独立。

项目当前 `UCombatComponent` 已具有 Lyra 的切槽和装备主流程，但额外混入了：

- PlayerState InventoryManager 的 ItemInstanceId 解析。
- Persistent/RuntimeOnly 生命周期和副本丢枪。
- Hub 槽位限制、SaveGame 序列化和男女主快照恢复。
- WeaponActor 缓存、HUD 快照和 AnimNotify 可见性控制。
- 可能在 `GetActiveWeaponInstance()` 读取时创建 EquipmentInstance 的惰性装备路径。

最后一项不允许继续扩散。动画、HUD 和其他本地表现必须使用无副作用的当前武器查询；读取 API 不得创建装备或修改服务器状态。

# 为什么不完整照抄 Lyra

Lyra 默认 QuickBar 与 Controller 生命周期匹配，但项目额外要求：

- InventoryManager 与 ASC 固定在 PlayerState，支持角色 Avatar 切换。
- 每个性别有独立的 Persistent 出战配置；它是“下次进入副本自动带回自己的枪”的依据。
- 副本拾取、交换或借用的 RuntimeOnly 武器只改当前会话 QuickBar。退出副本时清理 RuntimeOnly ItemInstance 与会话槽位，绝不写入账号仓库、Persistent 出战配置或 SaveGame。
- 默认三个武器槽和一个手雷槽，槽位允许规则由项目 Fragment/DataAsset 配置。
- 本地双人分屏必须由每个 LocalPlayer 自己的 PlayerController 持有自己的 QuickBar 和 HUD。

因此不能把 Lyra 的 `TArray<ULyraInventoryItemInstance*>` 原样搬入项目，也不能把库存或 Persistent 出战配置移到 Pawn 或 Controller。

# 目标调用链

```text
PlayerState InventoryManager
  ItemInstance + Persistent/RuntimeOnly + StatTagStack
        |
        v
PlayerState Persistent 出战配置
  每个性别的 Persistent ItemInstanceId 槽位
        |
        | 进入副本、回合开始或重生：物化 RuntimeOnly 战斗实例
        v
PlayerController ShootQuickBarComponent
  当前副本 RuntimeOnly ItemInstanceId 槽位 + ActiveSlotIndex + 当前 EquippedItem
        |
        v
Current Pawn EquipmentManager
  WeaponInstance + AbilitySet SourceObject + Spawned WeaponActor
        |
        v
Character AnimInstance / LocalPlayer HUD
  只读当前 WeaponInstance、瞄准和武器姿态状态
```

# 迁移阶段

1. 新建 Controller QuickBar 主线

- 基于 Lyra 的公开接口建立项目组件。
- 槽位使用 `ItemInstanceId`，不复制或持久化 WeaponActor。
- 提供无副作用的 `FindActiveWeaponInstance`。
- 暂不删除旧 CombatComponent。

2. 接入装备与角色切换

- QuickBar 在 Possess、UnPossess、角色切换和 Pawn 重生时卸载旧 Pawn 的装备，再解析新 Pawn 的 EquipmentManager。
- PlayerState 继续独占 Persistent 出战配置、角色快照和 SaveGame；Controller QuickBar 只在进入副本时从它初始化。
- RuntimeOnly 清理由 PlayerState InventoryManager 与 Controller QuickBar 协作完成；结束副本后下一次初始化只使用 Persistent 出战配置。

3. 迁移 UI、动画与拾取入口

- HUD QuickBar、Reticle、AnimInstance 和 AnimNotify 改为读取新组件的无副作用接口。
- 旧 CombatComponent 仅保留短期转发；所有调用点迁完后删除旧槽位数组和装备调度。

4. 再实施持枪状态桥接

- `UShootAnimInstance` 从当前 Pawn 所属 PlayerController 的 QuickBar 读取 `bWeaponEquipped`、`bAiming` 和武器类别。
- 角色与武器 Montage 同步仍从当前 WeaponInstance 的 SpawnedActor 查找，禁止全局 PlayerController 查询其他本地玩家。

# 第一项编码范围

已完成阶段 1 的最小骨架：

- 新增 `UShootQuickBarComponent`，继承 `UControllerComponent`，由 `AShootPlayerController` 创建，能随当前 Controller 的 Pawn 取得 `UCombatComponent`。
- 新增 `UCombatComponent::FindActiveWeaponInstance()`。该函数只解析已存在的槽位缓存或 EquipmentManager 中已装备、且 `Instigator` 对应槽位物品的 `UShootWeaponInstance`；绝不调用惰性创建路径。
- 现有 `GetActiveWeaponInstance()` 保留给旧兼容调用点，但不得再用于 HUD、动画或其他表现读取。

本阶段尚未迁移槽位数组、切枪、SaveGame、角色切换、UI 或动画蓝图。这样可以先验证 Controller 与 Pawn 的归属关系，不会同时改变库存、网络与角色切换三条链路。

# 副本武器生命周期

1. 玩家在商城购买或解锁武器

- 创建或保留 `Persistent` ItemInstance 于 PlayerState InventoryManager。
- 玩家在 Hub 配置某个性别的 Persistent 出战槽位；该配置保存到 SaveGame。

2. 进入副本、回合开始或角色重生

- 清除上一轮遗留的 RuntimeOnly 战斗实例和会话槽位。
- 从当前性别的 Persistent 出战配置物化新的 RuntimeOnly 战斗实例，绑定到 Controller QuickBar，并让当前 Pawn EquipmentManager 装备激活武器。
- 武器的局内弹药、临时附加状态和交换关系从此时开始独立于账号背包。

3. 副本内拾取或与其他玩家交换武器

- 取得或交换的武器作为 `RuntimeOnly` ItemInstance 加入当前玩家的 PlayerState InventoryManager。
- 只替换当前 Controller QuickBar 的会话槽位；不覆盖 Persistent 出战配置。
- 原主人的 Persistent 账号枪不转移所有权；副本结束后双方都会从各自的 Persistent 出战配置重新开始。

4. 回合重置、角色重生、退出副本或返回 Hub

- Controller QuickBar 卸载当前 Pawn 装备并清空 RuntimeOnly 会话槽位。
- PlayerState InventoryManager 删除 RuntimeOnly ItemInstance。
- Persistent 出战配置与账号武器保持不变；下一回合、下一次重生或下一局重新物化默认战斗武器。

# GameMode 的重置职责

QuickBar 不自行判断“何时死亡重生”“本模式是否回合制”或“副本是否结束”。GameMode/GameState 根据玩法规则调用统一的战斗载具重置入口：

- 团队竞技、生化或副本持续战斗：只在模式定义的出生、复活、检查点或副本结束时重置。
- 爆破等回合制：每回合开始时重置 RuntimeOnly 战斗实例，因此玩家会重新拿到自己 Persistent 出战配置所生成的默认武器。
- 若未来允许副本内切换男女主：先销毁当前 RuntimeOnly 战斗实例，再以目标性别的 Persistent 出战配置物化新的 RuntimeOnly 实例；是否允许由 GameMode 决定，不在 QuickBar 中硬编码限制。

# 最终架构取舍

项目采用“账号库存 + 会话 QuickBar + 模式策略”的三层模型。它保留 Lyra QuickBar 的 Controller/Pawn 装备分离，但不复制 Lyra 没有账号背包的前提。

1. 账号层：PlayerState InventoryManager

- 唯一持有 `Persistent` 武器、账号解锁、男女主出战配置和 SaveGame 数据。
- 不受死亡、回合重置、感染、局内交换或副本结束影响。

2. 战斗会话层：PlayerController QuickBar

- 唯一持有当前战斗的 RuntimeOnly 槽位、激活索引和当前装备引用。
- 进入模式时由 GameMode 指定开局策略初始化；离开模式、回合重置或重生时由 GameMode 请求清理/重建。
- 不保存账号配置，不直接判断“该模式是否允许带背包”。

3. Pawn 表现层：EquipmentManager、WeaponInstance、WeaponActor、AnimNotify

- 当前 Pawn 只持有被 QuickBar 装备出来的 Equipment/WeaponInstance/WeaponActor，不持有账号库存或会话槽位。
- `AN_PlayWeaponMontage` 是角色 Montage 与武器 Montage 的同步桥。它必须从当前 Pawn 的已装备 WeaponInstance 取得 WeaponActor 并执行 Montage Sync Follow；迁移期间不能改成全局 PlayerController 查询，也不能直接删除。

# GameMode 开局与覆盖策略

同一 QuickBar 支持不同模式，不为 CF/CSOL 式背包模式和求生之路式捡枪模式各写一套武器系统：

- `AccountLoadout`：由 Persistent 出战配置物化 RuntimeOnly 默认枪，适用于允许携带账号武器的模式。
- `StarterLoadout`：由模式配置生成手枪、近战或其他基础 RuntimeOnly 武器，账号背包只保留在仓库，不进入本局。
- `EmptyLoadout`：不生成初始武器，完全依赖地图拾取。

模式还需要声明 RuntimeOnly 的重建时机：副本开始、回合开始、重生、检查点或仅副本结束。该策略应由 GameMode/模式配置驱动，不能硬编码在 QuickBar、CombatComponent 或具体武器 GA 中。

感染、生化幽灵、特殊载具或剧情形态使用会话 QuickBar 的临时覆盖层：

- 覆盖层可隐藏或替换常规武器槽，提供爪子、河豚手雷等模式专属 RuntimeOnly 装备/能力。
- 解除覆盖时，按模式规则恢复、重建或清空常规会话槽位。
- 覆盖层绝不删除 Persistent 账号物品或男女主出战配置。

# CombatComponent 收敛边界

`UCombatComponent` 不能继续同时承担库存、会话槽位、装备与动画四种职责。最终只保留 Pawn 专属的战斗表现桥接：

- 提供当前 Pawn EquipmentManager 的安全访问。
- 接收 `AN_PlayWeaponMontage` 的动画通知，并转发给已装备 WeaponInstance/WeaponActor 处理武器表现。
- 不保存 Persistent 配置、RuntimeOnly 槽位、ActiveSlotIndex 或 SaveGame 数据。

# 后续实施顺序

1. 先为 QuickBar 定义模式开局策略、会话重建入口和临时覆盖层的数据契约；不改现有资产。
2. 将会话槽位、切枪和装备调度迁入 Controller QuickBar；PlayerState 仍独占 Persistent 配置。
3. 将 CombatComponent 缩为 Pawn 表现转发层，并改造项目定制的 `/Game/Blueprints/Animations/AnimNotifies/AN_PlayWeaponMontage` 适配新调用链。
4. 再接入武器 GA、准星、后坐力、武器类别动画和迁入的 Lyra 表现资产。

# 第二项编码范围

已完成玩家私有读取入口迁移：

- `UShootQuickBarComponent` 现提供当前激活槽位与槽位展示快照；迁移期内部仍转发至当前 Pawn 的 `UCombatComponent`，不维护第二份数组。
- `UShootHUDReticleComponent` 仅通过其 Owning `AShootPlayerController` 的 QuickBar 查询当前已装备武器，不再调用会惰性创建 Equipment 的旧接口。
- `UShootQuickbarWidgetBase` 仅通过其 Owning PlayerController 的 QuickBar 请求初始槽位快照；消息过滤仍以当前 Pawn 为准，保留现有跨本地玩家隔离。

这不是最终的数据归属迁移。Persistent 出战配置、存档和角色快照必须继续由 PlayerState 执行；Controller QuickBar 将接管的仅是当前副本会话槽位、切枪和 Pawn 装备交接。

# 第三项编码范围

已完成 RuntimeOnly 跨 Pawn 装备交接：

- `AShootPlayerController::OnUnPossess` 在旧 Pawn 引用清空前调用 `UShootQuickBarComponent::CaptureRuntimeSessionFromPawn`。
- `UCombatComponent::BuildRuntimeQuickbarSessionData` 只导出 `RuntimeOnly` 槽位 Guid 与其激活索引；Persistent 槽位不会进入 Controller 内存。
- `UCombatComponent::DetachRuntimeQuickbarSessionForPawnTransition` 卸载旧 Pawn 的 Equipment/WeaponActor 并清空该 Pawn 的 RuntimeOnly 槽位缓存，但不调用 InventoryManager 的删除接口。
- `AShootPlayerController::OnPossess` 在新 Pawn 初始化完成后调用恢复入口；恢复链只通过已保存的 RuntimeOnly Guid 找回 PlayerState InventoryManager 中仍然存在的实例，并重新创建新 Pawn 的 Equipment/WeaponActor 表现。
- 死亡、回合结束和退出副本仍必须调用 `ClearRuntimeSlots` 或模式重建入口；它们会删除 RuntimeOnly ItemInstance，不能使用 Pawn 交接接口替代。

这一阶段只解决跨 Pawn 的会话所有权，不让 Controller 维护第二份可复制的槽位数组。下一阶段才将切槽、拾取和交换的写入权从 `UCombatComponent` 移到 Controller QuickBar。

# 第四项编码范围

已完成副本拾取的 RuntimeOnly 入槽迁移：

- `AShootWeaponPickupActor` 拾取局内武器后调用 Controller QuickBar 寻找空会话槽位。
- `AShootResourcePickup` 与 `AShootInventoryGrantActor` 仅在新物品带 `EquippableItem Fragment` 且生命周期为 `RuntimeOnly` 时调用同一入口；服装、设计图等非装备物继续只进入 InventoryManager。
- Controller QuickBar 在每次成功入槽后刷新其 RuntimeOnly Guid 缓存，使后续 Pawn 重生或副本内换 Avatar 时能恢复最新拾取结果。
- Persistent 物品继续走现有 Hub 配置接口，不能复用副本入槽入口。

清理审计：`UCombatComponent` 的 Persistent 配置、SaveGame、死亡清理和内部 Equipment 操作仍有 C++ 调用点，且当前没有完整替代链路；本阶段不删除这些接口，避免把已验证的账号配置功能误判为垃圾代码。

# 第五项编码范围

已完成 Controller RuntimeOnly 切槽与交换语义入口：

- `SetActiveRuntimeSlot` 只接受 Controller 已缓存的 RuntimeOnly 槽位，拒绝 Persistent 槽位索引。
- `CycleRuntimeSlot` 使用当前实际 RuntimeOnly 槽位索引排序循环，允许中间存在空槽，避免旧 Pawn 数组的空位影响切枪。
- `SwapRuntimeSlots` 只交换两个 RuntimeOnly 会话槽，并同步更新当前激活索引和 Controller 缓存。
- 当前未发现 C++ 的输入调用点；下一步必须通过 UE MCP 审计现有蓝图/GAS 输入图表，再将其接到这些语义入口。不能猜测键盘或手柄键位，更不能在 C++ 写硬编码按键分支。

# 第六项编码范围

已完成切枪的网络与 GAS 语义入口：

- `UShootQuickBarComponent` 提供 `RequestSetActiveRuntimeSlot`、`RequestCycleRuntimeSlot` 与 `RequestSwapRuntimeSlots`。本地客户端只向自己拥有的 PlayerController 组件发送 Server RPC；服务器继续使用既有 RuntimeOnly 校验、装备调度和复制，不接受客户端直接修改 Pawn 槽位。
- 新增抽象基类 `UShootGA_QuickbarCycle`。它以 ServerOnly 策略执行，读取当前 Avatar 的 `AShootPlayerController`，再调用该 Controller 的 QuickBar；不使用全局 PlayerController，因此本地分屏和 Listen Server 客户端不会互相切枪。
- `AShootCharacter` 新增由 `BP_ShootCharacter` 配置的 `CoreCombatAbilities`。这组能力通过 PlayerState ASC 授予并按类去重，重生或副本内换 Avatar 不会重复授予。它与 `CoreInteractionAbilities` 分开，避免把交互扫描、持枪/空手切枪和具体武器 AbilitySet 混为一条授予链。
- 新增 `InputTag.Weapon.Next` 与 `InputTag.Weapon.Previous`，只表达切枪方向。已创建 `/Game/Weapons/Quickbar/GA_QuickbarNext` 与 `GA_QuickbarPrevious`，并分别配置前进/后退；`BP_ShootCharacter.CoreCombatAbilities` 已引用二者。
- 已创建 `IA_WeaponNext`、`IA_WeaponPrevious`，并在 `IMC_Default` 配置默认映射：鼠标滚轮上/下、手柄方向键右/左。这些仍是可编辑的 InputMapping 数据，不是 C++ 固定规则。
- 当前打开编辑器是在 Native Tag 新增前启动，无法把尚未注册的 Tag 写进 `DA_ShootInputConfig`。重启/重新加载编辑器后，必须将两个 IA 分别绑定 `InputTag.Weapon.Next/Previous`，再编译并保存 DataAsset；不得以已有技能 Tag 临时替代。

# 验证结果

- 2026-07-26：`Scripts/Build_Windows.ps1` 编译通过。
- 2026-07-26：QuickBar 玩家私有读取入口迁移后再次编译通过。
- 2026-07-26：RuntimeOnly 跨 Pawn 装备交接实现后再次编译通过。
- 2026-07-26：RuntimeOnly 拾取入槽迁移后再次编译通过。
- 2026-07-26：Controller RuntimeOnly 切槽与交换入口实现后再次编译通过。
- 2026-07-26：`UShootAnimInstance` 已从当前 Pawn 自己的 `AShootPlayerController::GameplayQuickBarComponent` 无副作用读取 `bWeaponEquipped`，并写入 `bLocallyControlled`。当前没有 Aim Ability 的可复制状态标签，`bAiming` 暂保持 false，待 `GA_Weapon_Aim` 与对应 GameplayTag 一并接入，不能使用旧 Character API 伪造状态。
- 2026-07-26：MCP 只读审计确认 `BP_ShootCharacter` 使用 `/Game/Blueprints/Input/DA_ShootInputConfig` 与 `IMC_Default`；当前 AbilityInputActions 只有 `IA_Jump`、四个技能、`IA_Reload`、`IA_Attack` 与 `IA_Interact`，没有切枪 InputAction。`BP_ShootPlayerController` 也没有承担玩法切枪输入图表。
- 2026-07-26：Controller RuntimeOnly 切枪请求 RPC、`UShootGA_QuickbarCycle`、`CoreCombatAbilities` 配置点与切枪 InputTag 编译通过。
- 2026-07-26：准星创建入口已改为 `UUIExtensionSubsystem::RegisterExtensionAsWidgetForContext`。`UShootHUDReticleComponent` 只按 Owning `AShootPlayerController` 注册或注销当前武器的准星类，不再调用 `AddToViewport`、`RemoveFromParent` 或保有 Widget 实例。`UShootReticleWidgetBase` 在创建后从自身 Owning PlayerController 的 QuickBar 无副作用读取当前 WeaponInstance，并在自身生命周期内过滤命中、ADS、击杀 GameplayMessage；世界坐标到屏幕坐标的换算也随 Widget 所属玩家执行。
- 2026-07-26：新增原生标签 `HUD.Slot.Reticle`，并新增 `UIExtension` 模块依赖；Windows Editor 编译通过。编辑器重启后必须在 `/Game/UI/Hud/W_DefaultHUD` 放置 `UUIExtensionPointWidget` 并将其 `ExtensionPointTag` 配为该标签，随后才可在 PIE 验证实际准星显示。
- 2026-07-26：散布恢复改为 WeaponInstance 的时间戳惰性计算。`UShootRangedWeaponInstance` 是 UObject，原有 `Tick()` 没有调用者，连续开火后的散布会永久累积；现在开火时刷新状态，服务器命中射线和本地 Reticle 查询均按同一世界时间计算恢复值，不额外引入 UObject 全局 Tick。
- 2026-07-26：按 Lyra `ULyraRangedWeaponInstance` 适配了可选 Heat 散布模型。`UShootInventoryFragment_RangedWeaponConfig` 新增 `HeatToSpreadCurve`、`HeatToHeatPerShotCurve`、`HeatToCoolDownPerSecondCurve`、冷却延迟与首发精准配置；三条曲线都有数据才启用。旧 `BaseSpreadAngle/SpreadAnglePerShot/MaxSpreadAngle/SpreadRecoveryRate` 继续作为未迁移武器的回退，避免资产一次性重配导致既有枪械失效。当前尚未接入 Lyra 的移动、蹲伏、跳跃、瞄准倍率，因为项目还没有可复制的 Aim 状态；不能以旧 Character 字段猜测替代。
- 2026-07-26：Heat 模型装备时按 Lyra 中值初始化并开始自然冷却；等待到最小 Heat 后，若该枪启用 `bAllowFirstShotAccuracy`，命中散布与 Reticle 会同时进入首发精准。Windows Editor 编译通过。
- 仍需在编辑器 PIE 中验证单人、双本地玩家及 Listen Server；这些运行时回归依赖后续将切槽入口接到新组件，当前最小骨架不改变游戏内切枪行为。

# 非目标

- 不引入 Lyra WeaponStateComponent、命中确认队列或严格命中校验。
- 不修改 PlayerState 库存、ItemInstance 生命周期或 ASC 位置。
- 不把 WeaponActor 重新作为弹药、能力或 QuickBar 数据主对象。
- 不在本阶段接入后坐力、Montage 同步或新的武器专属准星视觉资产。
