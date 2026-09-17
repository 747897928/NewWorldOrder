// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ShieldCapacity.generated.h"

/**
 * Modifier Magnitude Calculation for ShieldCapacity.
 * ShieldCapacity = (20% + Vitality × 1%) × MaxHP
 */
UCLASS()
class NEWWORLDORDER_API UMMC_ShieldCapacity : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_ShieldCapacity();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition VitalityDef;
	FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
};
