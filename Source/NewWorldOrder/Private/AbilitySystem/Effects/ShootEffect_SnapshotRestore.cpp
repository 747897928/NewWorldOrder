// 角色快照：通过 SetByCaller 注入属性 delta（仅用于恢复快照，不直接读写 AttributeSet）
#include "AbilitySystem/Effects/ShootEffect_SnapshotRestore.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_SnapshotRestore::UShootEffect_SnapshotRestore()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UShootAttributeSet::GetHealthAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = FName("SetByCaller.HealthSnapshotDelta");
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	}

	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UShootAttributeSet::GetShieldAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = FName("SetByCaller.ShieldSnapshotDelta");
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	}

	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UShootAttributeSet::GetUltimateChargeAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = FName("SetByCaller.UltimateChargeSnapshotDelta");
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	}
}
