// 通用换弹 Buff（SetByCaller）：倍率由 GA 注入
#include "AbilitySystem/Effects/ShootEffect_ReloadSpeed_SetByCaller.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_ReloadSpeed_SetByCaller::UShootEffect_ReloadSpeed_SetByCaller()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(3.0f));

	FGameplayModifierInfo Mod;
	Mod.Attribute = UShootAttributeSet::GetReloadSpeedMultiplierAttribute();
	Mod.ModifierOp = EGameplayModOp::Multiplicitive;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataName = FName("SetByCaller.ReloadSpeedMultiplier");
	Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Mod);
}
