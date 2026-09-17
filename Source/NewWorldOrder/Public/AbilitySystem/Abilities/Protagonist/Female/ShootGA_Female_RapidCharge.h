/**
 * 女主 C：疾速充能（RapidCharge）
 * - 设计：移速/换弹提升，击杀刷新持续，降低 ADS 移动惩罚，连杀越多持续越久。
 * - 当前实现：移速/换弹 Buff，击杀刷新。
 * - 缺失：ADS 减速调整、精英奖励等 kill-chain 细节。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"
#include "ShootGA_Female_RapidCharge.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API UShootGA_Female_RapidCharge : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Female_RapidCharge();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="RapidCharge")
	TSubclassOf<UGameplayEffect> MoveSpeedBuffClass;

	UPROPERTY(EditDefaultsOnly, Category="RapidCharge")
	TSubclassOf<UGameplayEffect> ReloadBuffClass;

	UPROPERTY(EditDefaultsOnly, Category="RapidCharge")
	float BuffDuration;

	UPROPERTY(EditDefaultsOnly, Category="RapidCharge")
	float MoveSpeedBonusLv1;

	UPROPERTY(EditDefaultsOnly, Category="RapidCharge")
	float MoveSpeedBonusLv2;

	UPROPERTY(EditDefaultsOnly, Category="RapidCharge")
	float ReloadSpeedBonusLv3;

	UPROPERTY(EditDefaultsOnly, Category="RapidCharge")
	float KillExtendSecondsLv1;

	UPROPERTY(EditDefaultsOnly, Category="RapidCharge")
	float KillExtendSecondsLv3;

private:
	void HandleDurationExpired();
	void RefreshBuffsAndDuration();
	void ApplyBuffsAndDuration(float NewDuration);

	FActiveGameplayEffectHandle MoveSpeedBuffHandle;
	FActiveGameplayEffectHandle ReloadBuffHandle;
	FTimerHandle DurationTimerHandle;
	bool bKillDelegateBound = false;

	void HandleKill(AActor* Killer, AActor* Victim);
};
