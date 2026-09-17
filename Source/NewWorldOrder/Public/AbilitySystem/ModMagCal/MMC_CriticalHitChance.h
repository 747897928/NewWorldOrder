// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_CriticalHitChance.generated.h"

/**
 * Modifier Magnitude Calculation for CriticalHitChance.
 * CriticalHitChance is calculated from Perception (Perception × 0.75%).
 */
UCLASS()
class NEWWORLDORDER_API UMMC_CriticalHitChance : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_CriticalHitChance();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition PerceptionDef;
};
