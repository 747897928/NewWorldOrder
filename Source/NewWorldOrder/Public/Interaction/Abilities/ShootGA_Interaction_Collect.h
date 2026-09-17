// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Interaction_Collect.generated.h"

class AShootResourcePickup;
class AShootInventoryGrantActor;
class AShootWeaponPickupActor;
class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;

/**
 * 拾取能力（等价 Lyra GA_Interaction_Collect）
 * - 由交互能力通过 GameplayEvent 激活
 * - 负责播放拾取动画/提示
 * - 服务器修改 ResourceInventory / InventoryManager
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_Collect : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_Collect(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

protected:
	void HandleResourcePickup(AShootResourcePickup* PickupActor, APawn* InstigatorPawn);

	void HandleWeaponPickup(AShootWeaponPickupActor* PickupActor, APawn* InstigatorPawn);

	void HandleInventoryGrantActor(AShootInventoryGrantActor* GrantActor, APawn* InstigatorPawn);

	void ApplyFactionEffects(AShootResourcePickup* PickupActor, UAbilitySystemComponent* InstigatorASC);

	void PlayPickupMontage(APawn* InstigatorPawn);

	UFUNCTION()
	void OnPickupMontageFinished();

protected:
	/** 拾取动画，可选 */
	UPROPERTY(EditDefaultsOnly, Category="Pickup")
	TObjectPtr<UAnimMontage> PickupMontage;

	/** 播放的 GameplayCue（音效/特效） */
	UPROPERTY(EditDefaultsOnly, Category="Pickup")
	FGameplayTag PickupCueTag;

	/** 禁用拾取物碰撞的延迟，避免重复触发 */
	UPROPERTY(EditDefaultsOnly, Category="Pickup")
	float PickupActorLifeSpan = 1.0f;

private:
	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	UPROPERTY()
	TWeakObjectPtr<AActor> CachedPickupActor;
};
