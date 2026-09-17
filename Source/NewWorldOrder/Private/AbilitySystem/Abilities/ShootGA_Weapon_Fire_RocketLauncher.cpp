// Copyright ZhaoYiJie

// AbilitySystem/Abilities/ShootGA_Weapon_Fire_RocketLauncher.cpp
#include "AbilitySystem/Abilities/ShootGA_Weapon_Fire_RocketLauncher.h"

UShootGA_Weapon_Fire_RocketLauncher::UShootGA_Weapon_Fire_RocketLauncher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FireGameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.Rocket.Fire"), false);
}
