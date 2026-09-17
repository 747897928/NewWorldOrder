/**
 * 男主 X：战术超载（TacticalOverload）
 * - 设计：爆发窗口，提升移速/射速/换弹，标记 Overload 状态，期间无限弹药；击杀可延长持续时间。
 * - 当前实现：Overload 状态会让 UShootAbilityCost_AmmoTagStack 跳过检查与消耗，因此有限时间内无限弹药已接通。
 * - 当前冷却：充能系统落地前使用 30 秒临时冷却；释放时发送服务器权威 GameplayCue。
 * - 缺失：击杀延长仍属于后续内容，不在当前四槽基础任务中伪装为已完成。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Male_TacticalOverload.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API UShootGA_Male_TacticalOverload : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Male_TacticalOverload();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// Buff GE：配置移动/射速/换弹等加成与持续时间
	UPROPERTY(EditDefaultsOnly, Category="TacticalOverload|Buff")
	TSubclassOf<UGameplayEffect> OverloadBuffEffect;

	// 未配置 Buff GE 或 GE 没有有效持续时间时仍必须自动结束，避免无限弹药状态永久残留。
	UPROPERTY(EditDefaultsOnly, Category="TacticalOverload|Duration", meta=(ClampMin="0.1"))
	float BaseDuration = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="TacticalOverload|Duration", meta=(ClampMin="0.0"))
	float DurationPerLevel = 2.f;

	UPROPERTY(EditDefaultsOnly, Category="TacticalOverload|Presentation", meta=(Categories="GameplayCue"))
	FGameplayTag ActivationGameplayCueTag;

	/** 持续环绕角色的 Cue；GA 激活时 Add，任意结束路径 Remove。 */
	UPROPERTY(EditDefaultsOnly, Category="TacticalOverload|Presentation", meta=(Categories="GameplayCue"))
	FGameplayTag ActiveGameplayCueTag;

private:
	FTimerHandle OverloadEndTimerHandle;
	FActiveGameplayEffectHandle ActiveOverloadEffectHandle;
	bool bActiveGameplayCueAdded = false;
	void HandleOverloadFinished(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bCancelled);
};
