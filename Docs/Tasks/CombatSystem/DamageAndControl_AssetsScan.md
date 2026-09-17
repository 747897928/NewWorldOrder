## Damage & Control GameplayEffects

- ShootEffect_Vulnerable (Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_Vulnerable.h / Private/.../ShootEffect_Vulnerable.cpp)  
  - Tag: Status.Vulnerable (FShootGameplayTags::Status_Vulnerable)  
  - Effect: 易伤/受伤放大，具体数值在 GE 配置中设置  
  - Usage: 可复用为易伤/标记类技能（如战术突击/震荡手雷）
  - 现状：Tag 授予由 `UTargetTagsGameplayEffectComponent` 配置（替代已废弃的 `InheritableOwnedTagsContainer`）。
- ShootEffect_HealInstant (Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_HealInstant.h / Private/.../ShootEffect_HealInstant.cpp)
  - Effect: Health += SetByCaller.Heal（即时治疗）
  - Usage: MedicalStation 等周期治疗/瞬时治疗的通用 GE（由调用方负责计算 SetByCaller.Heal）
- ShootEffect_SnapshotRestore (Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_SnapshotRestore.h / Private/.../ShootEffect_SnapshotRestore.cpp)
  - Effect: 通过 SetByCaller 注入属性 delta（Health/ShieldCapacity/UltimateCharge），用于“快照恢复”场景
  - Usage: AShootPlayerState::LoadCharacterFromSnapshot 在恢复 Inventory/QuickBar/外观后调用（不直接写 AttributeSet）
- ShootEffect_MoveSpeed / FireRate / ReloadSpeed / ShieldBonus / DamageReduction / HealingDone / HealingReceived  
  - Buff/DR/治疗系，均为标准 GE，可叠加到技能 Buff 窗口（Overload、RapidCharge 等）  
  - Tag: 无强制状态 Tag，依赖 GE 配置
- ShootEffect_Stun (Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_Stun.h / Private/.../ShootEffect_Stun.cpp)  
  - Tag: Status.Stunned + Ability.Weapon.NoFiring  
  - Effect: 眩晕状态（阻止开火），时长由外部 Spec 覆盖（爆炸控制可直接设 Duration）
  - Usage: ShockGrenade 的 ControlEffectClass 默认使用
- ShootEffect_Slow (Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_Slow.h / Private/.../ShootEffect_Slow.cpp)  
  - Tag: Status.Slowed  
  - Effect: MoveSpeedMultiplier *= 0.6（注意：角色移动系统需要消费该属性）  
  - Usage: 可用于技能/弹药造成减速效果
- ShootEffect_MoveSpeed_SetByCaller (Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_MoveSpeed_SetByCaller.h / Private/.../ShootEffect_MoveSpeed_SetByCaller.cpp)  
  - Tag: SetByCaller.MoveSpeedMultiplier  
  - Effect: MoveSpeedMultiplier *= SetByCaller 值  
  - Usage: RescueCloak/RapidCharge 等配置化移速 Buff
- ShootEffect_ReloadSpeed_SetByCaller (Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_ReloadSpeed_SetByCaller.h / Private/.../ShootEffect_ReloadSpeed_SetByCaller.cpp)  
  - Tag: SetByCaller.ReloadSpeedMultiplier  
  - Effect: ReloadSpeedMultiplier *= SetByCaller 值  
  - Usage: RapidCharge Lv3 换弹加速
- ShootEffect_ImmuneDeath (Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_ImmuneDeath.h / Private/.../ShootEffect_ImmuneDeath.cpp)  
  - Tag: Status.ImmuneDeath  
  - Effect: 生命值不会被降到 0 以下（由 AttributeSet 判定）  
  - Usage: 医疗专精满级的救援保护窗口
- ShootEffect_Marked (Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_Marked.h / Private/.../ShootEffect_Marked.cpp)  
  - Tag: Status.Marked  
  - Effect: 授予标记状态（支持 Duration 覆盖）  
  - Usage: MarkHunter 标记传播等机制
- Knockback 方案：`ShootSkillExplosionActor` 支持 KnockbackStrength，优先对 Character 使用 LaunchCharacter，否则对物理组件 AddRadialImpulse
- 未发现额外 Damage 直伤类 GE（伤害多由 Execution 或 GA 直接应用）
- TacticalAssault Phase 1：命中后可配置 DashDamageEffect/DashVulnerableEffect（推荐使用 ShootEffect_Vulnerable），未涉及硬控/击退。

## Grenade / Projectile Reusable Flow

- Common projectile GAs: ShootGA_Weapon_Fire_Projectile (现已使用 AmmoTagStack)  
  - Spawns projectile via weapon instance logic (Lyra-style), includes impact processing and GameplayCue hooks
- Skill actors:  
  - ShootSkillExplosionActor (Public/Private/AbilitySystem/Actors)  
    - Used by male ShockGrenade (ShootGA_Male_ShockGrenade.cpp) to spawn explosion; has ApplyExplosion, radial damage/GE hooks  
  - ShootSkillMedicalStation (healing to allies; periodic heal)  
  - Suggestion: ShockGrenade 复用现有 projectile spawn pipeline + ShootSkillExplosionActor for AoE damage/control; configure damage/CC GE on actor
- No dedicated “grenade projectile” class beyond general projectile spawning; can reuse weapon projectile flow plus explosion actor for detonation.

## Shield / Bulwark Actors

- AShootSkillShieldWall (Public/Private/AbilitySystem/Actors/ShootSkillShieldWall.*)  
  - InitShield(AActor* InOwnerActor), ApplyShield/RemoveShield; spawn via ShootGA_Male_SteelBulwark  
  - Uses collision component for blocking; supports owner assignment and begin/end play cleanup  
  - No explicit reflect API; currently basic spawn + shield apply/remove. Can extend with OnHit overlap for blocking/DR.
- ShootGA_Male_SteelBulwark spawns ShieldWallClass (default AShootSkillShieldWall) at front of player; handles spawn transform; no advanced reflect yet.
- No other reusable shield/barrier actors found.

## Ammo Consumption Extension Points (Current Reality)

- Authority ammo cost: GA → UShootAbilityCost_AmmoTagStack → UShootAbilityCost_ItemTagStack → ItemInstance.StatTags (Inventory_Ammo_Magazine, Quantity=GetAmmoPerShot)  
  - CheckCost/ApplyCost in AmmoTagStack now skip when ASC has Status_Overload (Overload = “infinite ammo” window)  
  - No remaining calls to UShootAbilityCost_Ammo (class removed); no current uses of UShootRangedWeaponInstance::ConsumeAmmo*
- All standard fire GAs (Projectile/Rifle/Sniper/Shotgun/GrenadeLauncher/RocketLauncher) now use AmmoTagStack; Overload 跳过扣弹。

## Recommended Reuse (for upcoming male Q/E/C)

- Damage/Control: reuse ShootEffect_Vulnerable for易伤; use existing DR/Move/FireRate/ReloadSpeed buffs; Knockback 由 SkillExplosionActor 执行;  
- ShockGrenade: reuse projectile spawn flow from weapon fire + ShootSkillExplosionActor for AoE; configure damage/CC GE on explosion.  
- SteelBulwark: reuse AShootSkillShieldWall spawn via GA; extend in GA/Actor later for blocking/DR; no reflect API present yet.

## Known Gaps / TODO (no code changes in this pass)

- No shared Knockback GE assets found; 使用 SkillExplosionActor 的 KnockbackStrength 即可。  
- Document/UI may still reference old Ammo cost; current code uses AmmoTagStack + Overload skip.
