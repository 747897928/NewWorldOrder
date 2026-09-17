// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Effects/ShootEffect_ZombieMeleeCooldown.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootEffect_ZombieMeleeCooldown)

UShootEffect_ZombieMeleeCooldown::UShootEffect_ZombieMeleeCooldown(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureCooldown(ObjectInitializer, TEXT("Cooldown.AI.ZombieMelee"), 1.0f);
}
