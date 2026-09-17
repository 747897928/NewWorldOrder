# GE 伤害回归 - 字段引用与资产事实

## 调查方法

- 源码阅读：PowerShell Get-Content 与 Select-String。
- 资产读取：本机 NewWorldOrder 编辑器 MCP 8000 的 execute_python_code，通过 AssetRegistry 与 get_editor_property 读取。
- Lyra 资产：只读取文件名、目录结构与 C++ 源码；未直接读取 Lyra .uasset 字节。
- 外部佐证：Epic 官方 Abilities in Lyra 文档、X157 Lyra Health and Damage 笔记。

## MCP 资产事实

### 三把枪 AbilitySet

| AbilitySet | 开火 Ability | 换弹 Ability | GrantedGameplayEffects |
|---|---|---|---|
| /Game/Weapons/Rifle/AS_Weapon_Rifle | /Script/NewWorldOrder.ShootGA_Weapon_Fire_Rifle | /Game/Weapons/Rifle/GA_Weapon_Reload_Rifle_C | 空 |
| /Game/Weapons/Pistol/AS_Weapon_Pistol | /Script/NewWorldOrder.ShootGA_Weapon_Fire_Pistol | /Game/Weapons/Pistol/GA_Weapon_Reload_Pistol_C | 空 |
| /Game/Weapons/Shotgun/AS_Weapon_Shotgun | /Script/NewWorldOrder.ShootGA_Weapon_Fire_Shotgun | /Game/Weapons/Shotgun/GA_Weapon_Reload_Shotgun_C | 空 |

### 三把枪 EquipmentDefinition

| 装备 | InstanceType | AbilitySetsToGrant | ActorsToSpawn |
|---|---|---|---|
| BP_Equipment_Rifle | B_WeaponInstance_Rifle_C | AS_Weapon_Rifle | B_Rifle_C，Socket weapon_r |
| BP_Equipment_Pistol | B_WeaponInstance_Pistol_C | AS_Weapon_Pistol | B_Pistol_C，Socket weapon_r |
| BP_Equipment_Shotgun | B_WeaponInstance_Shotgun_C | AS_Weapon_Shotgun | B_Shotgun_C，Socket weapon_r |

### 三把枪 ItemDefinition Fragments

公共结构：

- EquippableItem
- WeaponBasicConfig
- RangedWeaponConfig
- SetStats

弹药统计：

| 武器 | Magazine | Reserve |
|---|---|---|
| Rifle | 30 | 60 |
| Pistol | 12 | 48 |
| Shotgun | 8 | 16 |

RangedWeaponConfig 关键值：

| 字段 | Rifle | Pistol | Shotgun |
|---|---|---|---|
| BaseDamage | 10.0 | 10.0 | 10.0 |
| HeadshotMultiplier | 1.0 | 1.0 | 1.0 |
| MaxRange | 25000 | 25000 | 25000 |
| FalloffStart | 12500 | 12500 | 12500 |
| FalloffEnd | 25000 | 25000 | 25000 |
| DistanceFalloff | 无 Key | 无 Key | 无 Key |
| MaterialDamageMultiplier | 空 | 空 | 空 |
| MagazineSize | 30 | 12 | 8 |
| MaxReserve | 60 | 48 | 16 |
| AmmoPerShot | 1 | 1 | 1 |
| BulletsPerCartridge | 1 | 1 | 9 |
| BulletTraceSweepRadius | 5.5 | 6.0 | 0.5 |
| SpreadExponent | 0.8 | 1.0 | 1.0 |
| AllowFirstShotAccuracy | true | true | false |
| SpreadRecoveryCooldownDelay | 0.15 | 0.2 | 0.5 |

WeaponInstance 蓝图关键值：

| 字段 | Rifle | Pistol | Shotgun |
|---|---|---|---|
| SpreadExponent | 0.8 | 1.0 | 1.0 |
| AllowFirstShotAccuracy | true | true | false |
| SpreadAngleMultiplier_Aiming | 0.65 | 0.65 | 0.8 |
| SpreadAngleMultiplier_StandingStill | 0.8 | 0.9 | 0.9 |
| SpreadAngleMultiplier_Crouching | 0.6 | 0.65 | 0.8 |
| SpreadAngleMultiplier_JumpingOrFalling | 1.6 | 1.25 | 1.75 |
| TransitionRate_StandingStill | 5.0 | 6.0 | 2.2 |
| TransitionRate_Crouching | 5.0 | 5.0 | 5.0 |
| TransitionRate_JumpingOrFalling | 5.0 | 5.0 | 2.5 |
| StandingStillSpeedThreshold | 20.0 | 80.0 | 80.0 |
| StandingStillToMovingSpeedRange | 20.0 | 20.0 | 20.0 |

热量曲线：

- 三条 Heat 曲线在 WeaponInstance 蓝图上有配置，但 RuntimeFloatCurve 的 Python 拷贝不展开 Key。
- 代码逻辑：Instance 三条曲线有数据时优先，否则 Fragment 回退。
- 具体 Key 值待编辑器 Details 或后续 MCP 接口读取。

### Fire GA CDO

| 类 | DamageGameplayEffectClass | Fire Cue | Impact Cue |
|---|---|---|---|
| ShootGA_Weapon_Fire_Rifle | UShootEffect_DamageSetByCaller | GameplayCue.Weapon.Rifle.Fire | GameplayCue.Weapon.Rifle.Impact |
| ShootGA_Weapon_Fire_Pistol | UShootEffect_DamageSetByCaller | GameplayCue.Weapon.Pistol.Fire | GameplayCue.Weapon.Pistol.Impact |
| ShootGA_Weapon_Fire_Shotgun | UShootEffect_DamageSetByCaller | GameplayCue.Weapon.Shotgun.Fire | GameplayCue.Weapon.Shotgun.Impact |
| ShootGA_Weapon_Fire_Sniper | null | GameplayCue.Weapon.Sniper.Fire | GameplayCue.Weapon.Sniper.Impact |

### PhysMat 资产

| 资产 | 类 | Tags |
|---|---|---|
| /Game/Blueprints/Character/PhysMat_Player | PhysicalMaterialWithTags | 空 |
| /Game/Blueprints/Character/PhysMat_Player_WeakSpot | PhysicalMaterialWithTags | Gameplay.Zone.WeakSpot |

### GCN 资产

| 资产 | GameplayCueTag |
|---|---|
| /Game/Blueprints/GameplayCues/Weapons/Rifle/GCN_Weapon_Rifle_Fire | GameplayCue.Weapon.Rifle.Fire |
| /Game/Blueprints/GameplayCues/Weapons/Pistol/GCN_Weapon_Pistol_Fire | GameplayCue.Weapon.Pistol.Fire |
| /Game/Blueprints/GameplayCues/Weapons/Shotgun/GCN_Weapon_Shotgun_Fire | GameplayCue.Weapon.Shotgun.Fire |

## C++ 字段引用计数

统计范围：Source/NewWorldOrder 下所有 .h 与 .cpp。

| 字段 | 引用数 | 结论 |
|---|---|---|
| BaseDamage | 6 | 活跃，伤害主来源 |
| HeadshotMultiplier | 1 | 仅声明，无读取 |
| MaxRange | 8 | 活跃，Trace 距离，但其中多处为交互系统局部变量 |
| FalloffStart | 1 | 仅声明，无读取 |
| FalloffEnd | 1 | 仅声明，无读取 |
| DistanceFalloff | 3 | 活跃入口，但当前资产无 Key |
| MaterialDamageMultiplier | 2 | 活跃入口，但当前资产为空 |
| MagazineSize | 活跃 | 弹药 |
| MaxReserve | 活跃 | 弹药 |
| AmmoPerShot | 活跃 | 弹药 |
| FireInterval | 1 | 仅声明，无读取 |
| BurstCount | 1 | 仅声明，无读取 |
| bAutomaticFire | 1 | 仅声明，无读取 |
| BaseSpreadAngle | 活跃 | 旧散布回退 |
| SpreadAnglePerShot | 活跃 | 旧散布回退 |
| MaxSpreadAngle | 活跃 | 旧散布回退 |
| SpreadRecoveryRate | 活跃 | 旧散布回退 |
| LocalCameraPitch/Yaw Recoil | 活跃 | 本地表现 |
| MuzzleSocketName | 活跃 | 枪口位置 |
| AttachSocketHand | 1 | 仅声明，无读取 |
| AttachSocketHolster | 1 | 仅声明，无读取 |

## 关键代码行

当前伤害公式起点：

- ShootGA_Weapon_Fire_Rifle.cpp：ApplyDamageToTarget。
- ShootGA_Weapon_Fire_Shotgun.cpp：ApplyDamageToTarget。
- ShootGA_Weapon_Fire_Sniper.cpp：ApplyDamageToTarget。

硬编码标记加成：

- Rifle/Pistol 文件中的 VulnerableBonus = 0.1f。
- Shotgun 文件中的 VulnerableBonus = 0.1f。
- Sniper 文件中的 VulnerableBonus = 0.1f。

GE 写入：

- ShootEffect_DamageSetByCaller.cpp 中 FName("SetByCaller.Damage")。

属性处理：

- ShootAttributeSet.cpp 中 HandleIncomingDamage。
- 该函数不读取 Armor、ArmorPenetration、DamageReduction、DamageReductionBonus、CriticalHitChance、CriticalHitDamage、CriticalHitResistance。

## Lyra 11000 MCP 验证结果

1. 三个 GE_Damage 均为 Instant，Modifiers 空，Executions 只有 ULyraDamageExecution。
2. GA_Weapon_Fire 有 GE_Damage 变量；三个子蓝图分别指向 Pistol / RifleAuto / Shotgun 的 GE。
3. WeaponInstance 的 MaterialDamageMultiplier：
   - Pistol：Gameplay.Zone.WeakSpot = 2.0
   - Rifle：Gameplay.Zone.WeakSpot = 1.5
   - Shotgun：Gameplay.Zone.WeakSpot = 1.75
4. PhysMat_Player 无 Tag，PhysMat_Player_WeakSpot 为 Gameplay.Zone.WeakSpot。
5. Lyra BaseDamage 的非零来源仍未定位，GE、AbilitySet、WeaponInstance 均未发现赋值。
6. DistanceDamageFalloff 曲线有数据，但 Python 未展开 Key。

## 仍待验证

1. Lyra BaseDamage 非零来源。
2. 三把枪 DistanceDamageFalloff 的具体 Key 与数值。
3. 项目三把枪当前 PIE 实际伤害为 10 且无衰减、无爆头倍率。
4. 是否启用 Sniper 时 DamageGameplayEffectClass 为 null 的运行时表现。