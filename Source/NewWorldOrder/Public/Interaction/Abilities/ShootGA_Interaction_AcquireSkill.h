// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"

#include "ShootGA_Interaction_AcquireSkill.generated.h"

/**
 * 通用局内技能获取交互能力。
 * 只负责把服务器权威事务转交给 IShootSkillGrantSource，不知道来源是拾取物、商人还是 Round 奖励。
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_AcquireSkill : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_AcquireSkill(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
