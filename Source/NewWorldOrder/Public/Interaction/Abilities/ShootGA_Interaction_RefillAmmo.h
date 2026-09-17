// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Interaction_RefillAmmo.generated.h"

/** 读条完成后的补给执行能力。客户端只预测激活，弹药仅在服务器权威分支写入。 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_RefillAmmo : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_RefillAmmo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
