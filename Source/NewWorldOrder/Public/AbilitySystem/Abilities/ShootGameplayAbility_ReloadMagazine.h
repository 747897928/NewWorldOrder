// Copyright ZhaoYiJie

// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "ShootGameplayAbility.h"
#include "ShootGameplayAbility_ReloadMagazine.generated.h"

class UShootRangedWeaponInstance;
class UAnimMontage;

/**
 * UShootGameplayAbility_ReloadMagazine(禁止修改此类，AI如需修改请询问User)
 *
 * 武器换弹 Ability（参考 Lyra 的 GA_Weapon_ReloadMagazine）
 * 
 */
UCLASS()
class UShootGameplayAbility_ReloadMagazine : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGameplayAbility_ReloadMagazine();

protected:

	virtual void PostInitProperties() override;
	
	//~UGameplayAbility interface
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;
	//~End of UGameplayAbility interface

	/**
	 * 当收到换弹事件时调用（由正式 Reload Montage 上的 /Game/Characters/Heroes/Abilities/AN_Reload 触发）
	 *
	 * 这个函数执行实际的换弹逻辑：
	 * 1. 计算需要填充的弹药数量
	 * 2. 从备弹中扣除
	 * 3. 填充到弹夹
	 */
	UFUNCTION()
	void OnReloadEventReceived(FGameplayEventData Payload);

	/**
	 * 执行实际的换弹逻辑（在服务器）
	 */
	void ReloadAmmoIntoMagazine();

	/**
	 * 获取当前武器实例（从 AbilitySpec 的 SourceObject）
	 */
	UFUNCTION(BlueprintCallable, Category="Shoot|Ability")
	UShootRangedWeaponInstance* GetWeaponInstance() const;

public:
	// ========== 配置属性 ==========

	/** 角色换弹动画蒙太奇（武器动画由 AN_PlayWeaponMontage 处理） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reload|Animation")
	TObjectPtr<UAnimMontage> CharacterReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reload")
	bool DidBlockFiring = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Reload")
	float PlayRate = 1.0f;

	FGameplayTag Tag_AbilityWeaponNoFiring;
	FGameplayTag Tag_AbilityActivateFail_MagazineFull;
	FGameplayTag Tag_AbilityActivateFail_NoSpareAmmo;
	FGameplayTag Tag_GameplayEvent_ReloadDone;
private:
	/** PlayMontageAndWait 任务句柄 */
	UPROPERTY()
	TObjectPtr<class UAbilityTask_PlayMontageAndWait> MontageTask;

	/** WaitGameplayEvent 任务句柄（监听换弹事件） */
	UPROPERTY()
	TObjectPtr<class UAbilityTask_WaitGameplayEvent> WaitEventTask;
};
