// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"

#include "ShootGA_RobotFire.generated.h"

class AShootRobotCompanionCharacter;

/** Robot 自己 ASC 上的 ServerOnly 射击 GA；Montage Notify 决定权威命中时刻。 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API UShootGA_RobotFire : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_RobotFire();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Montage 忘记放 Notify 时的安全兜底，不取代 AnimNotify 正式命中链。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Fire", meta=(ClampMin="0.01", Units="s"))
	float MissingNotifyFallbackDelay = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Fire", meta=(Categories="GameplayCue"))
	FGameplayTag FireGameplayCueTag;

private:
	void HandleFireNotify();
	void ExecuteShotAndFinish();
	bool ValidateTarget(const AShootRobotCompanionCharacter& Robot, const AActor& Target) const;

	TWeakObjectPtr<AShootRobotCompanionCharacter> ActiveRobot;
	TWeakObjectPtr<AActor> ActiveTarget;
	TWeakObjectPtr<USceneComponent> ActiveMuzzle;
	FTimerHandle MissingNotifyTimerHandle;
	mutable double NextAllowedFireTime = -DBL_MAX;
	bool bShotExecuted = false;
};
