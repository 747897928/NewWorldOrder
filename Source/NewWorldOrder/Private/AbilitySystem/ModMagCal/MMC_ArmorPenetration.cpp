// Copyright ZhaoYiJie


#include "AbilitySystem/ModMagCal/MMC_ArmorPenetration.h"

#include "AbilitySystem/ShootAttributeSet.h"
#include "Interface/CombatInterface.h"

UMMC_ArmorPenetration::UMMC_ArmorPenetration()
{
	StrengthDef.AttributeToCapture = UShootAttributeSet::GetStrengthAttribute();
	StrengthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StrengthDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(StrengthDef);
}

float UMMC_ArmorPenetration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Strength = 0.f;
	GetCapturedAttributeMagnitude(StrengthDef, Spec, EvaluationParameters, Strength);
	Strength = FMath::Max<float>(Strength, 0.f);

	// 文档v7.2公式：ArmorPenetration = Strength × 0.8%
	// 30点力量 = 24%穿透
	// 返回值为百分比数值（例如24表示24%）
	return Strength * 0.8f;
}
