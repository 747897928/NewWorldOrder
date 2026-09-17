/**
 * 女主被动：智能辅助（SmartAssist）
 * - 设计：召唤/绑定无人机跟随，自动射击/标记或自爆，配置生命/伤害/爆炸半径。
 * - 当前实现：无人机 Buff 占位。
 * - 缺失：跟随/目标选择/输出/自爆逻辑（计划 Phase 4 完成）。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "ShootGA_Passive_SmartAssist.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API UShootGA_Passive_SmartAssist : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Passive_SmartAssist();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="SmartAssist")
	TSubclassOf<UGameplayEffect> DroneBuffEffectClass;

	/** 机器人正式实现前的占位 Buff 也必须跟随 AbilitySet 生命周期撤销。 */
	FActiveGameplayEffectHandle DroneBuffHandle;
};
