# 当前项目伤害链调查

## 资产基线

### 资产检索结果

通过 NewWorldOrder 编辑器 MCP 对 /Game 做 AssetRegistry 检索：

- 名称含 GE_ 的资产共 22 个，其中没有 GE_Damage_Pistol、GE_Damage_Rifle、GE_Damage_Shotgun。
- 名称含 GA_Weapon_Fire 的资产为 0。
- 名称含 GCN_ 的资产为 3 个：
  - /Game/Blueprints/GameplayCues/Weapons/Pistol/GCN_Weapon_Pistol_Fire
  - /Game/Blueprints/GameplayCues/Weapons/Rifle/GCN_Weapon_Rifle_Fire
  - /Game/Blueprints/GameplayCues/Weapons/Shotgun/GCN_Weapon_Shotgun_Fire
- PhysMat 资产：
  - /Game/Blueprints/Character/PhysMat_Player，PhysicalMaterialWithTags，Tags 为空。
  - /Game/Blueprints/Character/PhysMat_Player_WeakSpot，PhysicalMaterialWithTags，Tags 为 Gameplay.Zone.WeakSpot。

### 三把武器资产关系

Rifle：

- ID_Rifle 的 Fragments：EquippableItem、WeaponBasicConfig、RangedWeaponConfig、SetStats。
- EquippableItem.EquipmentDefinition 指向 BP_Equipment_Rifle。
- BP_Equipment_Rifle.InstanceType 指向 B_WeaponInstance_Rifle_C。
- BP_Equipment_Rifle.AbilitySetsToGrant 为 AS_Weapon_Rifle。
- AS_Weapon_Rifle.GrantedGameplayAbilities：
  - ShootGA_Weapon_Fire_Rifle，Level 1。
  - GA_Weapon_Reload_Rifle_C，Level 1。
- AS_Weapon_Rifle.GrantedGameplayEffects 为空。
- AS_Weapon_Rifle.GrantedAttributes 为空。

Pistol：

- ID_Pistol -> BP_Equipment_Pistol -> B_WeaponInstance_Pistol_C -> AS_Weapon_Pistol。
- AS_Weapon_Pistol 授予 ShootGA_Weapon_Fire_Pistol 与 GA_Weapon_Reload_Pistol_C。
- GrantedGameplayEffects 为空。

Shotgun：

- ID_Shotgun -> BP_Equipment_Shotgun -> B_WeaponInstance_Shotgun_C -> AS_Weapon_Shotgun。
- AS_Weapon_Shotgun 授予 ShootGA_Weapon_Fire_Shotgun 与 GA_Weapon_Reload_Shotgun_C。
- GrantedGameplayEffects 为空。

## 开火能力继承关系

```text
UGameplayAbility
-> UShootGameplayAbility
-> UShootGameplayAbility_Weapon_Fire
   -> UShootGA_Weapon_Fire_Rifle
      -> UShootGA_Weapon_Fire_Pistol
   -> UShootGA_Weapon_Fire_Shotgun
   -> UShootGA_Weapon_Fire_Sniper
```

- Rifle 是完整 C++ 实现。
- Pistol 只改 Fire/Impact Cue Tag 与 FireDelayTimeSecs。
- Shotgun 与 Sniper 各自重写一份 OnRangedWeaponTargetDataReady_Implementation 和 ApplyDamageToTarget，与 Rifle 主体重复。
- 投射物开火走另一条链：UShootGA_Weapon_Fire_Projectile -> AShootProjectileBase。

## 基类命中流程

UShootGameplayAbility_Weapon_Fire 的流程：

1. ActivateAbility 绑定 TargetData delegate，更新 WeaponInstance 开火时间。
2. 子类 ActivateAbility 在本地控制端调用 StartRangedWeaponTargeting。
3. 本地执行 PerformLocalTargeting。
4. OnTargetDataReadyCallback 中：
   - Authority 时丢弃客户端 TargetData，用自己的视角和当前散布重新 PerformLocalTargeting。
   - 客户端把本地 TargetData 回传服务器。
   - CommitAbility 成功才 AddSpread、应用本地相机后坐力、调用 OnRangedWeaponTargetDataReady。
5. 命中 Trace 使用 Lyra_TraceChannel_Weapon，bReturnPhysicalMaterial 为 true。

## Rifle/Pistol 伤害实现

文件：Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_Rifle.cpp

构造函数：

- DamageGameplayEffectClass 默认设为 UShootEffect_DamageSetByCaller::StaticClass()。
- FireGameplayCueTag 默认 GameplayCue.Weapon.Rifle.Fire。
- ImpactGameplayCueTag 默认 GameplayCue.Weapon.Rifle.Impact。
- Pistol 子类只把两个 Tag 换成 Pistol。

OnRangedWeaponTargetDataReady_Implementation：

- PlayFireEffects 执行 Fire Cue，SourceObject 设为 WeaponInstance。
- ProcessHits 遍历每个真实 HitResult。
- Authority 时对每个命中调用 ApplyDamageToTarget。

ApplyDamageToTarget 的实际顺序：

1. TargetASC = TargetActor 的 ASC。
2. SourceASC = 自身 ASC。
3. EffectContext = SourceASC->MakeEffectContext()。
4. EffectContext.AddSourceObject(Weapon)。
5. EffectContext.AddInstigator(AvatarActor, AvatarActor)。
6. EffectContext.AddHitResult(Hit)。
7. MakeOutgoingSpec(DamageGameplayEffectClass, GetAbilityLevel(), EffectContext)。
8. FinalDamage = Weapon->GetBaseDamage()。
9. FinalDamage *= Weapon->GetDistanceAttenuation(Distance)。
10. FinalDamage *= Weapon->GetPhysicalMaterialAttenuation(Hit.PhysMaterial)。
11. 若 Hit.PhysMaterial 含 Gameplay.Zone.WeakSpot 且 TargetASC 含 Status.Marked，则 FinalDamage *= 1.1。
12. Spec.SetSetByCallerMagnitude(FName("SetByCaller.Damage"), FinalDamage)。
13. SourceASC->ApplyGameplayEffectSpecToTarget(Spec, TargetASC)。

## Shotgun 与 Sniper 伤害实现

Shotgun 与 Sniper 各自复制了几乎相同的 ApplyDamageToTarget：

- Shotgun 默认 DamageGameplayEffectClass = UShootEffect_DamageSetByCaller。
- Sniper 构造函数没有设置 DamageGameplayEffectClass，CDO 值为 null，当前 Sniper 命中会静默跳过伤害。
- Shotgun 与 Sniper 也各自写死了 WeakSpot 且 Status.Marked 时乘 1.1。
- Sniper 使用 FGameplayTag::RequestGameplayTag(FName("SetByCaller.Damage"))，其余类使用 FName，最终通道相同。

## 伤害 GE 与 AttributeSet

UShootEffect_DamageSetByCaller：

- Instant。
- Modifier：UShootAttributeSet::IncomingDamage += SetByCaller.Damage。
- 它只负责把已算好的值写入 IncomingDamage，不做衰减、弱点、队伍或属性计算。

UShootAttributeSet::HandleIncomingDamage：

1. 读取并清零 IncomingDamage。
2. 检查目标存活。
3. 通过 AShootGameModeBase::GetFriendlyFireScalarForActors 计算友伤系数。
4. 检查 Status.ImmuneDeath，死亡时最低保留 1 点血。
5. SetHealth。
6. 发送伤害数字请求，WeakSpot 用于负 W 暴击样式。
7. Health 归零时调用 ICombatInterface::Die 并通知 GameMode。

当前没有消费：

- Armor。
- ArmorPenetration。
- DamageReduction。
- DamageReductionBonus。
- CriticalHitChance。
- CriticalHitDamage。
- CriticalHitResistance。

这些属性已经存在于 ShootAttributeSet，但武器伤害后处理没有进入该公式链。

## 伤害数据来源

UShootRangedWeaponInstance::GetBaseDamage()：

- 从当前 ItemInstance 的 ItemDefinition 查找 ShootInventoryFragment_RangedWeaponConfig。
- 返回 RangedCfg->BaseDamage。
- 找不到 Fragment 时返回 10.0。

三把枪当前 MCP 读取值：

- Rifle BaseDamage = 10.0，HeadshotMultiplier = 1.0。
- Pistol BaseDamage = 10.0，HeadshotMultiplier = 1.0。
- Shotgun BaseDamage = 10.0，HeadshotMultiplier = 1.0。

距离衰减：

- GetDistanceAttenuation 只读 RangedCfg->DistanceFalloff 曲线。
- 三把枪该曲线当前没有 Key，函数返回 1.0。
- FalloffStart 与 FalloffEnd 字段没有读取点，所以即使配置了也不生效。

物理材质衰减：

- GetPhysicalMaterialAttenuation 遍历 RangedCfg->MaterialDamageMultiplier。
- 三把枪该 Map 当前为空，函数返回 1.0。
- HeadshotMultiplier 没有进入该函数，也没有任何 C++ 读取点。

## 弹药与散布字段现状

- 弹药字段 MagazineSize、MaxReserve、AmmoPerShot 是活跃字段。
- 散布字段 SpreadExponent、BaseSpreadAngle、SpreadAnglePerShot、MaxSpreadAngle、SpreadRecoveryRate 是活跃字段。
- 热量曲线三件套在 Fragment 与 WeaponInstance 上各有一份；WeaponInstance 三条曲线齐全时优先使用 Instance 模型，Fragment 作为兼容回退。
- BulletsPerCartridge 活跃，Shotgun 当前为 9。
- BulletTraceSweepRadius 活跃。
- 本地后坐力四个字段活跃，只作用于开火者本地 PlayerController。
- MuzzleSocketName 活跃。
- FireInterval、BurstCount、bAutomaticFire 在 C++ 中没有读取点。
- AttachSocketHand、AttachSocketHolster 在 C++ 中没有读取点。

## GameplayCue 当前链

- GA 执行 K2_ExecuteGameplayCueWithParams(FireTag, CueParameters)。
- CueParameters 取第一个 TargetData 的 HitResult，SourceObject 设为 WeaponInstance。
- 三个 GCN 蓝图父类为 UShootGameplayCueNotify_WeaponFire。
- C++ 适配器 ResolveWeaponPresentationActor 从 WeaponInstance 的 SpawnedActors 找带 Fire 函数的 WeaponActor。
- ExecuteLyraWeaponFire 只传一组 ImpactPositions、ImpactNormals、ImpactSurfaceTypes。
- 当前 Fire Cue 参数来自第一个命中，不是全部 pellet 命中。

## 旧文档与实际代码的矛盾

Docs/Tasks/WeaponSystem/STATUS.md 第 24 条声称：

- “Pistol AbilitySet 配置具体 Montage 与伤害 GE”。

实际 MCP 读取：

- Pistol AbilitySet 只配置了两个 GrantedGameplayAbilities。
- GrantedGameplayEffects 为空。
- Pistol Fire GA 是 C++ 类，Montage 从 WeaponInstance Fragment 读取。
- Damage GE 来自 Rifle 构造函数默认值 UShootEffect_DamageSetByCaller。

Docs/Tasks/CombatSystem/DamageAndControl_AssetsScan.md 提到：

- “伤害多由 Execution 或 GA 直接应用”。

当前武器链没有 Execution Calculation，伤害由 GA 计算后通过 SetByCaller GE 应用。

## 当前设计文档的数值差距

Docs/SystemDesign/GameDesign/NumericalDesign/Weapons/ 中：

- AK47 BaseDamage 35，M4A1 BaseDamage 30。
- CritMultiplier 2.0。
- 有 Curve_CloseRange、Curve_MidRange、Curve_LongRange 距离衰减设计。
- 感知影响弱点倍率。

当前三把枪资产：

- BaseDamage 均为 10。
- 弱点倍率字段均为 1 且未读取。
- 距离衰减曲线为空。
- 物理材质倍率表为空。

结论：当前三把枪是 Lyra ShooterCore 的 POC 数值，不是项目正式武器数值。
