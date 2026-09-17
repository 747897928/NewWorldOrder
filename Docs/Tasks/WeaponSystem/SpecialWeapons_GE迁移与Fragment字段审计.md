# 特殊武器 GE 迁移与 Fragment 字段审计

审计日期：2026-08-25

范围：`/Game/Weapons` 全部 `ID_*`、`B_WeaponInstance_*`、四类特殊武器的 Fire/Reload/Projectile/GameplayCue 链，以及伤害 C++ 调用链。

## 结论

- 伤害基础值只存在于每把武器的 Damage GE；`UShootDamageExecution` 统一输出 `IncomingDamage`。
- `UShootRangedWeaponInstance` 只持有 Damage GE 引用、命中距离衰减曲线和物理材质倍率；它不再从 Fragment 回退读取 BaseDamage 或旧伤害字段。
- Ranged Fragment 只保留射程、弹药容量、每次消耗、动画映射、弹道采样、每枪子弹数、本地相机后坐力和枪口 Socket。
- Projectile Fragment 只保留投射物类与 `FProjectileWeaponConfig`：运动、引信、爆炸半径/衰减、爆炸 Cue、拖尾和网格。`ProjectileDamageEffect` 重复字段和未使用的 `ExplosionFX` 已删除。
- 四类特殊武器均可走 GE：Shotgun A 逐发装填不产生伤害；Sniper 走 hitscan + Damage GE；Grenade Launcher/Rocket Launcher 走服务器投射物 + radial Damage GE。手持手雷保留独立 `ShootGA_ThrowGrenade`，不冒充榴弹发射器。

## Fragment 字段归属

### RangedWeaponConfig：保留字段

| 字段 | 作用 | 不能删除的原因 |
| --- | --- | --- |
| `MaxRange` | hitscan Trace 最大距离 | `UShootRangedWeaponInstance::GetMaxDamageRange` 和服务器 Trace 使用 |
| `MagazineSize`、`MaxReserve` | 弹匣/备弹上限 | ItemInstance 初始弹药、Reload 和补给站使用 |
| `AmmoPerShot` | 每次开火的弹药成本 | `UShootAbilityCost_AmmoTagStack` 从 WeaponInstance 读取 |
| `CharacterMontages` | 按目标 Skeleton 选择 Fire/Reload/Equip/Unequip | Fire/Reload GA 和 `AN_PlayWeaponMontage` 的正式入口 |
| `BulletTraceSweepRadius` | Trace 的球扫半径 | 影响命中几何，不是伤害数值 |
| `BulletsPerCartridge` | Shotgun 每次生成的 pellet 数 | Fire GA 生成多条 TargetData，随后每条各走 GE |
| `LocalCameraPitch/YawRecoil*` | 拥有者本地相机后坐力 | 不复制、不参与服务器 Trace/伤害；当前项目没有在 WeaponInstance 重复保存 |
| `MuzzleSocketName` | 武器 Actor 枪口位置 | Fire Cue、投射物生成和准星表现需要 |

### RangedWeaponConfig：已删除字段

以下字段在当前项目调用链中没有合法的数据归属，均已从 C++ Fragment 删除，并通过编辑器 CDO 对全部 ID 复读确认无存量字段命中：

- `BaseDamage`、Headshot/Marked 伤害倍率。
- `FalloffStart`、`FalloffEnd`、距离伤害曲线、Fragment 材质伤害倍率。
- `FireInterval`、BurstCount、AutomaticFire。
- 固定 Spread/Heat/Recovery/FirstShot 字段以及 ADS/移动散布加成。
- 旧 Attach Socket 字段。

这些数据的正确归属是：基础伤害和可调弱点倍率放在每把 Damage GE；距离曲线和物理材质倍率放在 `B_WeaponInstance_*`；热量散布曲线和姿态倍率放在 `B_WeaponInstance_*`；开火节奏放在 GA/AbilitySet；弹药动态值放在 ItemInstance StatTagStack。

角色持枪动画层也不属于 Ranged Fragment。项目已经删除旧的 `EShootWeaponAnimationStyle` 与 `UShootRangedWeaponInstance::GetAnimationStyle()`；现行入口是 `UShootWeaponInstance` 继承自 `UShootEquipmentInstance` 后定义的 `EquippedAnimSet`、`UnequippedAnimSet`。该结构与 Lyra 的 `FLyraAnimLayerSelectionSet` 相同，由每把 `B_WeaponInstance_*` 直接配置 Layer，而不是通过 Rifle/Pistol/Shotgun 枚举分支选择。

### ProjectileWeaponConfig：保留字段

- `ProjectileClass`：服务器生成的投射物类型。
- `InitialSpeed`、`GravityScale`、`bShouldBounce`、`bRotationFollowsVelocity`：运动行为。
- `Bounciness`、`BounceFriction`、`BounceStopSpeed`：仅在允许弹跳时使用的蓝图调参；Grenade 子类不再硬编码这些数值。
- `bUseBallisticAim`、`bFavorHighArc`：是否让 UE 原生弹道解把固定初速对准真实阻挡点，以及选择低弧/高弧。
- `MaxServerAimErrorDegrees`：服务器接纳所属客户端准星目标点时的角度校验上限，不授予客户端命中或伤害权威。
- `FuseTime`：引信行为。
- `DamageInnerRadius`、`DamageOuterRadius`、`DamageFalloff`：只描述 radial GE Execution 的空间衰减，不保存伤害基础值。
- `bApplyMaterialMultipliers`：爆炸是否允许物理材质/弱点倍率。
- `ExplosionCueTag`：爆炸 GameplayCue 的数据驱动入口。
- `TrailSystem` 与 `TrailParticleSystem`：Niagara 与尚未完成等价验证的原始 Cascade 二选一，不能同时播放。
- `ProjectileMesh`：投射物表现网格。

## `/Game/Weapons` ID 审计结果

编辑器 AssetRegistry 递归扫描得到 7 个 `ID_*`，全部没有旧伤害/散布/射速字段：

| ID | Fragment 数量 | Ranged 配置 | Projectile |
| --- | ---: | --- | --- |
| `ID_Rifle` | 4 | 25000 / 30 / 60 / 1，Sweep 5.5，1 pellet | 否 |
| `ID_Pistol` | 4 | 25000 / 12 / 48 / 1，Sweep 6.0，1 pellet | 否 |
| `ID_Shotgun` | 4 | 25000 / 8 / 16 / 1，Sweep 0.5，9 pellets | 否 |
| `ID_Shotgun_A` | 4 | 25000 / 8 / 16 / 1，Sweep 0.5，9 pellets | 否 |
| `ID_Sniper_Rifle_A` | 4 | 50000 / 5 / 15 / 1，Sweep 2.5，1 pellet | 否 |
| `ID_Grenade_Launcher_A` | 5 | 25000 / 6 / 18 / 1，Sweep 0，1 pellet | 是 |
| `ID_Rocket_Launcher_A` | 5 | 50000 / 4 / 8 / 1，Sweep 0，1 pellet | 是 |

普通枪与 Shotgun A 的 4 个 Fragment 是 `EquippableItem + WeaponBasicConfig + RangedWeaponConfig + SetStats`；GL/Rocket 额外增加 `ProjectileWeaponConfig`。没有发现空 Fragment、伤害重复字段或不被调用的投射物伤害 GE 字段。

## 四类特殊武器调用链

### Shotgun A：逐发装填

- `UShootGA_Reload_ShotgunPerShell` 只接受 WeaponInstance 按 Skeleton 解析出的角色 Reload Montage，不再使用自己的 `ReloadMontage` 兜底。
- `Start -> InsertShell -> Loop -> End` 由角色 Montage 的 `UShootAnimNotify_InsertShell` 驱动；服务器每个事件最多调用一次 `ReloadAmmo(1)`。
- 非空弹匣允许 Fire GA 取消 Reload；空仓开始装填时加 `Ability.Weapon.NoFiring`。
- 已提交的 StatTag 弹药在取消后保留；未到 Notify 的弹不增加。
- 点击一次 R 后持续逐发装填；装满、备弹耗尽或收到主动结束事件时进入 `ShotgunEnd`。Fire 取消仍然直接切换开火，不强行播放收尾。
- 武器机械动作由武器 Montage 播放；角色 Montage 的 `UShootAnimNotify_PlayWeaponMontage` 先播武器，再调用 `MontageSync_Follow`。

### Sniper

- `ShootGA_Weapon_Fire_Sniper` 使用 WeaponInstance 的角色 Fire Montage、AmmoTagStack 和父类 `ApplyWeaponDamageToTarget`。
- 父类创建带 HitResult/SourceObject 的自定义 GE Context；Sniper GE `BaseDamage=80`、`MarkedWeakSpotMultiplier=2`。
- DistanceDamageFalloff 和 MaterialDamageMultiplier 来自 `B_WeaponInstance_Sniper_Rifle_A`；Fire Cue 使用 `GameplayCue.Weapon.Sniper.Fire`，Impact 沿用已有 `GameplayCue.Weapon.Rifle.Impact`。
- `ShootGA_Reload_Sniper` 接入 `ID_Sniper_Rifle_A` AbilitySet，输入为 R。

### Grenade Launcher

- `ShootGA_Weapon_Fire_GrenadeLauncher` 复用通用 Projectile Fire GA；服务器从 WeaponInstance 读取 Projectile Fragment 和 `GE_Damage_Grenade_Launcher_A` 生成 `AShootProjectileGrenade`。
- 配置为 1400cm/s、重力 1、可弹跳、3秒引信、200/500cm、线性衰减，爆炸 Cue 为 `GameplayCue.Weapon.GrenadeLauncher.Detonate`。
- `TrailSystem` 为空，原始 `P_Grenade_Trail_01` 作为唯一拖尾；火焰、声音、枪口 Cascade 保留在 `Fire_GrenadeLauncher_W` 同一动画闭包。
- 当前 Damage GE 基础值为 100，是纵切初始调参，不是最终产品平衡结论。

### Rocket Launcher

- `ShootGA_Weapon_Fire_RocketLauncher` 复用通用 Projectile Fire GA；服务器从 WeaponInstance 读取 Projectile Fragment 和 `GE_Damage_Rocket_Launcher_A` 生成 `AShootProjectileRocket`。
- 配置为 1800cm/s、无重力、碰撞即爆、300/600cm、无距离衰减，爆炸 Cue 为 `GameplayCue.Weapon.Rocket.Detonate`。
- `TrailSystem` 为空，原始 `P_RocketLauncher_Trail_01` 作为唯一拖尾；Fire Montage 的后喷口在 0.005850s 使用 `MuzzleFlashRear`，前喷口在 0.014262s 使用 `MuzzleFlash`，没有合并成通用枪口效果。
- 当前 Damage GE 基础值为 140，是纵切初始调参，不是最终产品平衡结论。

## 仍需明确的兼容边界

`UShootGA_Weapon_Fire_Rifle` 和 `UShootGameplayAbility_ReloadMagazine` 共享基类仍保留 `CharacterFireMontage`/`CharacterReloadMontage` 兜底属性。当前 7 个 ID 都配置了显式 Skeleton Montage，正式武器运行链不会命中这些字段；它们不承载伤害、弹药或投射物逻辑。

2026-08-28 的 BlueprintService 与 AssetRegistry 父类审计确认，三个正式 Reload 蓝图的兜底值均为 `None`；但遗留 `GA_AssaultWeapon_File`、`GA_AssaultWeapon_Reload` 仍分别保存 `AM_Fire_Rifle_Ironsights` 与 `AM_Reload_Rifle_Ironsights`。两者当前没有外部 Referencer，属于整套旧 Assault_Rifle_A 资产候删，而不是字段零消费者。项目源码已在共享类头部标注禁止未经确认修改，因此本轮保留字段与资产；只有先确认并处理遗留资产闭包后，才能重新讨论删除共享字段。

## 验证边界

- Windows Game Target 在本轮第二次编译通过；当前编辑器没有被关闭或重启，因此新增 C++ 尚未在该进程内热加载。
- MCP 已回读全部 ID Fragment、特殊 AbilitySet、WeaponInstance Damage GE、Damage GE 数值和投射物配置；GL/Rocket 的 Explosion Cue Tag 也已通过 `GameplayTag.export_text()` 确认。
- 2026-08-28 已删除错误的 `WaitInputRelease` 兼容：普通点击 R 不再在首个 InsertShell 前提前结束。Listen Server Host 自动化复验覆盖 `7/16 -> 8/15`、连续逐发提交及非空弹匣开火打断；远端拥有客户端复制仍是独立未完成边界。
- 本条记录的是 2026-08-25 的验证边界。Rocket/Grenade 的单人、分屏、Listen Server 真实输入、投射物实际掉血和远端 Cue 已在 2026-08-27 完成最终闭环；Sniper 仍是独立未完成项。

## 2026-08-26 投射物运行链复核与修正

本轮针对 Rocket/Grenade 的实际 C++ 调用链做了第二次审计。受保护的 `UShootGameplayAbility_ReloadMagazine`、`UShootGameplayAbility_Weapon_Fire`、`UShootGA_Weapon_Fire_Rifle` 未修改。

- `UShootGA_Weapon_Fire_Projectile` 保留唯一的 `UShootAbilityCost_AmmoTagStack`；Rocket/Grenade 子类不再重复创建同一弹药成本对象。
- 投射物基类现在先应用 `FProjectileWeaponConfig` 的速度、重力和弹跳值，再调用子类 `ConfigureMovement`。此前顺序可能让基类运动初始化覆盖 Rocket 的不可弹跳配置；Rocket 子类现在只负责强制 `bShouldBounce=false`，速度和重力继续由数据资产配置。
- Grenade 删除了空的 Bounce Delegate 和无行为回调。Grenade 命中不可伤害的墙面/地面时保留弹跳；命中 Pawn 或 `CanBeDamaged()` 的 Actor 时立即爆炸；3 秒 Fuse 仍作为最终兜底。这使 `bShouldBounce=true` 与 `FuseTime=3` 的资产配置真正有意义。
- 投射物生成位置优先使用装备 Actor 的 `MuzzleSocketName`；当装备 Actor 尚未完成复制、枪口位置暂时无效时，能力回退到 Avatar 位置，避免在世界原点生成一发不可见投射物。
- 网络策略没有引入未经设计的客户端预测投射物。GAS `LocalPredicted` 能力由拥有者客户端先执行、服务器执行权威结果，模拟代理不执行 GA；特殊投射物继续由服务器生成并复制，服务器负责爆炸、径向 GE 伤害和 Explosion GameplayCue，远端通过复制状态与 Cue 获得表现。

## 本轮地图调试入口

- `TestMap_ListenServer`：`DungeonPickup_Rocket_Launcher_A` 位于 `(350, 750, 70)`；`DungeonPickup_Grenade_Launcher_A` 位于 `(350, 1000, 70)`。
- `TestMap_SplitScreen`：P1/P2 各有一个 Rocket 和一个 Grenade，Rocket 位于 `(350, 750, 70)`、`(590, 750, 70)`，Grenade 位于 `(350, 1000, 70)`、`(590, 1000, 70)`。
- 2026-08-26 已通过 UE 编辑器 API 保存两个地图，并在编辑器关闭后用 UE 5.8.2 冷编译验证 C++ 改动通过。该句记录的是当时 MCP 尚未恢复的历史边界；后续双端运行时验收见本文末尾的 P0 联机闭环。

## 2026-08-27 准星对齐、网络边界与精简结论

- 根因不是 Muzzle Socket、网格轴或手写重力，而是世界空间速度被 `UProjectileMovementComponent` 按本地空间再次旋转。`AShootProjectileBase` 现在显式设置 `bInitialVelocityInLocalSpace=false`；Spawn Rotation、Actor +X、Velocity 和实际位移使用同一“枪口 -> 准星目标点”方向。
- 为处理第三人称近距离视差，Projectile GA 复用父类 TargetData 复制链，但通过默认返回 false 的虚函数只为实体投射物保留所属客户端准星点。服务器校验 `MaxRange` 和 `MaxServerAimErrorDegrees` 后才生成 Actor；碰撞、爆炸、伤害和 Cue 仍全是服务器权威。受保护的 Reload 与 Rifle 子类未修改；父 Fire GA 只增加默认不改变 hitscan 行为的扩展点。
- `ResolveFireDirectionFromTargetData` 只保留四步：取第一条 HitResult、确定 AimPoint、执行服务器校验、从 Muzzle 求初速度方向。Grenade 的固定速度弹道改用 UE `SuggestProjectileVelocity`，不再维护项目内抛体公式。
- Bounce 的 `0.4 / 0.2 / 50` 已从 `AShootProjectileGrenade::ConfigureMovement` 移到 `FProjectileWeaponConfig`；Grenade 子类只保留“命中 Pawn/可伤害目标立即爆炸、世界几何弹跳、Fuse 到期爆炸”的语义差异。
- 当前配置与验收基线：Rocket `1800cm/s, GravityScale=0, FuseTime=0`；Grenade `1400cm/s, GravityScale=1, FuseTime=3`。玩家已确认两把武器朝准星飞行正常；MCP 连续采样再次确认 Rocket Forward/Velocity/位移同向，Grenade Z 速度由正转负。

## 2026-08-27 P0 联机闭环

- Rocket/Grenade 已完成 Listen Server 最终双端回归：远端拥有客户端通过真实输入开火，服务器生成唯一权威投射物；Host、拥有客户端与模拟代理观察到同一投射物、尾迹和爆炸，没有重复生成或重复播放。
- Rocket/Grenade 命中目标后的 ASC 属性变化由服务器权威伤害链产生；客户端准星 TargetData 仍只参与服务器校验后的发射方向解析，不直接决定命中或伤害。
- Rifle、Pistol、Shotgun 的通用 Niagara Shell Eject、Muzzle Flash、Tracer、Fire GameplayCue 与弹药消耗已完成回归，特殊投射物分支没有改变三把 Lyra 主线武器。
- P0 现已关闭。Sniper 的完整输入与 Listen Server 验收、Shotgun A 的逐发装填联机同步继续作为独立任务，不因本节自动标记完成。
