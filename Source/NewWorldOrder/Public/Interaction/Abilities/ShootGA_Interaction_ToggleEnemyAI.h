// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Interaction_ToggleEnemyAI.generated.h"

/** TestMap AI 开关的交互执行能力；客户预测输入，服务器唯一修改状态。 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_ToggleEnemyAI : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_ToggleEnemyAI(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
