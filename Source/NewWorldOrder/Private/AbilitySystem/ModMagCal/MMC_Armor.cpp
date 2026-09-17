// Copyright ZhaoYiJie


#include "AbilitySystem/ModMagCal/MMC_Armor.h"

#include "AbilitySystem/ShootAttributeSet.h"

UMMC_Armor::UMMC_Armor()
{
	AgilityDef.AttributeToCapture = UShootAttributeSet::GetAgilityAttribute();
	AgilityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	AgilityDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(AgilityDef);
}

float UMMC_Armor::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Agility = 0.f;
	GetCapturedAttributeMagnitude(AgilityDef, Spec, EvaluationParameters, Agility);
	Agility = FMath::Max<float>(Agility, 0.f);

	// 简单线性公式作为示例（具体数值可根据游戏平衡调整）
	// 敌人通常不使用此MMC，而是直接在GE中设置固定护甲值
	// 此MMC主要供需要动态计算护甲的情况使用
	return 5.f + 2.f * Agility;
}
