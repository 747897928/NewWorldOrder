// 通用治疗受益加成，默认乘以 1.2，可在子类/默认对象覆盖
#include "AbilitySystem/Effects/ShootEffect_HealingReceived.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_HealingReceived::UShootEffect_HealingReceived()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(6.0f));

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetHealingReceivedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.2f));
	Modifiers.Add(Mod);
}
