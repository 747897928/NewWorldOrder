// 通用减伤效果，默认 +0.2（即额外 20% 减伤），持续 4 秒，可在子类/默认对象覆盖
#include "AbilitySystem/Effects/ShootEffect_DamageReduction.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_DamageReduction::UShootEffect_DamageReduction()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(4.0f));

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetDamageReductionBonusAttribute();
	Mod.ModifierOp = EGameplayModOp::Additive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(0.2f));
	Modifiers.Add(Mod);
}
