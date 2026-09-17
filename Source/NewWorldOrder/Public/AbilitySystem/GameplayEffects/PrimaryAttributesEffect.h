// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PrimaryAttributesEffect.generated.h"

/**
 * Template GameplayEffect for initializing or modifying primary attributes.
 *
 * Primary attributes are the core attributes that players can directly upgrade:
 * - Strength (力量): Increases weapon damage and armor penetration
 * - Vitality (体质): Increases max health, shield capacity, and damage reduction
 * - Agility (敏捷): Increases movement speed, reload speed, and dodge effectiveness
 * - Perception (感知): Increases critical hit chance, critical damage, and detection range
 *
 * Duration Policy: Instant (applies attribute changes immediately)
 * Application Policy: Can be configured per use case
 *
 * Usage Scenarios:
 * 1. Character Initialization: Set starting primary attributes based on character class
 * 2. Level Up: Grant attribute points that players can allocate
 * 3. Temporary Buffs: Provide temporary attribute boosts from consumables or abilities
 *
 * Blueprint Usage:
 * - Create Blueprint child classes for different scenarios (e.g., "GE_InitialAttributes_Male", "GE_LevelUpAttributes")
 * - Configure SetByCaller magnitudes or fixed values in Blueprint as needed
 * - This C++ template provides the base configuration that can be customized
 */
UCLASS(Blueprintable, BlueprintType)
class NEWWORLDORDER_API UPrimaryAttributesEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPrimaryAttributesEffect();
};
