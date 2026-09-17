// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"

#include "ShootGA_Interaction_OpenExpeditionScreen.generated.h"

/** 玩家私有 UI 行为必须只在 owning client 执行，不能让服务器替某个分屏玩家创建全局 Widget。 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_OpenExpeditionScreen : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_OpenExpeditionScreen(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
