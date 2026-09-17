// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility_ReloadMagazine.h"
#include "ShootGA_Reload_Rifle.generated.h"

/**
 * 
 */
UCLASS()
class UShootGA_Reload_Rifle : public UShootGameplayAbility_ReloadMagazine
{
	GENERATED_BODY()

public:
	UShootGA_Reload_Rifle();

	virtual void PostInitProperties() override;
};
