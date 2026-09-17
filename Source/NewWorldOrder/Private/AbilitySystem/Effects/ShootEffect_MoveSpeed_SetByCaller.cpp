// 通用移速 Buff（SetByCaller）：倍率由 GA 注入，避免硬编码
#include "AbilitySystem/Effects/ShootEffect_MoveSpeed_SetByCaller.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_MoveSpeed_SetByCaller::UShootEffect_MoveSpeed_SetByCaller()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetMoveSpeedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("SetByCaller.MoveSpeedMultiplier");
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Mod);
}
