// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_CriticalHitDamage.generated.h"

/**
 * Modifier Magnitude Calculation for CriticalHitDamage.
 * Returns base critical hit multiplier (150% = 1.5x damage).
 * v7.2: Updated from 1.2x to 1.5x to match game design standards.
 */
UCLASS()
class NEWWORLDORDER_API UMMC_CriticalHitDamage : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_CriticalHitDamage();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
