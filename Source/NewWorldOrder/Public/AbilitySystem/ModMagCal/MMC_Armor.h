// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_Armor.generated.h"

/**
 * Modifier Magnitude Calculation for Armor.
 * Armor reduces incoming damage, can be used by both players and enemies.
 * For players: calculated from Agility (if using calculated armor).
 * For enemies: typically set as fixed value via GE, but can also use this MMC.
 */
UCLASS()
class NEWWORLDORDER_API UMMC_Armor : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UMMC_Armor();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition AgilityDef;
};
