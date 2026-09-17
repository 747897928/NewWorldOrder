// Copyright ZhaoYiJie


#include "AbilitySystem/ModMagCal/MMC_CriticalHitChance.h"

#include "AbilitySystem/ShootAttributeSet.h"
#include "Interface/CombatInterface.h"

UMMC_CriticalHitChance::UMMC_CriticalHitChance()
{
	PerceptionDef.AttributeToCapture = UShootAttributeSet::GetPerceptionAttribute();
	PerceptionDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	PerceptionDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(PerceptionDef);
}

float UMMC_CriticalHitChance::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Perception = 0.f;
	GetCapturedAttributeMagnitude(PerceptionDef, Spec, EvaluationParameters, Perception);
	Perception = FMath::Max<float>(Perception, 0.f);

	// 文档v7.2公式：CriticalChance = Perception × 0.75%
	// 30点感知 = 22.5%暴击率
	// 返回值为百分比数值（例如22.5表示22.5%）
	return Perception * 0.75f;
}
