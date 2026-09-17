// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Effects/ShootEffect_RobotCompanion.h"

#include "AbilitySystem/ShootAttributeSet.h"

UShootEffect_RobotCompanionVitals::UShootEffect_RobotCompanionVitals()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	auto AddSetByCallerOverride = [this](const FGameplayAttribute& Attribute, const FName DataName)
	{
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Override;
		FSetByCallerFloat SetByCaller;
		SetByCaller.DataName = DataName;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
		Modifiers.Add(Modifier);
	};

	// MaxHealth 必须先写，随后 Health 才不会被 AttributeSet 的 Clamp 限制在旧上限。
	AddSetByCallerOverride(UShootAttributeSet::GetMaxHealthAttribute(), FName("SetByCaller.Robot.MaxHealth"));
	AddSetByCallerOverride(UShootAttributeSet::GetHealthAttribute(), FName("SetByCaller.Robot.Health"));
}
