// 通用射速 Buff，默认乘以 1.3，可在子类或默认对象上修改
#include "AbilitySystem/Effects/ShootEffect_FireRate.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_FireRate::UShootEffect_FireRate()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f)); // 默认 5 秒

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetFireRateMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.3f));
	Modifiers.Add(Mod);
}
