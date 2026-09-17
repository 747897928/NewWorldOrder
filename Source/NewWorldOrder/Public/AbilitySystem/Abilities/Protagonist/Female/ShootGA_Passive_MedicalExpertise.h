/**
 * 女主被动：医疗专精（MedicalExpertise）
 * - 设计：提升对友军的治疗输出/受益，为被救起目标提供短暂不死窗口。
 * - 当前实现：治疗输出/受益 Buff 占位。
 * - 缺失：拦截治疗事件放大系数、不死窗口逻辑。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "ShootGA_Passive_MedicalExpertise.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API UShootGA_Passive_MedicalExpertise : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Passive_MedicalExpertise();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly, Category="MedicalExpertise")
	TSubclassOf<UGameplayEffect> HealingBuffEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="MedicalExpertise")
	float HealingDoneBonus;

	UPROPERTY(EditDefaultsOnly, Category="MedicalExpertise")
	float HealingReceivedBonus;

	/** 被动自身施加的无限 GE 句柄；AbilitySet 取回能力时由 EndAbility 精确撤销。 */
	FActiveGameplayEffectHandle HealingBuffHandle;
};
