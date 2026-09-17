// 治疗包执行能力：由交互链（UShootGA_Interact）动态授予，服务器权威应用即时治疗并通知拾取物消耗或重生。
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Interaction_HealthPack.generated.h"

class AShootHealthpackPickup;

UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_HealthPack : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_HealthPack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	/** 单次治疗量（SetByCaller.Heal）。 */
	UPROPERTY(EditDefaultsOnly, Category="Healthpack", meta=(ClampMin="0.0"))
	float HealAmount = 50.f;
};
