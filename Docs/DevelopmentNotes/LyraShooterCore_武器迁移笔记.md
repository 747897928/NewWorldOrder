# Lyra ShooterCore 武器迁移笔记

日期：2026-08-09
状态：[可用]

# 用途

本笔记记录通过 LyraStarterGame MCP 实际读取的 ShooterCore 武器资产、蓝图父类和动画同步链。后续实现射击系统、迁移武器资产或上下文恢复时必须先读本笔记与 `Docs/Tasks/WeaponSystem/LyraBlueprintAudit_蓝图审计.md`。

# 项目侧既定边界

- 保留项目 PlayerState 上的 ASC、InventoryManager、ResourceInventory 和 Persistent/RuntimeOnly。
- 保留项目 CombatComponent -> EquipmentManager -> WeaponInstance 主线；WeaponActor 采用 Lyra 的 B_Weapon 单 Actor 表现模式，项目只在 SourceObject、QuickBar 和 GameplayCue 边界做适配。
- 不引入 Lyra WeaponStateComponent、未确认命中队列、服务器命中替换和严格校验。
- 用 Lyra 的资产组织、Equipment、WeaponInstance、AbilitySet、AnimLayer、Montage 同步和 Reticle 表现作为参考，不把 Lyra C++ 父类直接带入项目。

# 服务器结算与本地后坐力

- `UShootGameplayAbility_Weapon_Fire` 保留客户端本地 Trace 作为即时开火表现和本地准星预览；服务器收到能力预测链路后，使用服务器当前 PlayerController 视角、碰撞场景和 `UShootRangedWeaponInstance` 的散布重新 Trace，再以该结果执行伤害与服务器 GameplayCue。这是项目的基础权威结算，不是 Lyra WeaponStateComponent 的命中确认或严格校验。
- 服务器 Trace 必须允许 `AvatarPawn->HasAuthority()`。远端玩家 Pawn 在服务器上通常不具备 `IsLocallyControlled()`；若只允许本地控制端 Trace，代码会退化为错误复用客户端 HitResult。
- Lyra `ULyraWeaponInstance` 的输入反馈是 `ApplicableDeviceProperties`：武器装备时按拥有 Pawn 的 `FPlatformUserId` 激活持续的 `UInputDeviceProperty`，卸下或死亡时移除。Lyra 核心 C++ 没有每发直接修改鼠标相机的代码。
- 项目额外需要每发相机后坐力，因此 `UShootInventoryFragment_RangedWeaponConfig` 提供 `LocalCameraPitchRecoilMin/Max` 与 `LocalCameraYawRecoilMin/Max`。成功 `CommitAbility` 后，仅 `IsLocallyControlled()` 的开火者 `PlayerController` 消费这四个值；它不复制、不发 RPC、不参与散布或命中校验。后续如需手柄震动，应按 Lyra 的 `UInputDeviceProperty` 生命周期另行配置，不能把硬件分支写进武器 GA。

# 已读取的关键资产

- `/Game/Weapons/B_Weapon`
  - 父类为 `Actor`。
  - 提供 SkeletalMesh、枪口火焰、Tracer、弹壳、命中/贴花、假投射物、连续开火音频和本地 CustomStencil 表现。
  - 依赖 `B_WeaponFire`、`B_WeaponImpacts`、`B_WeaponDecals`，三者同样以 `Actor` 为父类。
  - 可迁移为项目 WeaponActor 表现模板；队伍染色调用需改为项目队伍接口或删除。
- `/Game/Weapons/GA_Weapon_Fire`
  - 父类为 `LyraGameplayAbility_RangedWeapon`。
  - 负责 TargetData、开火/命中 GameplayCue、服务器伤害、自动开火计时、失败开火 Montage 和可选物理场。
  - 不能原样作为项目运行时 GA；项目 `UShootGameplayAbility_Weapon_Fire` 是对应 C++ 主线，应按此蓝图补齐缺失表现。
- `/ShooterCore/Weapons/Rifle/B_Rifle`、`Pistol/B_Pistol`、`Shotgun/B_Shotgun`
  - 都继承 `B_Weapon`，可作为三类 WeaponActor 表现子蓝图的来源。
- `/ShooterCore/Weapons/*/GA_Weapon_Fire_*` 和 `GA_Weapon_Reload_*`
  - 分别继承 `GA_Weapon_Fire_C`、`GA_Weapon_ReloadMagazine_C`，专用子蓝图几乎不含独立逻辑。
  - 项目应以自己的 Rifle/Pistol/Shotgun GA 子类和 DataAsset 配置实现等价行为。
- `/ShooterCore/Weapons/*/AbilitySet_Shooter*`
  - 类型是 `LyraAbilitySet`，不能直接用作项目 `UShootAbilitySet`。
  - 2026-07-26 通过 Lyra MCP 读取到的最小能力表为：Rifle = `FireAuto` + `Reload` + 无输入标签的 `AutoReload`；Pistol = `Fire` + `Reload` + 无输入标签的 `AutoReload`；Shotgun = `FireAuto` + `Reload` + 无输入标签的 `AutoReload`。
  - 项目第一轮只配置显式 Fire/Reload：Rifle 使用项目 Rifle Fire 与通用弹匣 Reload，Pistol 使用 `UShootGA_Weapon_Fire_Pistol` 与通用弹匣 Reload，Shotgun 使用项目 Shotgun Fire 与通用弹匣 Reload。用户已明确 Shotgun 不是逐发装填，旧 `UShootGA_Reload_ShotgunPerShell` 不得进入这三把首批枪械的 AbilitySet；AutoReload 是否保留须等弹药耗尽交互验收后单列任务决定。
- `/ShooterCore/Weapons/*/GE_Damage_*`
  - 都继承 `GE_Damage_Basic_Instant`，无自定义图表。
  - 可迁移为数值和伤害配置参考，但接入前必须映射项目 AttributeSet、Damage GE 和 GameplayTag。
- `W_Reticle_Rifle`、`W_Reticle_Pistol`、`W_Reticle_Shotgun` 与 `W_AmmoCounter_*`
  - 都继承 `LyraReticleWidgetBase`。
  - 包含 ADS、命中/击杀消息、屏幕散布半径和动画表现。
  - 项目 `UShootReticleWidgetBase` 和 CommonUI HUD 层已经可承载它们：2026-07-26 在 `/Game/UI/Hud/W_DefaultHUD` 新增 `ReticleExtensionPoint`，其 `UUIExtensionPointWidget.ExtensionPointTag` 精确设为 `HUD.Slot.Reticle`，Canvas Slot 使用全屏锚点、零边距、ZOrder 10。
  - 用户迁移原始 Lyra 准星资产后，保留其路径和控件树；只需改父类为对应的项目 `UShootReticleWidgetBase` 子类并写入项目 ItemDefinition 的 RangedWeaponConfig，不能重新引入 `AddToViewport`。

# 角色与武器 Montage 同步

以下 Lyra 资产是武器动画同步的权威参考：

- `/Game/Weapons/Rifle/Animations/AM_MM_Rifle_Fire`
- `/Game/Weapons/Rifle/Animations/AM_Weap_Rifle_Fire`
- `/Game/Weapons/Rifle/Animations/AM_Weap_Rifle_Reload`
- `/Game/Weapons/Rifle/Animations/ABP_Weap_Rifle`
- `/Game/Weapons/GA_Weapon_Fire`
- `/Game/Characters/Heroes/Mannequin/Animations/AnimNotifies/AN_PlayWeaponMontage`

Lyra `AN_PlayWeaponMontage` 的父类为 `AnimNotify`，公开属性为 `MontageToPlay` 和
`RateScale`。其 `Received_Notify` 已通过 MCP 验证的流程为：

1. 从角色 `MeshComp` 获取角色 AnimInstance。
2. 从角色 Owner 查找 Lyra EquipmentManager，取得当前 Weapon Equipment 的 SpawnedActor。
3. 从 SpawnedActor 取得武器 SkeletalMeshComponent 和武器 AnimInstance。
4. 将角色 AnimInstance 保存为 Montage Leader，将武器 AnimInstance 保存为 Follower。
5. 在武器 AnimInstance 上调用 `Montage Sync Follow`，使武器 Fire/Reload Montage 跟随角色 Montage 的时间轴。

项目实现时，Notify 或其项目替代物必须改为通过 `UCombatComponent` / `UShootEquipmentManagerComponent` 找到当前 `UShootWeaponInstance` 的 SpawnedActor。角色 AnimInstance 是 Leader，武器 AnimInstance 是 Follower。不得通过全局 PlayerController 或硬编码 Actor 查找武器，避免本地分屏串到其他玩家。

2026-07-26 复核的旧项目 `/Game/Blueprints/Animations/AnimNotifies/AN_PlayWeaponMontage`
使用 `CombatComponent -> Get Active Slot Item`，属于旧表现桥。2026-07-31 正式 CC Montage
迁移时发现它不适合当前 Equipment 主线；复制 Lyra EquipmentManager 蓝图 Notify 后，手动
`Received_Notify` 仍返回失败。

项目代码中已经实现 C++ `UShootAnimNotify_PlayWeaponMontage`，其预期职责为：

- 从 Notify 所属角色 Pawn 获取 `UShootEquipmentManagerComponent`。
- 取当前 `UShootWeaponInstance` 的 SpawnedActor 与武器 SkeletalMesh AnimInstance。
- 先在武器 AnimInstance 播放 `MontageToPlay`，再调用 `MontageSync_Follow`，角色 Montage
  为 Leader、武器 Montage 为 Follower。
- 不查询全局 Controller，不参与弹药、伤害或装备状态；GAS 权威边界不变。
- 2026-08-13 已按 Lyra 11000 收敛为唯一原生主线：男女 Rifle/Pistol/Shotgun 六个 Fire Montage
  移除空配置的 BP Notify，改用 `UShootAnimNotify_PlayWeaponMontage`；六个 Reload Montage 在保留
  `AN_Reload` 结算 Notify 的同时新增同一个原生同步 Notify。每枚 Notify 均配置对应的
  `AM_Weap_Rifle|Pistol|Shotgun_Fire|Reload`，不再存在两套同步入口并行。
- 分屏真实 `IA_Attack` 中男性 Shotgun 弹药 `7 -> 6`，角色与武器 Montage 同时 Active/Playing，
  时间位置同为 `0.178007s`。Listen Server 远端客户端 Shotgun `8 -> 7` 后，客户端本地 Pawn 与
  服务器模拟代理的角色和武器 Montage 均同步在约 `0.342s`，且 `weapon_r/-90` 挂点不变。

2026-08-02 对 `/Game/Weapons/*/Animations/AM_MM_*_Equip` 复核后确认，它们仍绑定
`SK_Mannequin` 并直接引用 Mannequin 动画，且没有运行时引用。项目已在
`/Game/Characters/Heroes/CC/MM|MF/Animations/Weapons/Montages` 为 Rifle、Pistol、Shotgun
分别创建正式 Equip Montage；同目录同时建立 Fire/Reload Montage，便于后续把三把 `ID_*`
从旧 CC 目录一次性切换到唯一正式目录。新资产只解决骨架和目录迁移，不代表切枪时序已接通。

运行时已验证男 Rifle、女 Pistol、男 Shotgun Reload 的角色/武器 Montage 时间位置完全一致；
Listen Server 中主机 Rifle Fire 在服务器 Pawn 和客户端模拟代理上的角色/武器 Montage 也同步。

# 正式 CC 动画主线

- 运行时不再创建隐藏 Mannequin 动画源 Mesh，也不在男女主 AnimBP 中使用
  `Retarget Pose From Mesh`。Lyra Manny/Quinn 资产只作为离线重定向来源。
- 项目接口为 `/Game/Blueprints/Character/Animation/ALI_ShootWeaponLayers`，当前第一纵切
  只定义 `FullBody_Aiming`。男女各自的 Unarmed/Rifle/Pistol/Shotgun Linked Layer 均实现
  此接口并绑定各自正式 CC Skeleton。
- 正式主 AnimGraph 顺序为：基础 locomotion -> 武器层 -> `DefaultGroup.UpperBodyAdditive`
  -> `HairAnimationLayer` -> `ShoeAnimationLayer` -> Output。表演 Pose、脸部 Wrinkle、
  Mutable 头发和鞋子职责没有被武器层覆盖。
- `UShootAnimInstance` 从 Pawn EquipmentManager 读取当前武器，不从本地 Controller QuickBar
  读取，因此 Listen Server 的模拟代理也能选择正确层。
- Mutable 异步重建最终 AnimInstance 后，`AShootCharacter` 通过
  `OnMutableSkeletalMeshUpdated` 再次应用当前 Equipment 层；否则外观完成会把已经链接的
  武器层还原为空手。

# B_Weapon 单 Actor 表现桥

- 项目迁入的 `/Game/Weapons/B_Weapon` 曾因 Lyra `ObserveTeamColors`、`ApplyToActor` 和旧 `FindComponentsByClass` 节点而无法编译。该队伍染色路径与 PVE 射击、伤害和本地分屏无关，不能为修复它而引入 Lyra 上游依赖。
- 2026-07-26 已修复 `/Game/Weapons/B_Weapon` 编译。Lyra 原图的阵营材质染色依赖 `UAsyncAction_ObserveTeamColors`、`ULyraTeamDisplayAsset::ApplyToActor` 和 `ULyraTeamSubsystem`；项目只保留了敌我识别接口，没有这套显示资产子系统。已删除失效的 BeginPlay 阵营颜色观察与 `UpdateTeamColors` 函数图，并将已经从 UE API 移除的 `FindComponentsByClass` 替换为原生 `Get Components By Class(PrimitiveComponent)`，继续为全部 PrimitiveComponent 设置 Custom Depth Stencil。枪口、Tracer、弹壳、命中、音频与 CustomStencil 路径均保留。MCP 编译结果为 `BS_UP_TO_DATE`，其 `B_Rifle/B_Pistol/B_Shotgun` 子蓝图亦已重新编译通过。
- `B_Weapon` 的表现闭包已经在项目中：`/Game/Effects/Blueprints/B_WeaponFire`、`B_WeaponImpacts`、`B_WeaponDecals`。2026-07-26 已通过项目 MCP 逐个重新编译，均为 `BS_UP_TO_DATE`；此前按 `/Game/Weapons` 根目录搜索而得出的“未迁入”判断已撤回。不得移动这三个资产，`B_Weapon` 的原有引用会继续使用它们。
- 若以后明确需要“按队伍改变武器材质颜色”，应在项目层实现轻量的 TeamDisplay DataAsset 与观察组件，并通过现有 `ILyraTeamAgentInterface` 驱动；不要重新依赖 Lyra 的 TeamSubsystem 或修改上游插件。该功能与当前 PVE 射击主线解耦，不能阻塞武器迁移。
- 2026-07-26 同时在项目 8000 与 Lyra 11000 临时生成三把武器并立即销毁后，确认项目 `/Game/Weapons/Rifle/B_Rifle`、`Pistol/B_Pistol`、`Shotgun/B_Shotgun` 已保留与 Lyra 一致的 SkeletalMesh 和 `ABP_Weap_Rifle/Pistol/Shotgun`。它们不是“无组件数据壳”，而是完整的武器表现 Actor。
- Lyra 的 `WID_Rifle/Pistol/Shotgun` 各自只生成对应 B_*。因此项目 `BP_Equipment_Rifle/Pistol/Shotgun` 也已收敛为只生成 B_*，旧 `BP_ShootWeaponActor_*` 不再进入这三套 Equipment，避免同一 WeaponInstance 生成两套重复网格；删除旧资产前仍须完成全项目蓝图引用审计。
- 2026-08-09 再次从 Lyra 11000 读取三份 `WID_*` 后确认：三枪均附着到 `weapon_r`，相对旋转为 Z 轴 `-90` 度。项目此前的 `weapon_socket_hand_r + Identity` 并非 Lyra 配置，现已把三份 `BP_Equipment_*` 的唯一 B_* Spawn Entry 对齐为 `weapon_r/-90`。真实 PIE 中 Rifle 与 Shotgun 横移开火的枪体、双手和枪口方向随之稳定。
- 项目 Fire GA 已保留 Lyra 的 `GameplayCue.Weapon.*.Fire` 语义，但 SourceObject 是 `UShootWeaponInstance`，不是 Lyra EquipmentInstance。`UShootGameplayCueNotify_WeaponFire` 负责从本次 Cue SourceObject 或目标 Pawn 的 CombatComponent 找到当前 WeaponInstance，再从 SpawnedActors 查找带 `Fire` 事件的 B_*，按 B_Weapon 原事件参数调用枪口、弹壳、Tracer、Impact 与贴花图表。该适配不使用全局 PlayerController，满足本地分屏和 Listen Server 边界。
- 项目 GameplayCueManager 只扫描 `/Game/Blueprints/GameplayCueNotifies` 与 `/Game/Blueprints/GameplayCues`。三个项目 GCN 必须位于 `/Game/Blueprints/GameplayCues/Weapons/Rifle|Pistol|Shotgun`；照搬到 Lyra 的 ShooterCore 路径或项目 `/Game/Weapons` 会被编辑器判为无效路径。
- 三套 Equipment 已反查为 `BS_UP_TO_DATE`，`InstanceType=UShootRangedWeaponInstance`，并分别配置 `AS_Weapon_Rifle/Pistol/Shotgun`。正式 Equip/Unequip 已通过 ItemDefinition 的男女 `FShootWeaponCharacterMontageSet` 接入；`UShootWeaponInstance` 按“链接 Item Anim Layer -> 播放角色 Montage”的顺序消费配置，并处理 FastArray 与 Instigator 的复制乱序。
- 2026-08-14 玩家复验确认此前 `EquipMontageStartPosition=0.6s` 只是掩盖缺口：项目 CC Equip/Unequip 漏迁 Lyra 的 `ScaleDownWeaponR` 与 `DisableLHandIK` 曲线。男女 Rifle/Shotgun、Pistol 与 Generic Unequip 已按各自来源和子片段窗口恢复曲线，三把 WeaponInstance 均恢复从 `0.0s` 播放完整动作。后续不得用固定跳帧替代 Montage 曲线、Notify 与 IK/FK 数据迁移。

# 迁移顺序

1. 先完成项目持枪状态桥接和角色 AnimBP 输入。
2. 仅迁移 Rifle 表现闭包：B_Weapon 表现链、B_Rifle、Rifle Mesh/Material/Texture/Sound/Animation、所需 Niagara 资产和 Rifle Damage GE。
3. 用 MCP 检查迁入资产的缺失类和编译状态；只修 Rifle。
4. 用项目 ItemDefinition、EquipmentDefinition、UShootAbilitySet 和 GA 配置替代 Lyra Inventory/Equipment/AbilitySet 父类。
5. 按同一模式再迁 Pistol 和 Shotgun。

# 首批 ItemDefinition 与副本闭环

- 2026-07-26 已在项目创建 `/Game/Weapons/Rifle/ID_Rifle`、`Pistol/ID_Pistol`、`Shotgun/ID_Shotgun`。这里的 `ID_*` 对应 Lyra InventoryItemDefinition；项目现有 `BP_Equipment_*` 对应 Lyra `WID_*` 的 EquipmentDefinition 职责，不能混淆两类资产。
- 项目 `UShootInventoryFragment_SetStats` 按 Lyra `InventoryFragment_SetStats` 的生命周期迁入：ItemInstance 创建时写入 StatTagStack。Rifle 为弹匣 30/备弹 60，Pistol 为 12/48，Shotgun 为 8/16；RuntimeOnly 拾取不会写入 SaveGame 或 Persistent QuickBar。
- `UShootInventoryFragment_RangedWeaponConfig` 已复制 11000 Lyra `B_WeaponInstance_Rifle/Pistol/Shotgun` 的三条 Heat 曲线及射程、Sweep、SpreadExponent、恢复延迟、首发精准和 BulletsPerCartridge。固定散布字段只作为未配置 Heat 曲线资产的兼容回退。
- Lyra `ULyraEquipmentManagerComponent` 的构造函数是 `EquipmentList(this)`，并设置 `bWantsInitializeComponent = true`。项目曾错误使用空 `EquipmentList()`，导致首次拾取装备时 `OwnerComponent` 为空并断言崩溃；迁移 Equipment FastArray 时这两行属于必要生命周期契约，不能省略。
- HomeMap/测试副本传送入口使用项目 `AShootMapTravelPortal`：Standalone 走 OpenLevel，Listen Server 权威端走相对 ServerTravel。返回 HomeMap 前遍历本 World 的 PlayerController，并只清理各自 Controller QuickBar 的 RuntimeOnly 会话。正式测试地图现命名为 `/Game/Maps/TestMap_SplitScreen` 与 `/Game/Maps/TestMap_ListenServer`，不再用含义不明的 TestMap/TestMap2 区分模式。
- 玩家主动丢出的拾取物由 `InitializeFromDrop` 强制为 `PressToInteract`，并生成在角色前方 120cm、上方 40cm。固定点拾取蓝图仍由资产配置自己的 TriggerMode；两条链不能用同一个 AutoOverlap 默认值，否则掉落物会被同一玩家立即捡回。
- 单人 PIE 已验证三把枪拾取、三槽切换、对应 B_* 生成、官方初始弹药、逐把丢弃、空手状态和双向换图。分屏与 Listen Server 仍是本任务的后续验收门槛，不能把当前结果写成联机完成。

# Experience QuickBar 与真实输入验收

- Lyra `LAS_ShooterGame_StandardHUD` 的权威做法是由 Experience Action 向游戏 UI Layer 推入 HUD Layout，再向 HUD Slot 注册 QuickBar，而不是在 PlayerController 构造 Widget。项目第一纵切用项目层 `UShootExperienceDefinition` 和 `UShootExperienceManagerComponent` 复现该职责边界，未修改插件或引擎。
- `/Game/GameFramework/Experiences/DA_Experience_DungeonTest` 服务测试副本模式：推入 `/Game/UI/Hud/W_DefaultHUD`，并以 LocalPlayer 为 Context 向 `HUD.Slot.Quickbar` 注入 `/Game/UI/Weapon/WBP_QuickBar`。
- 旧 `UShootHUDQuickbarComponent` 依赖 PlayerController 上未配置的 WidgetClass，也绕过 Experience/CommonUI 生命周期，已判定为未打通的双轨实现并删除。保留的 `UShootQuickbarWidgetBase` 已从错误的 Activatable 生命周期改为 Lyra TaggedWidget 风格的 Construct/Destruct 消息监听。
- 仅创建 `IA_WeaponNext/Previous` 和 IMC 映射不等于输入闭环。2026-07-27 玩家测试确认两个 QuickBar GA 的 `StartupInputTag` 为空，导致滚轮事件没有 AbilitySpec 接收；修复资产 Tag 后真实滚轮才使 ActiveIndex `0 → 1 → 0`。
- `InputTag.Weapon.Drop` 是 Native Tag，原生 Drop GA 的 CDO 早于它注册，构造期请求会得到空 Tag。最终结构为 `UShootGA_DropWeapon` 承担服务器权威业务，`/Game/Weapons/Quickbar/GA_DropWeapon` 蓝图子类配置 `StartupInputTag`，与 Lyra 的逻辑类和数据配置分工一致。
- 真实玩家链验收结果：F 拾取三把 RuntimeOnly 枪；滚轮双向切换；G 仅丢当前枪并生成对应可见拾取蓝图；有剩余武器时按 QuickBar 顺序自动切换，最后一把丢弃才进入空手；掉落枪可再次用 F 拾取。分屏玩家 0/1 各有独立 HUD 与 QuickBar。
- Lyra `/ShooterCore/UserInterface/HUD/W_QuickBarSlot` 不会用物品图标替换 `WeaponCard` Brush；它保留 `MI_UI_Base_WeaponCard`，调用 `GetDynamicMaterial` 后把图标纹理写入材质参数 `IconTexture`，从而保留卡片底色、边框、遮罩和发光层。项目 `UShootQuickbarSlotWidgetBase` 已按该方式适配，禁止重新使用 `UImage::SetBrush(SlotData.Icon)`。
- 干净双本地玩家 PIE 运行态审计确认：玩家 0 的三个占用槽均保留 `MID_MI_UI_Base_WeaponCard`，`EmptyText=Hidden`，当前槽 `ItemGlow=SelfHitTestInvisible`，非当前槽 Glow 隐藏；玩家 1 的空槽仍保留原 UMG 空槽表现。`WBP_WeaponSlot` 的 Designer 控件树、材质、尺寸与用户搭建的 Lyra 壳子均未重建。
- 2026-08-10 复验确认 Heat、姿态倍率、屏幕角度投影和准星半径公式均会随真实开火正常更新。
  快速切枪异常来自旧实现反复向同一 UIExtension 插槽注册每把枪的准星类；旧 Widget UObject
  可在 CommonUI 栈中保留，使旧的大半径准星与当前准星生命周期错位。
- 当前 `UShootHUDReticleComponent` 只按 LocalPlayer 注册一枚稳定的
  `UShootReticleHostWidget`。Host 按 Lyra `ULyraWeaponUserInterface` 的职责持续对账当前
  WeaponInstance，并在内部同步清空旧 Slate 子控件、创建唯一的新准星；子准星不再在
  `NativeConstruct` 中第二次读取 QuickBar，因此没有“注册时是 Rifle、构造时已切到 Shotgun”
  的实例竞态。组件只在第二个本地 Controller 尚未绑定 LocalPlayer 的短窗口以 0.1 秒重试，
  注册成功后立即关闭 Tick；远端 Controller 不注册 Host。
- 项目 QuickBar 切槽会对同一 WeaponInstance 反复调用 Equip，与 Lyra 通常只在真正授予装备
  时进入 OnEquip 的实例生命周期不同。若每次都把 Heat 设为区间中点，会凭空制造高散布并呈现
  “切枪后从最大准星恢复”。项目层因此从 Heat 曲线最小值开始，服务器 Trace 与 UI 仍读取同一
  `CurrentSpreadAngle`，没有按武器或性别写显示特判。
- `TestMap_ListenServer` 的网络 GameMode 必须使用 `KEEP_CURRENT`，不能沿用只允许主本地玩家的
  `PRIMARY_ONLY`。后者会在 InitGame 阶段注入 `?MaxPlayers=1`，表现为 Listen Server 主机
  正常进入地图、第二客户端却停留在 FrontEndMap；这不是复制或登录逻辑故障。
- 2026-08-07 用两套独立 Enhanced Input 子系统完成联机回归：主机 Rifle
  `30 -> 29 -> 30`，远端客户端 Pistol 在服务器和客户端两侧均为
  `12 -> 11 -> 12`，随后双方丢枪均复制为 `ActiveIndex=-1`。该结果验证的是现有
  TargetData、CommitAbility/ApplyCost、WeaponInstance 预测值和 QuickBar 复制边界，
  不需要引入 WeaponStateComponent。
- 动画蓝图显示 `BS_UP_TO_DATE` 不代表它引用的 BlendSpace 数据完整。正式男性
  `AO_MM_Pistol_Idle_ADS` 曾保留 15 个采样坐标但动画对象全部为空，只有冷启动加载时才报告
  invalid sample；当前按源资产同名、同坐标关系接入 15 个正式 MM 序列。迁移动画后必须同时
  扫描 AnimBP 编译状态和 BlendSpace/AimOffset 的 `sample_data.animation`。

# Mannequin 依赖审计补充

- 只扫描正式 AnimBP 的编译状态不足以证明 Mannequin 已退出运行数据。2026-08-12 使用
  AssetRegistry 复核男女 18 个正式 AnimBP/动画接口，得到 9 个直接 Mannequin 硬依赖包；
  它们包含枚举、结构、接口、父 AnimBP、Transition Notify、Skeleton、ControlRig 与
  IK Retargeter。递归闭包为 37 个包，父类默认值还会带入 Mannequin Sequence 和 AimOffset。
- 这 18 项的 `target_skeleton` 已逐项确认正确：MM 九项是 `ChenHaoYu_Skeleton`，MF 九项是
  `ShenWanYun_Skeleton`。所以 `SK_Mannequin` 的包依赖不能被误报成 AnimBP Target Skeleton
  回退，但也不能在没有定位引用字段前直接删除。
- 扩展到男女正式 CC 目录 742 个资产后，共有 49 个 Mannequin 依赖包、1157 条直接硬依赖边。
  AnimationModifier、压缩设置、Retarget Source 等编辑器链路可以作为迁移源暂留，但必须与
  `sample_data.animation`、`ref_pose_seq` 这类实际姿势数据分开统计。
- 2026-08-13 已修复上述运行姿势数据：MM 31 个 `ref_pose_seq` 全部改为同侧 ChenHaoYu Base；
  MF 两个 AimOffset 新建并接入 30 个同侧 ShenWanYun 正式样本，Preview Base 也改为同侧正式
  Idle。复读为 30/30 样本有效、剩余 Manny Base 为 0，15 点坐标和 Mesh Space Additive 语义保持。
- Lyra 11000 复读确认 Rifle Hipfire、Unarmed Ready 与 Pistol ADS 都是相同的 15 点 AimOffset
  网格；方向 Sequence 使用 Mesh Space Rotation Offset Additive，Base Pose 为同组中心样本的
  第 0 帧。项目 Mannequin 目录中的这 30 条源序列实际已绑定 ShenWanYun Skeleton，因此修复采用
  复制到正式 MF 目录并恢复 Additive 元数据，避免对已是目标骨架的数据再次 IK Retarget。

# 队伍伤害与敌人死亡验收

- Lyra 的 `ULyraTeamSubsystem::FindTeamFromObject` 不只对 Avatar Pawn 本体 Cast 队伍接口：
  玩家 Pawn 会继续解析 PlayerState，AI Pawn 会继续解析 Controller。项目伤害上下文同样传入
  Avatar Pawn，因此 `AShootGameModeBase::GetFriendlyFireScalarForActors` 必须沿这条关联链取队伍。
- 2026-08-02 修复前，两个玩家 PlayerState 都保持 NoTeam，且 GameMode 只对 Pawn 本体 Cast
  `ILyraTeamAgentInterface`，导致配置为 0 的 FriendlyFireScalar 实际返回 1。修复后权威端仍为
  NoTeam 的 `AShootPlayerState` 初始化为 Team 1；`AShootPlayerController` 按 Lyra 模式只代理
  PlayerState 队伍并转发委托，`ULyraLocalPlayer` 继续观察 Controller，不保存第三份 TeamId。
- 冷编译与双分屏 PIE 实测：两个 PlayerState 均为 Team 1；Pawn 与 Controller 两种入口下
  `P0 -> P1` 倍率均为 0，`P0 -> Enemy` 倍率为 1。实际 Rifle 射击中，友军枪消耗 1 发但
  Health 276、Shield 102.12 均不变；对齐敌人后的 1 发使 Health 276 降为 266。
- 敌人 Health 降为 0 后 `IsDead=true`，Capsule 为 NoCollision，能力被取消。`AShootCharacterBase`
  只负责进入死亡状态；`AEnemyBotCharacter` 记录 BeginPlay 初始 Transform；`AShootGameModeBase::PlayerDied`
  根据地图配置清理尸体并延迟生成相同敌人蓝图类。TestMap 的 `BP_TestGameMode` 开启该规则，
  其他地图默认关闭。实测重生实例满血、`IsDead=false`，并自动获得新的 EnemyBotController。

# 不可做的迁移

- 不迁移整个 `/Game/Weapons` 根目录。
- 不迁移 Lyra WeaponStateComponent 或其命中确认资产。
- 不把 `LyraAbilitySet`、`LyraEquipmentDefinition`、`LyraInventoryItemDefinition`、`LyraReticleWidgetBase` 当作项目运行时父类。
- 不在没有验证骨架重定向前，将 Manny/Quinn 动画直接绑定到男女主 CC Skeleton AnimBP。
