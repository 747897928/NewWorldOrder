// 通用换弹速度 Buff，默认乘以 1.3，可在子类或默认对象上修改
#include "AbilitySystem/Effects/ShootEffect_ReloadSpeed.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_ReloadSpeed::UShootEffect_ReloadSpeed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetReloadSpeedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.3f));
	Modifiers.Add(Mod);
}
