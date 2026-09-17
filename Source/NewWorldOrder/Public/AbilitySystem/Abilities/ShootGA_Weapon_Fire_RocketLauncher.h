// Copyright ZhaoYiJie

// AbilitySystem/Abilities/ShootGA_Weapon_Fire_RocketLauncher.h
#pragma once
#include "CoreMinimal.h"
#include "ShootGA_Weapon_Fire_Projectile.h"
#include "ShootGA_Weapon_Fire_RocketLauncher.generated.h"

UCLASS()
class UShootGA_Weapon_Fire_RocketLauncher : public UShootGA_Weapon_Fire_Projectile
{
	GENERATED_BODY()
public:
	UShootGA_Weapon_Fire_RocketLauncher(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
