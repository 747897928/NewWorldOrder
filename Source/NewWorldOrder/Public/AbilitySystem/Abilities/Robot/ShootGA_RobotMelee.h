// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"

#include "ShootGA_RobotMelee.generated.h"

class AShootRobotCompanionCharacter;

/** Robot 自己 ASC 上的近战 GA；Claw/Chomp Montage Notify 决定服务器命中时刻。 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API UShootGA_RobotMelee : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_RobotMelee();

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

	/** Montage 漏配 Notify 时的安全兜底；正式伤害帧仍由 UShootAnimNotify_RobotMelee 驱动。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Melee", meta=(ClampMin="0.01", Units="s"))
	float MissingNotifyFallbackDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Melee", meta=(Categories="GameplayCue"))
	FGameplayTag MeleeGameplayCueTag;

private:
	void HandleMeleeNotify();
	void ExecuteImpactAndFinish();
	bool ValidateTarget(const AShootRobotCompanionCharacter& Robot, const AActor& Target) const;

	TWeakObjectPtr<AShootRobotCompanionCharacter> ActiveRobot;
	TWeakObjectPtr<AActor> ActiveTarget;
	FTimerHandle MissingNotifyTimerHandle;
	mutable double NextAllowedMeleeTime = -DBL_MAX;
	bool bUseChomp = false;
	bool bImpactExecuted = false;
};
