// Copyright ZhaoYiJie

// AbilitySystem/Abilities/ShootGA_Weapon_Fire_GrenadeLauncher.cpp
#include "AbilitySystem/Abilities/ShootGA_Weapon_Fire_GrenadeLauncher.h"

UShootGA_Weapon_Fire_GrenadeLauncher::UShootGA_Weapon_Fire_GrenadeLauncher(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FireGameplayCueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Weapon.GrenadeLauncher.Fire"), false);
}
