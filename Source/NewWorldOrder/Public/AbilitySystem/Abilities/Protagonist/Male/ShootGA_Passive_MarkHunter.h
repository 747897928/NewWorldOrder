/**
 * 男主被动：猎杀标记（MarkHunter）
 * - 设计：击杀回复生命，精英有额外收益，并向周围敌人扩散 Marked/易伤。
 * - 当前实现：监听击杀并按阈值回血。
 * - 缺失：Mark/易伤扩散逻辑、精英/群体的差异效果。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Passive_MarkHunter.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API UShootGA_Passive_MarkHunter : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Passive_MarkHunter();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 大于该阈值视为精英回血比例
	UPROPERTY(EditDefaultsOnly, Category="MarkHunter")
	float EliteHealthThreshold;

	// 达到该等级后解锁“击杀普通敌人回血”
	UPROPERTY(EditDefaultsOnly, Category="MarkHunter")
	int32 NormalHealMinLevel;

	UPROPERTY(EditDefaultsOnly, Category="MarkHunter")
	float HealPercentElite;

	UPROPERTY(EditDefaultsOnly, Category="MarkHunter")
	float HealPercentNormal;

	// 标记传播基础半径（Lv3 会应用额外倍率）
	UPROPERTY(EditDefaultsOnly, Category="MarkHunter|Spread")
	float SpreadRadius;

	// 标记传播持续时间（用于应用 Status.Marked）
	UPROPERTY(EditDefaultsOnly, Category="MarkHunter|Spread")
	float SpreadDuration;

	// Lv3 标记传播半径倍率（+50% = 1.5）
	UPROPERTY(EditDefaultsOnly, Category="MarkHunter|Spread")
	float SpreadRadiusMultiplierLv3;

	// 是否要求被击杀目标已有标记（配合女主战术扫描）
	UPROPERTY(EditDefaultsOnly, Category="MarkHunter|Spread")
	bool bSpreadRequiresVictimMarked;

	// 扩散时施加的标记 GE
	UPROPERTY(EditDefaultsOnly, Category="MarkHunter|Spread")
	TSubclassOf<UGameplayEffect> SpreadMarkEffectClass;

private:
	
	FDelegateHandle KillDelegateHandle;
	
	UFUNCTION()
	void HandleKill(AActor* Killer, AActor* Victim);
};
