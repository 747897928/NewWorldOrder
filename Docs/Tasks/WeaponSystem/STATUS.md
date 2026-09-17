---
task_id: WeaponSystem
status: in_progress
assigned_to: Codex
progress: 95%
started: 2026-07-25
---

# 任务状态

当前阶段：QuickBar Controller 主线、RuntimeOnly 跨 Pawn 交接、准星 UIExtension、七把武器的 ItemDefinition/Equipment/WeaponInstance 闭环、正式 CC 男女武器动画层和角色/武器 Montage 同步已落地。Rifle/Pistol/Shotgun/Rocket/Grenade/Shotgun_A/Sniper 的核心开火、换弹、ADS 与 Listen Server 行为已完成玩家验收；Shotgun_A 和特殊武器动画的美术修型属于独立动画任务。当前工程收尾主线是弹药 StatTag 单一数据源与项目级 CameraMode/第一人称基础。

# 已确认事实

- WeaponInstance 已是 UObject，不是旧文档所述的 Actor。
- 弹药已使用 `UShootInventoryItemInstance` 的 StatTagStack。
- 弹药容量迁移使用 `Inventory.Ammo.MagazineCapacity` 和 `Inventory.Ammo.ReserveCapacity`；新武器的当前弹药与容量都在 `UShootInventoryFragment_SetStats.InitialItemStats` 配置，不再在 Ranged Fragment 重复填写。详见 `AmmoStatTagMigration_弹药StatTag单一数据源与接口精修.md`。
- `UCombatComponent` 已按 QuickBar 槽位创建/卸载 Equipment，并通过 Equipment 授予武器能力。
- 开火能力在 `UShootGameplayAbility_Weapon_Fire` 中完成本地预测 TargetData、消耗 Cost 后的散布累加；服务器收到预测链路后以自身视角、碰撞场景和当前散布重新 Trace，最终伤害不信任客户端 HitResult。
- `UShootHUDReticleComponent` 已改为按 Owning PlayerController 在 `HUD.Slot.Reticle` 注册 Widget 类；不再使用 `AddToViewport` 或 `RemoveFromParent`。`UShootReticleWidgetBase` 自己从 Owning QuickBar 读取武器并过滤本地 GameplayMessage。2026-07-26 已在 `W_DefaultHUD` 添加 `UUIExtensionPointWidget`，标签精确匹配 `HUD.Slot.Reticle`，Canvas Slot 为全屏锚点、零边距、ZOrder 10；该布局让每个 LocalPlayer 的准星在自己的 HUD 视口居中，不会落到全局左上角。
- 已修复 `UShootRangedWeaponInstance` 的散布恢复：它不再依赖没有调用者的 UObject Tick，而是由开火/读取时按世界时间惰性恢复，因此服务器命中散布与本地准星半径一致。
- 已按 Lyra `ULyraRangedWeaponInstance` 接入可选 Heat 曲线散布模型：三条 Heat 曲线齐全时使用 Heat、每发升温、延迟冷却和首发精准；曲线缺失时不再回读 Fragment 的旧固定散布字段，也不保留第二套散布状态。Windows Game Target 已编译通过。
- 已验证 Heat 模型会在装备后自然冷却到最小散布，首发精准不再依赖先开火一次。
- 已将“服务器按散布结算”落实到基础开火能力：不引入 Lyra WeaponStateComponent 或严格校验队列，客户端只保留即时开火表现和本地命中预览；服务器重新生成最终 TargetData、命中特效和伤害。Windows Editor 已编译通过。
- 已补齐 `UShootGA_Weapon_Fire_Pistol`：它复用 Rifle 已验证的服务器权威 Trace、伤害和本地后坐力链，只切换 Pistol Fire/Impact Cue 语义与默认开火节奏；Pistol AbilitySet、角色 Montage 和 `GE_Damage_Pistol` 已由编辑器 CDO 回读确认。
- 扳机模式由 Fire GA 的 `UShootGameplayAbility.ActivationPolicy` 表达，不重新放回 Item Fragment：
  Rifle 与 Assault_Rifle_A 直接授予 `UShootGA_Weapon_Fire_Rifle`，使用 `WhileInputActive` 标识全自动语义。
  Rifle GA 在同一次激活中按 `FireDelayTimeSecs` 驱动预测射击循环，并由 `WaitInputRelease` 在松开时清理
  定时器；不能依赖“Montage 结束后由 Held 再激活”，该旧尝试会退化成长按只打一发、松开附近再补一发。
  Pistol 虽继承 Rifle 的命中实现，但在 `UShootGA_Weapon_Fire_Pistol` 明确恢复 `OnInputTriggered`。Shotgun、Shotgun_A、
  Sniper、Rocket 和 Grenade Launcher 都不继承 Rifle GA，继续保持一次按压一次开火。未来新增 Burst
  或某把全自动手枪时，应新增明确 GA 子类或正式武器行为数据项，不能靠 WeaponId 分支，也不能修改
  Pistol 的共享默认值。
- Rifle 第一发在按下时立即提交，第二发及后续每发至少间隔 `FireDelayTimeSecs=0.10s`，即默认约
  600 RPM；该间隔不等待开火音频播放完毕。Rifle 的正式 MetaSound 由持久 AudioComponent 接收逐发
  Trigger，单发尾音允许跨过下一发并自然重叠，弹匣打空后仍听到尾音不等于继续扣弹，也不能通过
  Ability 结束时 Stop 音频来截断尾音。PIE 已验证 Pistol 长按 0.75 秒只消耗 1 发，松开后弹药保持
  不变；Rifle 松开后弹药在后续 1.35 秒保持稳定。最终射速与尾音听感仍以玩家验收为准。
- `UShootAnimInstance` 已改为从当前 Pawn 的 `UShootEquipmentManagerComponent` 读取
  `UShootRangedWeaponInstance`，模拟代理不再依赖本地不存在的 Controller QuickBar。项目尚无
  Aim Ability 的可复制状态标签，因此第一纵切只迁入 hip-fire 正式 CC locomotion，
  不沿用旧 Character API 伪造瞄准或 ADS 状态。
- Rifle、Pistol、Shotgun 的项目 WeaponActor、AbilitySet、GA、GE、Reticle、武器/角色 Montage、AnimBP、特效和音效闭环已迁入；Lyra 资产只作为只读迁移参考，不是运行时依赖。
- 项目已有 Mannequin Rifle/Pistol 动画，但男女主使用 CC Skeleton，必须先验证重定向方案。
- 已通过 Lyra MCP 完成核心蓝图审计。`B_Weapon` 表现链与 Damage GE 属于可迁移资产；GA、AbilitySet、Inventory/Equipment Definition 与 Reticle 属于需要转换父类或手工复刻的参考资产。
- `AShootPlayerController` 现已创建 `UShootQuickBarComponent`，为后续切槽、角色切换和 HUD 提供 Controller 生命周期入口。
- `UCombatComponent::FindActiveWeaponInstance()` 已提供无副作用查询；旧的 `GetActiveWeaponInstance()` 仍可能惰性创建 Equipment，因此禁止新增 HUD 或动画调用点。
- 准星和 QuickBar Widget 的初始读取已改为 Owning PlayerController 的 `UShootQuickBarComponent`，避免分屏 Widget 直接访问 Pawn CombatComponent。
- `AShootPlayerController::OnUnPossess` 会保存旧 Pawn 的 RuntimeOnly 槽位 Guid 与激活索引，并卸载旧 Pawn 的装备表现但不删除临时 ItemInstance；`OnPossess` 会在新 Pawn 完成初始化后重新绑定这些会话武器。Persistent 槽位、角色快照和 SaveGame 不进入 Controller。
- `AShootWeaponPickupActor`、`AShootResourcePickup` 与 `AShootInventoryGrantActor` 的 RuntimeOnly 自动入槽已改走 Controller QuickBar。Persistent 物品仍走既有 Hub 配置链，避免局内入口污染账号配置。
- Controller QuickBar 已提供 RuntimeOnly 专用的选槽、循环切枪、槽位交换和丢弃接口。`DA_ShootInputConfig` 已正式绑定 `IA_WeaponNext/Previous/Drop` 到 `InputTag.Weapon.Next/Previous/Drop`；IMC 使用滚轮、手柄方向键和 G 配置，C++ 没有硬编码设备分支。
- `GA_QuickbarNext/Previous` 旧资产的 `StartupInputTag` 原本为空，是“IMC 有映射但玩家不能切枪”的直接原因；现已分别配置正式 Tag。`GA_DropWeapon` 使用原生服务器权威业务类加蓝图配置子类，避免 Native Tag 注册晚于原生 CDO 构造而得到空 Tag。
- 已通过项目 MCP 复核 `/Game/Blueprints/Animations/AnimNotifies/AN_PlayWeaponMontage`：它从通知所属 Pawn 的 CombatComponent 获取当前 WeaponActor，再将其 Mesh AnimInstance 作为 Montage Sync Follow 的 Follower。该路径满足本地分屏的 Pawn 表现桥约束；Mannequin 目录中的 Lyra 原始 Notify 只作迁移参考，不直接绑定项目角色 Montage。
- 已修复迁入的 `/Game/Weapons/B_Weapon`：删除未迁入 Lyra 队伍材质观察链，替换废弃的 `FindComponentsByClass` 节点为 UE 原生 `Get Components By Class`，并保留 CustomStencil 与全部射击表现链。`B_Weapon`、`B_Rifle/B_Pistol/B_Shotgun` 均已在重启后的编辑器中编译通过。
- `B_WeaponFire/B_WeaponImpacts/B_WeaponDecals` 实际位于 `/Game/Effects/Blueprints`，并非 `/Game/Weapons`。已全局检索并重新编译验证三者均正常；项目保留 Lyra 武器表现闭包，不移动资产路径。
- 2026-07-26 对 11000 Lyra 实例复核后确认：官方 `WID_Rifle/Pistol/Shotgun` 的 `ActorsToSpawn` 各只有对应 `B_Rifle/B_Pistol/B_Shotgun`；B_* 本身已经同时承担 SkeletalMesh、武器 AnimBP 与 `Fire` 表现，不需要再生成项目 `BP_ShootWeaponActor_*`。项目三套 Equipment 已收敛为同样的单 Actor 模式，继续附着到项目角色 Socket `weapon_socket_hand_r`。
- 项目 Fire GA 发出逐武器 `GameplayCue.Weapon.*.Fire`，对应 `GCN_Weapon_*_Fire` 位于 GameplayCueManager 可扫描的 `/Game/Blueprints/GameplayCues/Weapons/*`。`GCN` 只承接一次开火的本地表现，不持有弹药、命中或伤害权威。项目适配类 `UShootGameplayCueNotify_WeaponFire` 从 Cue 的 `UShootWeaponInstance` 找到 Equipment SpawnedActor，再调用 `B_Weapon.Fire` 生成枪口、弹壳、Tracer、Impact 与 Decal。声音采用唯一入口：Rifle 的 GCN 通过 `B_Weapon.TriggerFireAudio` 向同一个 AudioComponent 重复发送 Trigger；Pistol 与基础 Shotgun 的 GCN 在 `BurstEffects` 每发各生成一次对应 MetaSound；`Shotgun_A` 共享 Shotgun GA/GCN，因此同样使用正式霰弹枪 MetaSound。Sniper、Rocket、Grenade Launcher 当前保留各自武器 AnimSequence 中的专用 Fire Sound Notify，GCN 不再叠加通用枪声。七把武器的配置矩阵和新增武器规则见 `WeaponFireAudio_开火音频接入规范.md`。
- 三套既有 Equipment 当前均为 `BS_UP_TO_DATE`，`InstanceType=UShootRangedWeaponInstance`，并分别授予 `AS_Weapon_Rifle/Pistol/Shotgun`。Equip 已接入 QuickBar 实际切槽；武器切武器时播放新武器 Equip，切到空手时播放 `AM_Generic_Unequip`。既有 `/Game/Weapons/Shotgun` 继续使用通用整匣 Reload；只有商城泵动式 `/Game/Weapons/Shotgun_A` 使用已经重写和验证的逐发装填 GA，不能把两类配置混用。
- 已审计 `/Game/Weapons` 下全部 7 个 `ID_*`。Rifle/Pistol/Shotgun/Shotgun_A/Sniper 各为 `EquippableItem + WeaponBasicConfig + RangedWeaponConfig + SetStats` 四个 Fragment；Grenade Launcher/Rocket Launcher 额外各有一个 `ProjectileWeaponConfig`。全部 ID 均无 Fragment `BaseDamage`、旧 Falloff、材质伤害、射速、Burst、自动开火或旧散布字段；伤害 GE、B_WeaponInstance 曲线/材质倍率和投射物半径分别归各自职责层。
- Lyra `ULyraEquipmentManagerComponent` 构造时使用 `EquipmentList(this)` 并开启 `bWantsInitializeComponent`。项目迁移时漏掉这两项，第一次拾取会在 `FShootEquipmentList::AddEntry` 对空 `OwnerComponent` 断言；现已按官方实现修复并完成崩溃后重启复验。
- `/Game/Maps/HomeMap` 已放置 `DungeonPortal_ToTestMap`，`/Game/Maps/TestMap` 已放置返回入口和 Rifle/Pistol/Shotgun 三个固定点 `PressToInteract` 拾取物。TestMap 保留两个 PlayerStart，为后续分屏验收提供独立出生点。
- 单人 PIE 已确认：初始 QuickBar 为 `[空, 空, 空]`；三把枪拾取后分别生成唯一 `B_Rifle/B_Pistol/B_Shotgun`；三个槽位可权威切换；丢弃物在角色前方 120cm 生成并强制 `PressToInteract`，不会被同帧 AutoOverlap 捡回；全部丢弃后 `Active=-1` 且为空手。
- 2026-07-27 双本地玩家 PIE 已从玩家 0 的真实 Slate/Enhanced Input 链验证：F 依次拾取 Rifle/Pistol/Shotgun，滚轮上/下切槽，G 只丢当前 RuntimeOnly 武器并生成对应 `BP_WeaponPickup_*`，有剩余枪时自动切到下一把，F 可重新拾取；最后一把丢弃后 `Active=-1` 且三个槽为空。此验收没有用直接调用 QuickBar 函数替代玩家输入。
- `/Game/UI/Weapon/WBP_QuickBar`、`WBP_WeaponSlot`、`WBP_WeaponAmmoAndName` 已分别接入项目消息驱动基类；TestMap 由 `DA_Experience_DungeonTest` 通过 `W_DefaultHUD` 的 `HUD.Slot.Quickbar` 默认注入。旧 Controller 直挂 `UShootHUDQuickbarComponent` 已删除，避免双轨 UI。
- QuickBar 槽位图标已按 Lyra `W_QuickBarSlot` 恢复为材质参数更新：保留 `MI_UI_Base_WeaponCard`，仅向动态材质 `IconTexture` 写入物品图标，不再用裸图标覆盖 `WeaponCard` Brush。双本地玩家 PIE 已确认三把枪对应槽均使用 `MID_MI_UI_Base_WeaponCard`，空槽和选中 Glow 仍由原 UMG 壳子表现。
- Standalone PIE 已确认 HomeMap → TestMap 与 TestMap → HomeMap 双向换图。返回入口在 OpenLevel 前调用 Controller QuickBar `ClearRuntimeSession`，回到 HomeMap 后新 Pawn 的 Combat 与 Controller QuickBar 均为 `Active=-1`、无 RuntimeOnly 武器。Listen Server 分支使用服务器权威相对 `ServerTravel`，仍待双端实机验收。
- 2026-07-31 已删除运行时隐藏 Mannequin 动画源 Mesh。男女主正式 CC AnimBP 直接使用
  `/Game/Blueprints/Character/Animation/ALI_ShootWeaponLayers`，输出顺序保持
  `武器层 -> DefaultGroup.UpperBodyAdditive -> HairAnimationLayer -> ShoeAnimationLayer`。
  Mutable 异步完成外观后由 `OnMutableSkeletalMeshUpdated` 重放当前 Equipment 动画层。
- 沈婉芸与陈浩宇各自拥有 Unarmed/Rifle/Pistol/Shotgun 正式 Linked Layer；男性新增三套正式
  CC locomotion BlendSpace，女性 Shotgun Idle 样本已修正。运行时关键资产直接依赖审计中
  不再包含 `/Game/Characters/Heroes`。
- 12 个男女正式 CC Fire/Reload Montage 均改用项目 C++
  `UShootAnimNotify_PlayWeaponMontage`。它从 Pawn EquipmentManager 取得当前 WeaponInstance 和
  SpawnedActor，先播放武器 Montage，再让它跟随角色 Montage；不依赖旧 CombatComponent Notify
  蓝图，也不修改插件或引擎。
- TestMap 双本地玩家已确认：男 Rifle、女 Pistol、男 Shotgun 和空手层互不串线；男 Rifle、
  女 Pistol、男 Shotgun 的角色 Reload 与武器 Reload Montage 时间位置一致。
- TestMap2 Listen Server 已确认：服务器 World 有一名本地、一名远端 Controller，客户端
  World 只有一名本地 Controller；主机 Rifle Equipment/正式层复制到客户端远端代理，主机
  Rifle GAS Fire 在服务器和客户端代理上的角色/武器 Montage 采样位置均为 `0.130000s`。
- Listen Server 验收发现客户端 QuickBar/ActiveIndex 可能早于 Equipment FastArray 到达，
  导致首次刷新时准星找不到 WeaponInstance。`UShootHUDReticleComponent` 现只在本地 Controller
  上以 0.1 秒低频对账补齐复制顺序；复验中客户端本地 QuickBar 为 Pistol、Active=0，
  `W_Reticle_Pistol` 正确绑定客户端本地 OwningPlayer。
- 2026-08-17 已修复男女正式 `ABP_Mannequin_Base` 的 GameplayTag 属性映射持久化问题。根因是 `FGameplayTagBlueprintPropertyMap` 中保存了跨蓝图或已失效的变量 GUID，包重载后属性解析退化为 `None`；男女 AnimBP 并不互斥。现已用各自当前变量 GUID 重建并完成多轮编译、包重载后再编译，二者均为 `BS_UP_TO_DATE`。以后每次 PIE 前先显式编译这两个资产。
- 男女 6 个正式 Equip Montage 已按 Lyra 对齐 `ScaleDownWeaponR`、`DisableLHandIK` 的插值/切线和 `sfx_WeaponSwap_nl_meta_Preset` PlaySound Notify。第二条 Timing 标记就是该声音 Notify，不是缺失的动画轨。
- 男性 Rifle/Pistol 的 4 条 Equip Sequence 已用关闭重定向阶段手臂 IK 求解的方式重新导出；正式 Montage 与 Manny 源动作逐帧已基本一致，额外肘部过弯得到收敛。剩余写实 CC 上的橡皮感属于源换枪姿势与 CC 比例/蒙皮的美术适配缺口，后续应制作 CC 专用 Equip 动画或做 Control Rig/DCC 修型，不能继续归咎于 Generic Unequip、曲线、音效 Notify 或 C++ 播放链。

# 当前首要任务

2026-08-25 用户指定的当前顺序是：先由玩家手工验收 `Shotgun_A` 功能验证版；通过后回到商城武器主线，开始 Cascade 到 Niagara 的实际迁移。动画离线合成已拆成独立试水文档，可并行尝试，但不阻塞本轮 Shotgun_A 功能验收。下面的 QuickBar 段落保留为仍然有效的系统架构约束和历史验收边界，不代表覆盖该最新顺序。

QuickBar 主线收敛的第一轮已完成，详见 `QuickbarArchitecture_架构决策.md`。

现有 `UCombatComponent` 复制了 Lyra QuickBar 的核心流程，但职责过载，并存在读取当前 WeaponInstance 时可能创建 Equipment 的风险。先建立 Controller 上的项目 QuickBar 主线和无副作用查询，再把动画、HUD、存档与角色切换逐步迁移过去。

下一小任务验收目标：先完成男性 CC 专用 Equip 姿势修型的手工视觉验收；随后由玩家在可见窗口完成一轮手工手感验收，重点检查男女角色八方向脚步、
枪口与手部贴合、三个准星视觉尺寸、鼠标和手柄后坐力体感，以及 Listen Server 客户端真实
按键拾取/丢枪和 TestMap2 返回入口的 ServerTravel。自动化已验证服务器权威客户端拥有者
Pistol 发放、QuickBar 复制、正式层和本地准星，但 MCP 在同一进程内直接调用客户端交互 RPC
会阻塞编辑器工具线程，因此不把该调用写成“真实客户端按键已完成”。PlayerState 的 Persistent
出战配置、角色快照和 SaveGame 不迁入 Controller。

# 后续任务门槛

- 未完成 QuickBar 主线和持枪状态任务前，不接入武器专属蒙太奇。
- 未完成 CommonUI 准星任务前，不继续扩展旧 `AddToViewport` 方案。
- 每个任务完成后必须编译、PIE 单人、双本地玩家和 Listen Server 客户端回归，并提交推送。

## 手雷与治疗包表现验收 (2026-08-23)

- 手雷表现已接入项目原生 `UShootGameplayCueNotify_Presentation`：爆炸 Cue 配置 `NS_Grenade_Explosion`、`sfx_Weapon_GrenadeExplosion_nl_meta` 和 `ATT_Grenade`；投掷释放声由 `GA_Grenade` 的 `ThrowSound` 配置在本地控制端播放。`AShootProjectileBase` 不再直接生成爆炸 Niagara，避免 Listen Server 重复表现。
- GameplayCue Runtime Object Library 已延后到 Asset Registry 初始 gather 完成后扫描，修复编辑器刚启动时 CueSet 为空导致 Cue 不执行的问题。
- 治疗包保持交互拾取链：`AShootHealthpackPickup` -> `UShootGA_Interaction_HealthPack` -> `UShootEffect_HealInstant`；实际治疗量大于零时执行 `GameplayCue.Character.Heal`。默认 `RespawnDelay=30` 秒，消耗期间隐藏并关闭碰撞，服务器计时后恢复；设置为 `0` 时才销毁拾取物。当前治疗 Cue 暂用 `NS_Pickup_Success`，因为本项目尚未迁入 Lyra 的 `NS_Heal` 与 `sfx_Heal_nl_metaPreset`。
- TestMap Listen Server 已用可见踩踏火焰夹具验证回血包路径：`/Game/Blueprints/Testing/BP_TestDamageHazard` 使用 `NS_Fire` 显示火焰，玩家不需要按 `F`，进入 Box 后自动挂周期 GE。测试 GE `UShootEffect_TestHazardDamagePeriodic` 现在写入 `IncomingDamage`，正式后处理按 `Shield` 优先、剩余伤害再扣 `Health`；离开后 GE 数量归零且属性停止变化。
- 暂存 audio/effect 资产已按实际依赖闭包审计。手雷 Cue、GA 和 Niagara 依赖保留；无引用的旧爆炸 MetaSound、Noise/Punch/Ricochet/SFX 变体及其独占 MetaSound 节点已删除；现有 Audio Modulation 修改因仍被 Audio Classes/ParameterPatches 引用而保留。详见 `GrenadeHealthpack_实施记录.md`。

## 商城武器资产迁移第一阶段 (2026-08-24)

- 已通过 UE MCP/AssetRegistry/AssetTools 将 `MilitaryWeapSilver` 与 `FPS_Weapon_Bundle` 的 650 个真实资产按枪族迁入 `/Game/Weapons`，目标无路径冲突；弹药、骨骼、网格、动画、材质、纹理、音频、Cascade 和 Control Rig 均随所属枪族归档。
- 两个源根目录已无真实资产，只剩重定向器。重定向器清理、外部引用 Fix Up 和 Cascade/Niagara 取舍仍是后续资产治理任务；四类特殊武器的 GAS/GE、弹药和表现最小闭环已落地，详见 `SpecialWeapons_GE迁移与Fragment字段审计.md`。
- 本阶段未修改上游插件或引擎源码，未覆盖现有 `Assault_Rifle_A_Diff` 项目目标，也未自动删除源目录或重定向器。

## 商城武器资产迁移第二阶段审计 (2026-08-24)

- 重启 NewWorldOrder 编辑器后确认两个源根目录共 653 个 `ObjectRedirector`、真实资产为 0；91 个重定向器仍被 56 个源目录外包引用，当前不能删除源目录。
- Fix Up Redirectors 只完成影响评估，没有自动执行。该操作会重存大量目标资产并删除多个重定向器，按项目安全规则交由用户在 Content Browser 人工执行后再复核。
- 25 个 Cascade 已逐项列出真实引用、LOD、动画 Notify 时序和 Niagara 语义候选。21 个具有 3 个 LOD，Epic 转换器只处理 LOD 0；另外还有 AnimTrail/Beam 不支持和 Ribbon 部分支持等限制。
- 可证明一对一 Niagara 等价的项目为 0；本阶段没有转换、删除或修改动画 Notify。详细证据见 `CascadeNiagaraAudit_审计报告.md`。
- `BP_Negative_HealthPotion`、`GCN_Weapon_Impact` 重启后均为 `BS_UP_TO_DATE` 且依赖新路径，但其接手前已有的序列化修改继续留在工作区，不纳入本阶段提交。

## 商城 Shotgun A 逐发装填第一纵切 (2026-08-25)

- 已建立独立的 `ID_Shotgun_A -> BP_Equipment_Shotgun_A -> B_WeaponInstance_Shotgun_A -> B_Shotgun_A` RuntimeOnly 闭环；AbilitySet 授予 `UShootGA_Weapon_Fire_Shotgun` 和 `UShootGA_Reload_ShotgunPerShell`。
- 逐发 GA 已从早期未接入代码重写为项目当前 SourceObject、ItemInstance StatTagStack 和角色骨架 Montage 映射链。`UShootAnimNotify_InsertShell` 只在动画提交点发送 `GameplayEvent.Reload.InsertShell`，服务器每次只转移一发。
- Reload Montage 使用 `ShotgunStart/ShotgunLoop/ShotgunEnd`。非空弹匣允许开火取消 Reload；空仓开始换弹时用 `Ability.Weapon.NoFiring` 阻止开火。当前不区分膛内弹药，也没有隐式加入 Source 风格的排队开火。
- 逐发 GA 只接受 WeaponInstance 的显式角色骨架 Reload Montage；点击一次 Reload 后持续逐发装填，装满、无备弹或主动结束事件进入 `ShotgunEnd`，未经过 InsertShell 提交点的弹药不会增加。Fire GA 不再保留 Character Montage 兜底字段。
- 角色 Fire/Reload Montage 通过 `UShootAnimNotify_PlayWeaponMontage` 子类驱动武器 Montage，并继续使用 `MontageSync_Follow`。慢放 PIE 在 0.16000、1.58333、2.63333 秒均采到角色和武器相同位置；从 `5/12` 逐发装到 `8/9`。
- 四个角色 Fire/Reload Montage 已从 `/Game/Weapons/Shotgun_A/Animations/Character/MF|MM` 移入 `/Game/Characters/Heroes/CC/MF|MM/Animations/Weapons/Montages`；`ID_Shotgun_A` 已直接引用新路径，旧路径仅保留本次受控移动产生的四个重定向器。
- `TestMap_ListenServer` 已放置一个 `BP_WeaponPickup_Shotgun_A`；`TestMap_SplitScreen` 已放置两个。第一名 LocalPlayer 通过正式交互输入拾取、开火、逐发装填、非空打断、空仓门禁、蹲下、跳跃和移动输入链完成复验。
- Split Screen PIE 有两个本地 Controller 和两个 Shotgun_A 拾取物；Player 0 拾取后 Player 1 仍无 Active Weapon，第二个拾取物仍存在，入口未串玩家。
- 运行时武器使用用户修正后的 `M_Shotgun_01`。本轮没有覆盖该材质修改。
- 2026-08-28 已纠正 `WaitInputRelease` 导致普通点击 R 在首个提交点前结束的问题。Listen Server Host 自动化复验为 `7/16 -> 8/15`；从 `3/12` 连续提交到 `5/10` 后开火打断为 `4/10`。远端拥有客户端复制仍待双端验收。
- 2026-08-28 已将共享 `UShootGA_Weapon_AutoReload` 授予 Shotgun_A，并保持逐发 GA 独占 InsertShell、循环和打断时序。Listen Server Host 空仓复验为 `0/24 -> 8/16`；零备弹 `0/0` 连续 3 秒不激活 Reload。切枪时旧被动由 Equipment GrantedHandles 的 `ClearAbility` 链移除；玩家随后完成了远端拥有客户端的 AutoReload、逐发复制、Montage/开火打断与模拟代理验收。
- 女性使用商城 CC 动画；男性已生成独立离线重定向结果。两者尚未完成“商城上半身 + Lyra/CC 下半身站姿”的最终动画修型，不把当前功能动画标记为美术完成。已新增可交给 Blender 5.2 或 UE Control Rig 执行的 `ShotgunAnimationBlend_动画合成试水.md`。
- 本纵切没有迁移或删除 `P_Shotgun_MuzzleFlash_01`，也没有完成 Listen Server 双端验收。完整资产清单、时序、运行证据、玩家入口和后续边界见 `ShotgunPerShell_逐发装填接入.md`。

## 四类特殊武器 GE 迁移与字段审计 (2026-08-25)

- Sniper：`ID_Sniper_Rifle_A -> B_WeaponInstance_Sniper_Rifle_A -> GE_Damage_Sniper_Rifle_A`，Fire/Reload 使用原生 `ShootGA_Weapon_Fire_Sniper`、`ShootGA_Reload_Sniper`；Sniper 只使用已有的通用 Rifle Impact Cue，避免注册没有资产承载的伪专用 Impact Tag。
- Grenade Launcher/Rocket Launcher：均由 `ShootGA_Weapon_Fire_Projectile` 派生 Fire GA，在服务器生成复制投射物；爆炸由 `FShootGameplayEffectContext + UShootDamageExecution` 应用各自 Damage GE。手持手雷 `ShootGA_ThrowGrenade` 仍是独立能力，不冒充发射器。
- GL 使用 `AShootProjectileGrenade`、3 秒引信、弹跳、200/500cm 线性衰减；Rocket 使用 `AShootProjectileRocket`、1800cm/s、无重力、300/600cm 无衰减。两者均关闭爆炸物理材质倍率，并保留原始 Cascade 尾迹；Rocket Fire Montage 的前后喷口仍分别由 `MuzzleFlashRear`/`MuzzleFlash` Socket 和独立时间点驱动。
- 特殊武器初始 GE 数值为 Sniper 80、GL 100、Rocket 140；这是当前纵切的可调候选，不替代产品最终平衡验收。Rocket/Grenade 的 Listen Server、真实输入、模拟代理表现和服务器权威伤害已在 2026-08-27 完成最终闭环；Sniper 的 ADS、Scope、枪口表现及 Listen Server 也已在 2026-08-28 完成玩家验收。

## 武器拾取表现与配置冗余审计 (2026-08-26)

- 方案 B 已确认：`AActor` 本身没有可渲染 Mesh；`AShootWeaponPickupActor` 的 `Collision` 是 `USphereComponent`，因此必须保留父类上的 `VisualComponent`（唯一 StaticMesh 入口），删除拾取蓝图本地重复的 `SkeletalMesh`。装备后的 SkeletalMesh 仍属于独立 Equipment Actor。新增武器必须在父类 Visual 配置武器本体 StaticMesh，不能按商城资产的 `Pickup` 后缀猜语义。
- 已通过 UE 编辑器资产接口统一 7 个拾取蓝图：四个特殊武器的原 SkeletalMesh 已分别转换为武器本体 StaticMesh，回写原材质后配置到父类 Visual；Pistol、Rifle、旧 Shotgun 的既有转换资产继续使用。7 个蓝图本地 `SkeletalMesh` 均已删除。视觉核对发现四个历史 `*_Pickup` 模型实际是弹药/小型拾取道具，已重命名为 `SM_*_AmmoPickup` 或 `SM_Shotgun_A_AmmoBox`，避免后续 AI 误用。
- 7 个拾取蓝图的 `InteractionText` 覆盖值已清空，且父类 C++ 的该覆盖字段已删除，交互名称统一使用 ItemDefinition `DisplayName`；`InteractionSubText` 的 RuntimeOnly 提示保留。7 个 `WorldLabel` 已校正为各自武器名称，它是可选世界文字，不是 StaticMesh 入口。
- 当前 `TestMap_ListenServer` 已完成单人 PIE 回归：Pistol、Rifle、旧 Shotgun、Shotgun_A 均通过真实 `F` 输入拾取，拾取 Actor 销毁，Controller QuickBar 读到对应 RuntimeOnly 武器；测试只临时传送 PIE 玩家到拾取物附近定位，不直接调用拾取函数。
- 已从 `WeaponBasicConfig` 的 C++ 定义删除无消费者且与 ItemDefinition 根部重复的 `DisplayName`、`Icon`、`AmmoIcon`、`WeaponTags`，并从武器拾取父类删除无消费者的 `InteractionText` 覆盖字段，保留 `WeaponId`、`ReticleWidgetClass`、`DroppedPickupActorClass`。父类拾取 Actor 的高置信度死接口和未使用 include 也已清理。用户完成 UE5.8.2 冷编译并重启后，7 个拾取蓝图与 7 个 ID 已全部编译保存，CDO 回读确认旧字段消失。
- 详细证据和验证结果见 `WeaponPickup_Audit_拾取表现清理.md`。7 个 StaticMesh、材质、拾取组件列表、EventGraph 和拾取 CDO 配置已通过 UE 编辑器资产接口回读；7 个 `ID_*` 的 EquipmentDefinition 均只生成 1 个 `B_*` Equipment Actor，且 `B_*` 继承 `B_Weapon` 的 SkeletalMeshComponent。单人、两个独立 PIE 客户端和 Listen Server 拾取链均已通过真实 `F` 输入验收。`WorldLabel` 没有源代码消费者，是唯一待产品决定是否删除的可选世界文字组件；本轮未擅自删除。

## 特殊武器投射物链清理与测试入口 (2026-08-26)

- 本轮只修改 `ShootGA_Weapon_Fire_Projectile` 及其 Rocket/Grenade 子类、`ShootProjectileBase`、`ShootProjectileRocket`、`ShootProjectileGrenade`；`UShootGameplayAbility_ReloadMagazine`、`UShootGameplayAbility_Weapon_Fire`、`UShootGA_Weapon_Fire_Rifle` 三个 Lyra 主线类未修改。
- 复核确认两类特殊武器的职责：Rocket 使用 `/Game/Weapons/Rocket_Launcher_A/Mesh/RocketLauncherA_Ammo`，1800cm/s、GravityScale=0、不可弹跳，碰撞后爆炸；Grenade 使用 `/Game/Weapons/Grenade_Launcher_A/Mesh/GrenadeLauncherA_Ammo`，1400cm/s、GravityScale=1、可弹跳、3秒引信，属于抛物线投射物。两者都由 `ProjectileWeaponConfig` 驱动，装备后的武器 SkeletalMesh 仍在独立 `B_*` Equipment Actor 上。
- 清理了 Rocket/Grenade 子类重复创建的 `AmmoCost`；实际消耗只由 `UShootGA_Weapon_Fire_Projectile` 的父类成本对象提供。删除了 Grenade 没有行为的 Bounce Delegate 注册和空回调，避免留下“配置了但不做事”的假链路。
- 修正投射物初始化顺序：先写入 `ProjectileWeaponConfig` 的速度、重力和弹跳配置，再调用 Rocket/Grenade 的 `ConfigureMovement` 约束，防止基类初始化覆盖 Rocket 的不可弹跳设置。修正 Grenade 命中判断：墙面/地面等不可伤害 Actor 继续弹跳，Pawn 或可伤害 Actor 才立即引爆，FuseTime 到期仍会引爆。
- `UShootGA_Weapon_Fire_Projectile::GetMuzzleLocation` 增加装备表现尚未完成复制时的 Avatar 位置兜底，避免 WeaponInstance 尚未解析枪口 Socket 时把投射物生成在世界原点。该兜底不改变正常 Equipment Actor 已就绪时的枪口位置。
- 网络边界按 GAS 规则保持为服务器生成并复制投射物：`LocalPredicted` 能力只在拥有者客户端和服务器执行，模拟代理不执行 GA；远端通过复制的投射物移动、服务器伤害和 GameplayCue 看到表现。本轮没有擅自引入客户端预测投射物和回滚链。
- 已在 `TestMap_ListenServer` 放置 `DungeonPickup_Rocket_Launcher_A`（350, 750, 70）和 `DungeonPickup_Grenade_Launcher_A`（350, 1000, 70）。已在 `TestMap_SplitScreen` 按现有 P1/P2 对称布局放置 Rocket（350/590, 750, 70）和 Grenade（350/590, 1000, 70），方便两个本地玩家分别调试。
- UE 5.8.2 编辑器关闭后，项目规定的 `Scripts/Build_Windows.ps1` 冷编译已成功；该条记录的是 2026-08-26 当时的验证边界。MCP 恢复后的 Rocket/Grenade 真实输入、弹跳/爆炸、伤害和远端 Cue 已在 2026-08-27 完成最终闭环。

## 特殊投射物准星对齐与实现精简 (2026-08-27)

- 玩家验收和 PIE 数据共同确认了侧飞/反飞根因：GA 已把 Actor +X 旋转到“枪口 -> 准星目标点”，同时传入世界空间速度；`UProjectileMovementComponent` 的默认 `bInitialVelocityInLocalSpace=true` 又把速度乘了一次 Actor 旋转。`AShootProjectileBase` 现固定使用世界空间初速度，Actor Forward、Velocity 与实际位移保持同向。
- `UShootGA_Weapon_Fire_Projectile` 继续复用 `UShootGameplayAbility_Weapon_Fire` 的 LocalPredicted TargetData、Commit、散布和后坐力链。实体投射物只保留所属客户端的准星目标点；服务器校验目标距离和控制旋转夹角后，从服务器枪口生成唯一权威 Actor。`UShootGA_Weapon_Fire_Rifle`、Pistol、Shotgun 仍走父类默认的服务器重 Trace，行为未改变。
- 榴弹的固定速度抛体方向改由 UE `UGameplayStatics::SuggestProjectileVelocity` 求解；后续重力、扫掠、弹跳和旋转仍全部由 `UProjectileMovementComponent` 驱动。手写抛体公式已删除；目标不可达时维持枪口到准星的初始方向并自然下坠，不伪造命中。
- Grenade 子类不再重复覆盖基类已经写入的重力和 Bounce 开关。`Bounciness`、`BounceFriction`、`BounceStopSpeed` 已进入 `FProjectileWeaponConfig`，当前 ID 配置值为 `0.4 / 0.2 / 50`。排查期间增加但两把资产均为零的 Mesh/Trail 旋转偏移已删除，避免误导为资产轴问题。
- 当前 ID 配置为 Rocket `1800cm/s, GravityScale=0, FuseTime=0`；Grenade `1400cm/s, GravityScale=1, FuseTime=3`。Rocket FuseTime=0 仍表示碰撞引爆，不增加定时销毁。
- PIE 连续采样：Rocket 速度约 `(1798.68,-65.87,20.62)`，位置 X `678.25 -> 1000.91`；Grenade Z 速度 `332.30 -> -51.37`，保持抛物线。精简前还验证 Rocket 命中后 Actor 销毁，目标 Health `276 -> 238.12`。UE 5.8.2 Editor Target 冷编译成功。
- Niagara-first 仍是长期方向，但当前 Rocket/Grenade 的前后喷口、枪口和尾迹没有经过一对一等价验收的 Niagara 资产，因此继续保留动画 Notify 驱动的专用 Cascade，不把 Rifle/Pistol/Shotgun 的通用 Niagara Actor 生搬到特殊武器。详细边界见 `CascadeNiagaraAudit_审计报告.md`。

## 特殊投射物 P0 联机闭环 (2026-08-27)

- Rocket/Grenade 已完成 Listen Server 最终双端回归：远端拥有客户端通过真实输入开火，服务器保持唯一权威投射物；Host、拥有客户端和模拟代理观察到同一投射物、尾迹与爆炸，没有重复生成或重复播放。
- Rocket/Grenade 对目标 ASC 的伤害继续只由服务器结算，客户端 TargetData 只提供经距离和角度校验的准星目标点，不授予命中或伤害权威。
- Rifle、Pistol、Shotgun 的通用 Shell Eject、Muzzle Flash、Tracer、Fire GameplayCue 和弹药消耗已完成回归，特殊投射物分支没有改变三把 Lyra 主线武器。
- 本节是当前状态。上文和关联文档中标注为“待下一轮 PIE”的句子保留为当时的历史验证边界，不再代表当前 Rocket/Grenade 状态；Sniper 与 Shotgun A 的独立未完成项不受本结论影响。

## Sniper ADS 与枪口表现收尾 (2026-08-28)

- ADS 的按住进入、松开退出、快速点击和相机原始端点恢复已由玩家验收；普通武器使用 FOV 70 / SpringArm 150，Sniper 使用 FOV 30 / SpringArm 75，本轮没有用扩大 FOV 掩盖枪口表现问题。
- `B_Weapon.Enable Tracer FX=false` 只关闭通用 `B_WeaponFire` Niagara Tracer。持续亮光柱的真实来源是 Sniper Fire Montage 自带的 `P_SniperRifle_Tracer_01` Cascade Notify；底层 Sequence 与 Montage 还重复播放声音和两套枪口火焰。
- 当前 `AM_Sniper_Rifle_A_Fire_W` 不再保存直接 Notify；`Fire_SniperRifle_W` 只保留一次 Fire Sound 和一次 `P_SniperRifle_MuzzleFlash_01`。`B_Weapon` 新增默认开启的 `Enable Muzzle Flash FX` 并传给 `B_WeaponFire`，Sniper 子蓝图关闭该通用 Niagara Muzzle 开关，其他四把命中扫描武器继续开启。
- 新增 `UShootAnimNotify_PlayScopedMuzzleFlash`。它继续生成 Sniper 专用 Cascade，但在武器 Owner Pawn 持有 `Event.Movement.ADS` 时对生成组件设置 `Owner No See`，清理开镜玩家镜内烟雾，同时保留另一名本地分屏玩家、远端玩家和模拟代理的枪口火焰。
- Windows Editor 冷编译成功，输出基础 `UnrealEditor-NewWorldOrder.dll`；重启后新 Notify 类可反射，Sequence 引用、Montage 零直接 Notify、Sniper 两个通用 FX 开关、父子蓝图编译状态和图表接线均已复读通过。玩家随后确认开镜拥有者不再看到烟雾，其他玩家仍能看到枪口火焰，贴在枪口前的持续光柱也已消失；该视觉问题已关闭。

## 特殊武器独立 Linked Layer 入口 (2026-08-28)

- Lyra 与项目当前均通过 WeaponInstance 的 `EquippedAnimSet/UnequippedAnimSet` 选择角色 Linked Layer，没有枪型枚举。旧 `EShootWeaponAnimationStyle` 与 `GetAnimationStyle()` 已不在主线，不恢复 WeaponId 分支。
- Shotgun_A、Sniper_Rifle_A、Grenade_Launcher_A、Rocket_Launcher_A 已各新增男女独立 Linked Layer，并回写对应 `B_WeaponInstance_*`。Shotgun_A 从 Shotgun 姿势复制，另外三把从 Rifle 姿势复制；当前视觉保持不变。
- 新资产是后续专用待机、移动、瞄准和持枪姿势的隔离入口；各武器网格 AnimBP 及 `ID_*` 中按骨架配置的 Fire/Reload Montage 仍是独立链，换弹或开火动画不需要通过 C++ 枚举选择。
- 8 个新 Layer 与 4 个 WeaponInstance 均已由编辑器复读为 `BS_UP_TO_DATE`，女性规则继续使用 `Cosmetic.AnimationStyle.Feminine`。每个 Layer 写有 `NewWorldOrder.AnimationLayerIntent` 元数据，说明当前是无差异占位、后续应在本资产替换专用动画。
