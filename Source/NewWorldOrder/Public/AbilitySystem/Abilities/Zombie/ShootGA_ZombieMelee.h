// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "TimerManager.h"

#include "ShootGA_ZombieMelee.generated.h"

class AEnemyBotCharacter;

/**
 * Zombie 自有 ASC 上的服务器近战能力。
 *
 * BT 只按 Ability.Skill.Zombie.Melee 请求激活；真正伤害只能从 Attack Montage 的
 * UShootAnimNotify_ZombieMeleeHit 进入，并在服务器命中帧重新验证目标。
 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API UShootGA_ZombieMelee : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_ZombieMelee();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	/** 由 Zombie Attack Montage 的命中 Notify 调用；每次能力实例最多结算一次。 */
	void HandleMeleeNotify();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void ExecuteMeleeHit();
	void FinishAttackWithoutHit();

	TWeakObjectPtr<AEnemyBotCharacter> ActiveZombie;
	TWeakObjectPtr<AActor> ActiveTarget;
	FTimerHandle FinishAttackTimerHandle;
	bool bHitApplied = false;
};
