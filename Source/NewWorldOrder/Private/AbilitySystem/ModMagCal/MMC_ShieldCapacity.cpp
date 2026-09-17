// Copyright ZhaoYiJie


#include "AbilitySystem/ModMagCal/MMC_ShieldCapacity.h"

#include "AbilitySystem/ShootAttributeSet.h"

UMMC_ShieldCapacity::UMMC_ShieldCapacity()
{
	VitalityDef.AttributeToCapture = UShootAttributeSet::GetVitalityAttribute();
	VitalityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	VitalityDef.bSnapshot = false;

	MaxHealthDef.AttributeToCapture = UShootAttributeSet::GetMaxHealthAttribute();
	MaxHealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	MaxHealthDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(VitalityDef);
	RelevantAttributesToCapture.Add(MaxHealthDef);
}

float UMMC_ShieldCapacity::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Vitality = 0.f;
	GetCapturedAttributeMagnitude(VitalityDef, Spec, EvaluationParameters, Vitality);
	Vitality = FMath::Max<float>(Vitality, 0.f);

	float MaxHealth = 0.f;
	GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvaluationParameters, MaxHealth);
	MaxHealth = FMath::Max<float>(MaxHealth, 0.f);

	// 文档v7.2公式：ShieldCapacity = (20% + Vitality × 1%) × MaxHP
	// 30点体力男主（MaxHP=414）: 414 × 0.5 = 207护盾
	float capacityPercentage = 0.2f + (Vitality * 0.01f);
	return MaxHealth * capacityPercentage;
}
