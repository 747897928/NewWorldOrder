// Copyright ZhaoYiJie


#include "AbilitySystem/ModMagCal/MMC_MaxHealth.h"

#include "AbilitySystem/ShootAttributeSet.h"
#include "Interface/CombatInterface.h"

UMMC_MaxHealth::UMMC_MaxHealth()
{
	// 在构造函数中，初始化属性捕获定义 VitalityDef。
	// AttributeToCapture: 要捕获哪个属性？这里通过静态函数 GetVitalityAttribute() 获取到Vitality属性的标识符。
	// AttributeSource: 从谁身上捕获？EGameplayEffectAttributeCaptureSource::Target 表示从效果的目标（即拥有这个属性集的对象）身上捕获。
	// bSnapshot: 是否捕获"快照"？false表示在效果应用的那一刻实时获取属性值。true则表示在效果创建时就获取（适用于一些需要固定值的场景）。
	VitalityDef.AttributeToCapture = UShootAttributeSet::GetVitalityAttribute();
	VitalityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	VitalityDef.bSnapshot = false;

	// 将这个捕获定义添加到"需要捕获的相关属性"列表中。必须在构造函数中完成这一步！
	RelevantAttributesToCapture.Add(VitalityDef);
}

float UMMC_MaxHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// Gather tags from source and target
	// 1. 准备评估参数
	// GameplayEffectSpec 包含了这次效果应用的所有信息（来源、等级、上下文等）。
	// 我们从Spec中获取来源（Source）和目标（Target）的GameplayTag容器。
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// 创建一个评估参数结构体，并把Tags设置进去。某些属性计算可能需要根据Tag来做决策。
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// 2. 捕获属性值
	float Vitality = 0.f;
	GetCapturedAttributeMagnitude(VitalityDef, Spec, EvaluationParameters, Vitality);
	Vitality = FMath::Max<float>(Vitality, 0.f);

	// 3. 使用文档v7.2三段递减公式计算VitalityBonus
	// 0-10点: 每点+12 HP
	// 11-20点: 每点+8 HP
	// 21-30点: 每点+6 HP
	float VitalityBonus = 0.f;

	if (Vitality <= 10.f)
	{
		// 0-10点: 每点+12 HP
		VitalityBonus = Vitality * 12.f;
	}
	else if (Vitality <= 20.f)
	{
		// 10*12 + (Vitality-10)*8
		VitalityBonus = 120.f + (Vitality - 10.f) * 8.f;
	}
	else
	{
		// 10*12 + 10*8 + (Vitality-20)*6
		VitalityBonus = 120.f + 80.f + (Vitality - 20.f) * 6.f;
	}

	// MaxHP = BaseHP(100) + VitalityBonus
	// 注意：角色系数（男主+15%，女主-10%）应在GE中通过Multiplier处理
	return 100.f + VitalityBonus;
}
