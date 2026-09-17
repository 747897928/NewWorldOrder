// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ArmorPenetration.generated.h"

/**
 * Modifier Magnitude Calculation for ArmorPenetration.
 * ArmorPenetration is calculated from Strength (Strength × 0.8%).
 */
UCLASS()
class NEWWORLDORDER_API UMMC_ArmorPenetration : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_ArmorPenetration();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition StrengthDef;
};
