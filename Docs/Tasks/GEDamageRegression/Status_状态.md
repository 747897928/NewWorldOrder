# GE 伤害系统回归 Lyra - 状态

## 当前阶段

- 阶段：首期三把枪 GE 数值链、通用 Cue、Listen Server 真实客户端射击，以及四类特殊武器的 GE/投射物迁移已完成代码与资产闭环，进入特殊武器网络/真实输入回归。
- 实施状态：2026-08-25 完成 Sniper、Grenade Launcher、Rocket Launcher 的每枪 GE 与投射物 Execution 接入，并完成 `/Game/Weapons` 全部 `ID_*` Fragment 字段审计；当前编辑器尚未重载本轮新增 C++，因此特殊武器 PIE 不提前宣称完成。
- 代码与资产：伤害数字沿用既有服务器实际掉血闭环；Fire GA 只装配命中上下文，数值公式已移出 Rifle/Pistol/Shotgun GA。
- 产出：本任务包内调查文档、Implementation_迁移方案.md 与 GameplayCueAndDamageNumbers_完整性复核.md。

## 已完成调查

1. 已读 NewWorldOrder AGENTS.md。
2. 已读当前项目武器开火 C++ 主线：
   - Source/NewWorldOrder/Public/AbilitySystem/Abilities/ShootGameplayAbility_Weapon_Fire.h
   - Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGameplayAbility_Weapon_Fire.cpp
   - ShootGA_Weapon_Fire_Rifle / Pistol / Shotgun / Sniper 头文件与 cpp
   - ShootGA_Weapon_Fire_Projectile 与 ShootProjectileBase 概览
3. 已读当前项目武器配置主线：
   - ShootInventoryFragment_RangedWeaponConfig.h
   - ShootRangedWeaponInstance.h / cpp
   - ShootInventoryItemDefinition、EquipmentDefinition、AbilitySet 头文件
4. 已读当前项目 GE 与属性主线：
   - ShootEffect_DamageSetByCaller.h / cpp
   - ShootAttributeSet.h / cpp 的 IncomingDamage 处理
   - ShootGameplayAbility 基类与费用流程
   - ShootGameplayCueNotify_WeaponFire.h / cpp
5. 已读 Lyra 本机源码主线：
   - LyraGameplayAbility_RangedWeapon.h / cpp
   - LyraRangedWeaponInstance.h / cpp
   - LyraAbilitySourceInterface.h
   - LyraDamageExecution.h / cpp
   - LyraCombatSet.h / cpp
   - LyraHealthSet.h
   - LyraGameplayAbility.h / cpp 的 MakeEffectContext 与 ApplyAbilityTagsToGameplayEffectSpec
   - LyraGameplayEffectContext.h / cpp
   - PhysicalMaterialWithTags.h / cpp
   - LyraGameplayAbility_FromEquipment.h / cpp
6. 已通过 NewWorldOrder 编辑器 MCP 查询资产：
   - /Game/Weapons 下 7 个 `ID_*`、7 个 `B_WeaponInstance_*`、各 AbilitySet、Equipment、Damage GE 与 GameplayCue。
   - /Game/Blueprints/GameplayCues/Weapons 下三个 GCN。
   - /Game/Blueprints/Character/PhysMat_Player 与 PhysMat_Player_WeakSpot。
   - 全 /Game 资产名检索，确认特殊武器的 `GE_Damage_*`、AbilitySet、投射物与 Cue 均已建立；Fire GA 仍由项目 C++ 类承载。
7. 已完成 Source 全量字段引用计数，区分活跃字段与仅声明字段。
8. 已通过 Lyra 11000 MCP 验证：
   - GA_Weapon_Fire 的 GE_Damage 变量与三个子蓝图指向。
   - GE_Damage_Pistol / GE_Damage_RifleAuto / GE_Damage_Shotgun 的 DurationPolicy、Execution、Scoped Modifier 与 Tags。
   - WeaponInstance 的 MaterialDamageMultiplier 弱点倍率。
   - PhysMat_Player 与 PhysMat_Player_WeakSpot 的 Tag。

## 迁移前已确认基线

- 当前三把枪没有每枪独立 Damage GE。
- Rifle、Pistol、Shotgun 的 Fire GA 默认 DamageGameplayEffectClass 都是 UShootEffect_DamageSetByCaller。
- Sniper 的 DamageGameplayEffectClass 为 null，当前若启用 Sniper 会直接跳过伤害。
- 伤害公式仍由 GA 的 ApplyDamageToTarget 在 C++ 里计算，属于半硬编码。
- HeadshotMultiplier、FalloffStart、FalloffEnd、FireInterval、BurstCount、bAutomaticFire、AttachSocketHand、AttachSocketHolster 在 C++ 中只有声明，没有读取。
- 三把枪当前 DistanceFalloff 曲线为空、MaterialDamageMultiplier 为空，距离衰减和弱点倍率不生效。
- WeakSpot 只在目标同时有 Status.Marked 时获得硬编码 10% 加成，并只用于伤害数字暴击样式。
- PhysMat_Player 无 Gameplay Tag；PhysMat_Player_WeakSpot 有 Gameplay.Zone.WeakSpot。
- 项目没有 Lyra 式 FLyraGameplayEffectContext，GA 手动 AddSourceObject 和 AddHitResult。
- 当前 AbilitySet 的 GrantedGameplayEffects 为空，STATUS 文档中“由 AbilitySet 配置伤害 GE”的说法与资产事实不一致。

## 已完成实施

1. 新增 `UShootEffect_DamageBase` 与 `UShootDamageExecution`；Execution 输出到 `UShootAttributeSet::IncomingDamage`。
2. `UShootRangedWeaponInstance` 持有每枪 Damage GE、距离曲线与物理材质倍率；已删除 Ranged Fragment 的伤害/旧衰减/旧散布回退字段，不再形成第二套伤害来源。
3. 创建并配置首发三把：
   - Pistol：BaseDamage 18，WeakSpot 2.0，20 m 后阶跃为 0.5。
   - Rifle：BaseDamage 12，WeakSpot 1.5，28 m 后阶跃为 0.5。
   - Shotgun：每弹丸 BaseDamage 12，WeakSpot 1.75，6.4 m 后 0.7、20 m 后 0.5。
4. Rifle/Pistol/Shotgun Fire GA 已统一调用父类 `ApplyWeaponDamageToTarget`；删除重复手算、SetByCaller 和 `Status.Marked +10%` 分支。
5. 冷编译成功；首发三把与三类特殊武器的 GE/WeaponInstance 蓝图配置已由 MCP CDO 复读；Game Target 二次编译也已通过。
6. PIE 可启动，未出现本次链路的 `Damage skipped`、GE 类加载错误或 Blueprint Runtime Error。
7. 三把枪已按 Lyra 真实 CDO 配置共用 `GameplayCue.Weapon.Rifle.Impact`；新增通用 `GCN_Weapon_Impact`，按 Character 与 Default/Concrete/Glass 播放项目现有命中 SoundCue，且不重复 `B_WeaponImpacts` 的 Niagara/Decal。
8. Impact Cue 改动冷编译成功；重启后 CDO、GCN Tag、SoundCue 条件与蓝图编译状态复读通过，PIE 启停无本链 GameplayCue 错误。
9. 已通过真实 Fire GA、TargetData、HitResult 与 GE Execution 完成首轮数值验收：
   - Rifle 近距身体 12，30 m 身体 6。
   - Pistol 近距身体 18，25 m 身体 9。
   - Shotgun 近距一次射击共扣 87；这是多个弹丸命中的合计值，不把它误记成单弹丸伤害。
10. 修复 `AEnemyBotCharacter` 构造函数错误覆盖父类碰撞配置的问题：敌人 Capsule 不再阻挡主 Weapon Trace。`AShootCharacterBase` 的 `LyraPawnCapsule` / `LyraPawnMesh` 分工恢复后，主射线会穿过 Capsule 并命中 Mesh 的 PhysicsAsset；粗略胶囊查询继续使用独立的 `Weapon_Capsule` 通道。
11. 弱点闭环已用运行时证据验收：
   - 身体射线命中 `CharacterMesh0`，PhysicalMaterial 为 `PhysMat_Player`，真实 Rifle GA 扣血 12。
   - 头部射线命中 `CharacterMesh0` 的 `head`，PhysicalMaterial 为 `PhysMat_Player_WeakSpot`，真实 Rifle GA 扣血 18，等于 12 × 1.5。
   - 真实 Pistol GA 头部扣血 36，等于 18 × 2.0。
   - 修复前相同射线被 `CollisionCylinder` 抢占，BoneName 为 None、PhysicalMaterial 为 DefaultPhysicalMaterial，因此弱点倍率必然不可达。
12. TestMap_SplitScreen 的死亡与敌人重生链已做运行时验收：真实 Pistol 弱点射击把 Health 从正数降到 0 后，旧 `BP_EnemyBotCharacter_C_0` 按 1 秒尸体寿命清理；启用 `bRespawnEnemyBots` 的 `BP_TestGameMode` 在约 5 秒后生成 `BP_EnemyBotCharacter_C_1`，新敌人 Health 为 276 且拥有新的 `BP_EnemyBotController_C_1`。因此现状不是“没有死亡/重生代码”，后续 GameFrameworkMigration 应补规则和表现，不应重写这条已工作的服务器链。
13. TestMap_ListenServer 的双世界与复制前置条件已通过：Listen Server 上成功把 Rifle 交给远端客户端对应的服务器 Pawn，客户端 QuickBar 收到 30/60 弹药与激活槽复制；服务端和客户端均看到 Health 276 的同一敌人，客户端本地 Weapon 射线命中 `CharacterMesh0`、`spine_05` 与 `PhysMat_Player`。这证明远端拾枪、武器状态、目标角色、碰撞与物理材质复制链可用。
14. Listen Server 的远端真实输入开火已验收：通过 Slate 给 Client 1 的 PIE 视口聚焦，客户端按真实 `F` 走 Enhanced Input / InputTag / ASC 拾取 Rifle，服务端与客户端同时得到 30/60；随后真实左键开火，两端弹匣同步 `30 → 29 → 28`。第二枪命中服务器敌人弱点后，两端 Health 同步 `276 → 258`，与 Rifle 近距 `12 × 1.5 = 18` 一致。Host 没有获得远端客户端的武器实例。
15. 编辑器 Python 在客户端世界直接调用 `AbilitySystemComponent.try_activate_ability` 会绕开项目的 Enhanced Input → InputTag → ASC 输入入口，并触发 1600 层 `ServerTryActivateAbility` 预测键递归；该调用已永久排除出网络验收方案。此现象只说明自动化入口无效，不能冒充正常输入缺陷，也不能据此修改玩法代码。
16. 三把枪 Fire Cue 的本地 CameraShake / ForceFeedback 最小闭包已完成：
   - `UShootGameplayCueNotify_WeaponFire` 改为继承 `UGameplayCueNotify_Burst`，父类只负责蓝图配置的本地反馈；项目子类继续负责 `UShootWeaponInstance -> B_Weapon.Fire` 适配，不复制 Lyra 的大型音频图。
   - Pistol 使用 `CS_Weapon_Fire_Pistol + FFE_Weapon_Fire`；Rifle 使用 `CS_Weapon_Fire_Rifle + FFE_Weapon_Fire_Auto`；Shotgun 使用 `CS_Weapon_Fire_Shotgun + FFE_Weapon_Fire`。SpawnCondition 保持 Lyra 的 `InstigatorActor` 本地控制者条件，避免分屏串线。
   - 依赖闭包审计确认完整迁移 Pistol/Rifle/Shotgun Fire GCN 分别会引入 370/408/363 个包，Character DamageTaken 为 259 个包，通用 Impact 为 81 个包，因此没有盲目复制整套 ShooterCore 音频依赖。
   - 本轮只迁入 8 个零递归 `/Game` 依赖的反馈资产：四个 Weapon Fire CameraShake、两个 Weapon Fire ForceFeedback，以及 Character DamageTaken 的 CameraShake/ForceFeedback。
   - C++ UCLASS 父类变更已冷编译成功；三个 Fire GCN 编译为 UpToDate，CDO 复读引用正确。TestMap_SplitScreen 中真实 `F` 拾取 Rifle 后真实左键射击，弹匣 `30 -> 29`，且没有新增 GameplayCue 或 Blueprint Runtime Error；结束后已恢复 HomeMap、Standalone、单客户端、单进程。
17. `GameplayCue.Character.DamageTaken` 的项目化最小闭包已完成：
   - `UShootAttributeSet` 只在服务器最终扣除 Health 且 `ActualHealthDamage > 0` 后执行 Cue；友伤归零、免疫和只命中护盾不会误播，过量伤害使用实际 Health 损失。
   - 新增 `/Game/Blueprints/GameplayCueNotifies/GCN_Character_DamageTaken`，只配置 Lyra 的 `CS_Character_DamageTaken + FFE_Character_Damage`，并沿用 `InstigatorActor` 本地控制者条件；不复制 Lyra 259 包的完整粒子、音频、Lens、Montage 与消息依赖。
   - 现有 `SendDamageNumberFeedback -> ClientAddDamageNumber -> Niagara` 仍是唯一 NumberPop 链，GCN 不生成第二份数字。
   - 新 UCLASS/GameplayTag 相关代码冷编译成功；GCN 编译与序列化 CDO 复读通过。TestMap_SplitScreen 中 P0 真实 `F` 拾取 Rifle 后真实左键命中，弹匣下降且敌人 Health `276 -> 264`；日志无 GameplayCue 查找失败、Blueprint Runtime Error 或 Accessed None。结束后恢复 HomeMap、Standalone、单客户端、单进程。
18. Shotgun 弱点的逐 pellet GE 证据已补齐：运行时 `B_WeaponInstance_Shotgun` 复读 `BulletsPerCartridge = 9`，P0 真实 `F` 拾取并以真实左键近距命中头部后，敌人 Health `276 -> 87`，实际损失 189，严格等于 `9 × BaseDamage 12 × WeakSpot 1.75`。这证明 9 个 TargetData 各自携带 WeakSpot PhysicalMaterial 并独立进入 GE Execution，不再以总伤害反推不确定的单弹丸结果。
19. C++ GameplayEffect 的 SetByCaller 初始化顺序缺陷已修复并回归：
   - 冷启动前复读确认七个 C++ GE 的 `DataName` 与 `DataTag` 均为空；原因是 GE CDO 构造阶段早于原生 GameplayTag 初始化，`RequestGameplayTag(..., false)` 将无效 Tag 永久写进 Modifier。
   - 七个 C++ GE 改为保存稳定的文本 `DataName`；所有 C++ Spec 写入统一走 `FShootGameplayTags::SetSetByCallerMagnitude`，同时写 FName 与 GameplayTag 通道，兼容 C++ GE、蓝图 GE 和现有 ActiveEffect 快照。
   - 冷编译、编辑器重启后复读通过：Armor、Medical、MoveSpeed、ReloadSpeed、ShieldWall、Heal、SnapshotRestore 的全部 Modifier 都持有正确 `DataName`，既有 Damage GE 保持 `SetByCaller.Damage`。
   - TestMap_SplitScreen 中男性 `ShieldCapacityBonus=0.2`、女性 `HealingDoneMultiplier=1.2` / `ReloadSpeedMultiplier=1.05`；对应被动 GE 各只有一份，日志无 `Data None`。
   - `ApplyGenderAbilityKit` 已改为幂等重建：连续再次调用两次后属性与 ActiveEffect 数量不变，避免读档恢复/初始化重入造成永久 GE 翻倍。
20. 分屏真实输入补齐伤害数字的数据入口证据：P0 Rifle 弹匣 `30 -> 29`，敌人 Health `276 -> 258`，实际损失 18；命中后生成唯一临时 NiagaraActor，System 为 `/Game/Effects/Particles/Impacts/NS_DamageNumbers`。这证明 NumberPop 使用最终实际 Health 损失而不是旧固定 10。MCP 当前无法读取 `APlayerController::HiddenActors`，瞬时数字也未稳定截入自动截图，因此 P0 可见而 P1/Host 不可见的逐帧视觉隔离仍明确保留为人工验收，不把代码结构冒充视觉结论。

## 未完成与未决

1. Lyra 数值来源已补充复核：三个 Damage GE 的顶层 Modifiers 为空，但各自 Execution 内有一个 Scoped Modifier，把捕获的 CombatSet.BaseDamage 覆盖为 Pistol 18、Rifle 12、Shotgun 12。项目用 `UShootEffect_DamageBase::BaseDamage` 表达同一份每枪数据，不为这一项单独复制 CombatSet 生命周期。
2. Lyra DistanceDamageFalloff Key 已复读：Pistol 20 m 后 50%，Rifle 28 m 后 50%，Shotgun 6.4 m 后 70%、20 m 后 50%。详见 Investigation_Lyra伤害链.md 的实际 Key 表；Pistol/Rifle 的 1 cm 阶跃必须保留 Constant 插值。
3. 真实射击矩阵已覆盖 Rifle/Pistol 近远距离、Shotgun 近距多弹丸合计、Rifle/Pistol 弱点、Shotgun 九 pellet 全弱点，以及 Listen Server 远端客户端真实输入拾枪、开火、服务器权威扣血和双端复制。
4. `/Game/Weapons` 下全部 7 个 `ID_*` 已完成 Fragment 审计：旧 `BaseDamage`、Headshot、Falloff、Material、FireInterval、Burst、Automatic、Attach 和固定散布字段均无存量命中；投射物 Fragment 只保留运动、爆炸半径/衰减、Cue、拖尾和网格。
5. Sniper 已切换到 `GE_Damage_Sniper_Rifle_A`；GL/Rocket 已切换到各自 GE + 服务器投射物爆炸 Execution。当前缺的是特殊武器的 Listen Server/真实输入 PIE 证据，不是兼容伤害回退。
6. 武器 Impact Cue 首期闭环与 Fire Cue 的 CameraShake / ForceFeedback 最小闭包已完成；设备反馈、完整音频参数链和 Lyra 的大型声音依赖仍未迁入。详见 GameplayCueAndDamageNumbers_完整性复核.md。
7. `GameplayCue.Character.DamageTaken` 已完成 CameraShake / ForceFeedback 最小闭包；Lyra 的受击粒子、DamageTaken/Dealt 音频、Lens、命中 Montage 与 GameplayMessage 尚未迁入，且后续扩展仍禁止新增第二套 NumberPop。
8. 本次 PIE 同时暴露的 C++ GE SetByCaller `Data None` 已修复并完成分屏回归；既存 MF AimOffset 失效样本引用仍属于独立动画问题，不混入本任务。

## 下一步顺序

1. Niagara 使用实际 Health 损失的数据入口已由真实射击与 NiagaraActor 生成证据关闭；只属于伤害来源 LocalPlayer 的逐帧视觉隔离仍需高速录屏或人工同帧观察。
2. Manny BlendSpace 无效样本继续作为独立动画问题记录；C++ GE SetByCaller 初始化缺陷已完成，不再列为待办。
3. 完整阅读 GameFrameworkMigration 任务包后，再审计 Health 归零到死亡、淘汰通知和重生的现有调用链，决定最小补齐范围；不要重写已工作的 `PlayerDied` / `RespawnEnemyBot`。
4. 在用户允许的当前编辑器安全窗口完成特殊武器的单人、分屏和 Listen Server 真实输入回归，并记录投射物生成、爆炸 Cue、弹药复制和 GE 实际掉血；不要把手持手雷能力混入发射器结论。
