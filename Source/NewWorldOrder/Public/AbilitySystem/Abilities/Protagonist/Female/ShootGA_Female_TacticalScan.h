/**
 * 女主 Q：战术扫描（TacticalScan）
 * - 设计：范围扫描敌人施加 Marked，命中群体时延长持续，提供弱点高亮/视野提示。
 * - 当前实现：范围标记 Marked/弱点高亮占位。
 * - 缺失：群体时长翻倍、视觉提示 Cue。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"
#include "ShootGA_Female_TacticalScan.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;

UCLASS()
class NEWWORLDORDER_API UShootGA_Female_TacticalScan : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Female_TacticalScan();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="TacticalScan")
	float Radius;

	UPROPERTY(EditDefaultsOnly, Category="TacticalScan")
	float Duration;

	UPROPERTY(EditDefaultsOnly, Category="TacticalScan")
	float VulnerableBonus;

	UPROPERTY(EditDefaultsOnly, Category="TacticalScan")
	TSubclassOf<UGameplayEffect> MarkEffectClass;

	// 默认易伤 GE（用于提供机制层的易伤效果），状态标签由 Status_Vulnerable/Status_Marked 额外加到目标上（到期移除）
	UPROPERTY(EditDefaultsOnly, Category="TacticalScan")
	TSubclassOf<UGameplayEffect> VulnerableEffectClass;

private:
	void HandleScanExpired();
	FTimerHandle ScanTimerHandle;
	TArray<TWeakObjectPtr<UAbilitySystemComponent>> CachedTargetASCs;
};
