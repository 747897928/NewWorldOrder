// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Interaction/Abilities/ShootGameplayAbility_Interact.h"
#include "ShootGA_Interaction_Revive.generated.h"

/**
 * 救援交互能力（用于倒地复活/救起）
 * - 通过 TriggerEventData 的 EventMagnitude 接受救援速度倍率（0.5 表示耗时减半）
 * - 结束时向施救者发送 GameplayEvent.Rescue.Completed
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Interaction_Revive : public UShootGameplayAbility_Interact
{
	GENERATED_BODY()

public:
	UShootGA_Interaction_Revive(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

private:
	UFUNCTION()
	void HandleRescueDelayFinished();

	// 广播救援进度时长消息（用于 UI 进度条）
	void BroadcastRescueDurationMessage(float DurationSeconds) const;

	// 基础救援耗时（秒）
	UPROPERTY(EditDefaultsOnly, Category="Rescue")
	float BaseRescueDuration;

	// 施救者与被救者缓存，完成时发送事件
	TWeakObjectPtr<const AActor> CachedInstigator;
	TWeakObjectPtr<const AActor> CachedTarget;

	// 延迟任务
	UPROPERTY()
	TObjectPtr<class UAbilityTask_WaitDelay> RescueDelayTask;
};
