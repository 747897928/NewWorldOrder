// 通用移速 Buff，默认乘以 1.2，可在子类或默认对象上修改
#include "AbilitySystem/Effects/ShootEffect_MoveSpeed.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_MoveSpeed::UShootEffect_MoveSpeed()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f)); // 默认 3 秒，可覆写

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetMoveSpeedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.2f));
	Modifiers.Add(Mod);
}
