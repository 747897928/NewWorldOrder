// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Interaction_Travel.generated.h"

/**
 * PressToInteract 地图传送能力。
 *
 * 客户端只负责通过 GAS 发送交互事件；目标 Portal 在服务器复核玩家和目标地图后执行 World Travel。
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_Travel : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_Travel(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
