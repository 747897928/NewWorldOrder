// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"

#include "ShootGA_Interaction_OpenWardrobeScreen.generated.h"

/** 衣柜是玩家私有 UI，只在 owning client 执行，不由服务器创建全局 Widget。 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_OpenWardrobeScreen : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_OpenWardrobeScreen(
		const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
