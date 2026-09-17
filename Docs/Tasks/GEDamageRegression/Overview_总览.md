# GE 伤害系统回归 Lyra - 总览

## 目标

把武器开火伤害链收敛为：

1. 装备授予 Fire GA，Fire GA 的 SourceObject 是 WeaponInstance。
2. Fire GA 只负责服务器权威命中与 TargetData 整理。
3. 每把武器配置自己的 Damage GE。
4. Damage GE 或 Execution Calculation 负责把武器基础伤害、距离衰减、物理材质倍率、队伍规则转换为最终 Health 变化。
5. PhysMat_Player / PhysMat_Player_WeakSpot 只作为命中物理材质参与倍率，不在 GA 里写死 0.1 这类标记加成。
6. RPG 攻击属性以后通过 SetByCaller、Execution Calculation 或 Attribute Capture 接入，本阶段只留口，不实现。

## 边界

- 不引入 Lyra WeaponStateComponent。
- 保留项目现有 TargetData 回传和服务器重新 Trace 流程。
- 保留项目现有 ItemDefinition -> Equipment -> WeaponInstance -> AbilitySet 主线。
- 不修改 Lyra 或引擎源码。
- 调查基线已经完成，用户已批准进入分阶段实施。2026-08-22 已完成首期 GE 数值链；伤害数字、GameplayCue 与服务器伤害职责继续分离。

## 当前项目伤害链

```text
InputTag.LMB
-> ShootGA_Weapon_Fire_Rifle / Pistol / Shotgun
-> UShootGameplayAbility_Weapon_Fire
   -> 本地 TargetData / 服务器重新 Trace
   -> CommitAbility 扣弹药
   -> OnRangedWeaponTargetDataReady_Implementation
      -> Fire GameplayCue
      -> Impact GameplayCue
      -> ApplyWeaponDamageToTarget
         -> 从当前 WeaponInstance 选择每枪 Damage GE
         -> Context 写入 WeaponInstance、Instigator 与 HitResult
         -> ApplyGameplayEffectSpecToTarget
-> GE_Damage_Pistol / Rifle / Shotgun
   -> UShootDamageExecution
      -> GE BaseDamage
      -> WeaponInstance DistanceDamageFalloff
      -> WeaponInstance MaterialDamageMultiplier
      -> UShootAttributeSet::IncomingDamage
-> ShootAttributeSet::HandleIncomingDamage
   -> 友伤系数
   -> Shield 优先吸收
   -> 剩余伤害扣 Health
   -> 免疫死亡
   -> 死亡回调
   -> 伤害数字
```

## 当前问题

1. Pistol、Rifle、Shotgun 已有独立 GE_Damage 资产，但 Sniper 尚未迁移。
2. 三把首发武器已切到共享 UShootDamageExecution；投射物和其他旧调用方仍可能使用 UShootEffect_DamageSetByCaller。
3. 新伤害链的 BaseDamage 已写在每枪 GE；Fragment 旧 BaseDamage 暂留作兼容回退，尚未完成引用审计与清理。
4. HeadshotMultiplier 字段无任何 C++ 读取点。
5. 三把首发武器的 DistanceDamageFalloff 与 MaterialDamageMultiplier 已按 Lyra 精确配置并由 Execution 消费。
6. Rifle/Pistol/Shotgun 的 `Status.Marked +10%` 手算分支已经删除；Sniper 仍保留旧实现，未纳入本阶段。
7. 项目没有 Lyra 的 FLyraGameplayEffectContext 和 ULyraGameplayAbility::MakeEffectContext 自动注入 AbilitySource 的链路。
8. 当前 AttributeSet 的 IncomingDamage 后处理已做友伤、Shield 优先吸收、剩余伤害扣 Health 和死亡；Armor、DamageReduction、CriticalHit 等已存在属性仍未接入。
9. 现有 WeaponSystem STATUS 中“由 Pistol AbilitySet 配置具体 Montage 与伤害 GE”与资产事实不符；三个 AbilitySet 的 GrantedGameplayEffects 均为空。

## Lyra 参考链

```text
GA_Weapon_Fire_Pistol / GA_Weapon_Fire_Rifle_Auto / GA_Weapon_Fire_Shotgun
（Blueprint 子类）
-> ULyraGameplayAbility_RangedWeapon
   -> 本地 TargetData / WeaponStateComponent 确认
   -> BlueprintImplementableEvent OnRangedWeaponTargetDataReady
      -> Fire GameplayCue
      -> 每个命中 Impact GameplayCue
      -> 服务器逐 TargetData 应用各武器自己的 Damage GE
         （Pistol: GE_Damage_Pistol；Rifle: GE_Damage_RifleAuto；Shotgun: GE_Damage_Shotgun）
-> GE 继承 GameplayEffectParent_Damage_Basic
-> ULyraDamageExecution
   -> 捕获 Source ULyraCombatSet::BaseDamage
   -> 队伍 CanCauseDamage
   -> AbilitySource = WeaponInstance
   -> GetDistanceAttenuation（WeaponInstance 上的 DistanceDamageFalloff）
   -> GetPhysicalMaterialAttenuation（WeaponInstance 上的 MaterialDamageMultiplier）
   -> 输出到 ULyraHealthSet::Damage / Health
```

## 目标形态建议

- 保留 C++ 基类 ULyraGameplayAbility_RangedWeapon 式命中职责，开火 GA 只按武器选择 GE。
- 每把武器 GE 是数据资产或 Blueprint 子类，持有该武器基础伤害。
- 衰减与弱点倍率由 WeaponInstance 实现 ILyraAbilitySourceInterface，GE 执行时查询。
- 项目保留 SetByCaller GE 给旧投射物和技能调用方；三把首发武器的命中伤害不再经过 SetByCaller。
- 未来 RPG 属性通过属性捕获进入统一 Execution，不再增加 GA 内计算公式。

## 推荐决策

- 项目内实现轻量 Execution，不复制 Lyra 的 FLyraGameplayEffectContext。
- 每枪 GE 持有 BaseDamage，Execution 读取 GE 的 BaseDamage。
- 距离衰减和 PhysMat 倍率从 Fragment 迁到 WeaponInstance。
- 具体步骤见 Implementation_迁移方案.md。
