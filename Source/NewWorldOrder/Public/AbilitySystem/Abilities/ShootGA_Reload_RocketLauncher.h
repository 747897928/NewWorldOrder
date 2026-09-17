// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility_ReloadMagazine.h"
#include "ShootGA_Reload_RocketLauncher.generated.h"

/**
 * 火箭筒专用整弹匣换弹。
 *
 * 结算仍沿用通用 ReloadMagazine 的 GameplayEvent.ReloadDone 语义；唯一的
 * 特殊行为是在蒙太奇开始时通知 Rocket WeaponInstance 生成并挂接可见弹头，
 * 因而该弹头能完整跟随武器 Ammo 骨骼的“手 -> 发射器”动画。
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Reload_RocketLauncher : public UShootGameplayAbility_ReloadMagazine
{
	GENERATED_BODY()

public:
	UShootGA_Reload_RocketLauncher();

	/** CDO 阶段不能读取尚未填充的 FShootGameplayTags 单例。 */
	virtual void PostInitProperties() override;

protected:
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

	UFUNCTION()
	void OnRocketInsertEvent(FGameplayEventData Payload);

private:
	/** 角色 ReloadRocket 上的专用通知，在弹头进入发射器的时刻转挂 AmmoSocket。 */
	FGameplayTag RocketInsertEventTag;

	UPROPERTY()
	TObjectPtr<class UAbilityTask_WaitGameplayEvent> RocketInsertEventTask;
};
