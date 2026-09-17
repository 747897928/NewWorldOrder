/**
 * 女主 E：救援隐匿（RescueCloak）
 * - 设计：施加隐身/不可选目标，提升移速，救援/复活速度加成，救起后短暂无敌/高 DR。
 * - 当前实现：隐身/移速 Buff 占位。
 * - 缺失：复活速度/保护、队友范围隐身、AI 感知屏蔽 Cue。
 */
#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "TimerManager.h"
#include "ShootGA_Female_RescueCloak.generated.h"

class UGameplayEffect;

UCLASS()
class NEWWORLDORDER_API UShootGA_Female_RescueCloak : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Female_RescueCloak();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, Category="RescueCloak")
	TSubclassOf<UGameplayEffect> MoveSpeedBuffClass;

	// 进入救援隐匿时添加的状态 Tag（用于 AI 感知/UI 查询）；默认使用 FShootGameplayTags::Status_Cloaked
	UPROPERTY(EditDefaultsOnly, Category="RescueCloak")
	FGameplayTag CloakStatusTag;

	UPROPERTY(EditDefaultsOnly, Category="RescueCloak")
	float CloakDuration;

	UPROPERTY(EditDefaultsOnly, Category="RescueCloak")
	float MoveSpeedBonus;

	// 救起后治疗比例（MaxHP 百分比）
	UPROPERTY(EditDefaultsOnly, Category="RescueCloak|Revive")
	float ReviveHealPercent;

	// 救起后免疫致死持续时间
	UPROPERTY(EditDefaultsOnly, Category="RescueCloak|Revive")
	float ReviveImmuneDeathDuration;

	// 救起后治疗用的 GE（默认使用 HealInstant）
	UPROPERTY(EditDefaultsOnly, Category="RescueCloak|Revive")
	TSubclassOf<UGameplayEffect> ReviveHealEffectClass;

	// 救起后保护用的 GE（默认使用 ImmuneDeath）
	UPROPERTY(EditDefaultsOnly, Category="RescueCloak|Revive")
	TSubclassOf<UGameplayEffect> ReviveProtectionEffectClass;

	// 救援速度倍率（2.0 表示救援时间减半）
	UPROPERTY(EditDefaultsOnly, Category="RescueCloak|Revive")
	float RescueSpeedMultiplier;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void HandleCloakExpired();
	void HandleWeaponFireTagChanged(const FGameplayTag Tag, int32 NewCount);
	void HandleRescueCompleted(const FGameplayEventData* Payload);
	void BindRescueEvent(UAbilitySystemComponent* ASC);
	void UnbindRescueEvent(UAbilitySystemComponent* ASC);

	FTimerHandle CloakTimerHandle;
	FActiveGameplayEffectHandle MoveSpeedBuffHandle;
	FDelegateHandle WeaponFireTagHandle;
	FDelegateHandle RescueCompletedHandle;
	FGameplayTag WeaponFireTag;
	FGameplayTag RescueCompletedTag;
	FGameplayTag RescueSpeedStatusTag;
};
