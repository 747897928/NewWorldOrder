// Copyright ZhaoYiJie


#include "AbilitySystem/ModMagCal/MMC_DamageReduction.h"

#include "AbilitySystem/ShootAttributeSet.h"

UMMC_DamageReduction::UMMC_DamageReduction()
{
	VitalityDef.AttributeToCapture = UShootAttributeSet::GetVitalityAttribute();
	VitalityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	VitalityDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(VitalityDef);
}

float UMMC_DamageReduction::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Vitality = 0.f;
	GetCapturedAttributeMagnitude(VitalityDef, Spec, EvaluationParameters, Vitality);
	Vitality = FMath::Max<float>(Vitality, 0.f);

	// 文档v7.2公式：DamageReduction = Vitality × 0.2%
	// 30点体力: 6%减伤
	// 返回值为百分比数值（例如6表示6%）
	return Vitality * 0.2f;
}
