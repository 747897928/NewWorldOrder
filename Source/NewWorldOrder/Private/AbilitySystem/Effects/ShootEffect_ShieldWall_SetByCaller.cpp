// 护盾墙 Buff：ShieldCapacityBonus/DamageReductionBonus 通过 SetByCaller 注入
#include "AbilitySystem/Effects/ShootEffect_ShieldWall_SetByCaller.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_ShieldWall_SetByCaller::UShootEffect_ShieldWall_SetByCaller()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));

	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UShootAttributeSet::GetShieldCapacityBonusAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = FName("SetByCaller.ShieldCapacityBonus");
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	}

	{
		FGameplayModifierInfo Mod;
		Mod.Attribute = UShootAttributeSet::GetDamageReductionBonusAttribute();
		Mod.ModifierOp = EGameplayModOp::Additive;

		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = FName("SetByCaller.DamageReductionBonus");
		Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Mod);
	}
}
