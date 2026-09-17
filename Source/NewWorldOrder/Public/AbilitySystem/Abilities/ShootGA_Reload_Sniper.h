// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility_ReloadMagazine.h"
#include "ShootGA_Reload_Sniper.generated.h"

/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Reload_Sniper : public UShootGameplayAbility_ReloadMagazine
{
	GENERATED_BODY()

public:
	virtual void PostInitProperties() override;
};
