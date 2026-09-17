// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"

#include "ShootGA_DropWeapon.generated.h"

/** 玩家通过 Enhanced Input -> InputTag -> ASC 激活的服务器权威丢枪能力。 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API UShootGA_DropWeapon : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_DropWeapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
