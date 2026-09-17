// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "SecondaryAttributesEffect.generated.h"

/**
 * Template GameplayEffect for applying secondary attributes calculated from primary attributes.
 *
 * This effect uses Modifier Magnitude Calculations (MMCs) to derive secondary attributes:
 * - MaxHealth (from Vitality via MMC_MaxHealth)
 * - Armor (from Agility via MMC_Armor)
 * - ArmorPenetration (from Strength via MMC_ArmorPenetration)
 * - CriticalHitChance (from Perception via MMC_CriticalHitChance)
 * - CriticalHitDamage (from Perception via MMC_CriticalHitDamage)
 * - ShieldCapacity (from Vitality via MMC_ShieldCapacity)
 * - DamageReduction (from Vitality via MMC_DamageReduction)
 *
 * Duration Policy: Infinite (attributes persist until explicitly removed)
 * Application Policy: Instant (calculations apply immediately when attributes change)
 *
 * Blueprint Usage:
 * - Create a Blueprint child class (e.g., "GE_SecondaryAttributes")
 * - The modifiers are configured in C++ and inherited by Blueprint
 * - Can be customized in Blueprint if needed, but default setup should work as-is
 */
UCLASS(Blueprintable, BlueprintType)
class NEWWORLDORDER_API USecondaryAttributesEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USecondaryAttributesEffect();
};
