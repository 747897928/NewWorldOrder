// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_DamageReduction.generated.h"

/**
 * Modifier Magnitude Calculation for DamageReduction.
 * DamageReduction = Vitality × 0.2%
 */
UCLASS()
class NEWWORLDORDER_API UMMC_DamageReduction : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_DamageReduction();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition VitalityDef;
};
