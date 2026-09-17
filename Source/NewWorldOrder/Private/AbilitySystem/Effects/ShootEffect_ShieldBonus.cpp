// 通用护盾容量加成，默认 +20% MaxShield（通过 ShieldCapacityBonus 加法），持续 6 秒
#include "AbilitySystem/Effects/ShootEffect_ShieldBonus.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_ShieldBonus::UShootEffect_ShieldBonus()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(6.0f));

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetShieldCapacityBonusAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.2f));
	Modifiers.Add(Mod);
}
