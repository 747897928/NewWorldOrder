// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_MaxHealth.generated.h"

/**
 * modifier magnitude calculation.
 */
UCLASS()
class NEWWORLDORDER_API UMMC_MaxHealth : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_MaxHealth();

	// 最重要的函数：计算基础数值 magnitude。
	// 当一个GameplayEffect使用这个类时，就会调用这个函数来决定最终要改变多少数值。
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:

	// 定义一个属性捕获定义。它告诉GAS："我需要从目标（Target）身上捕获Vitality这个属性的值"。
	FGameplayEffectAttributeCaptureDefinition VitalityDef;
};
