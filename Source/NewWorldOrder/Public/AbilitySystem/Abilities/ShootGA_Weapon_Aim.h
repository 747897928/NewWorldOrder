#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Weapon_Aim.generated.h"

class AShootCharacter;
class UShootRangedWeaponInstance;

/**
 * 对齐 Lyra GA_ADS 的共享按住瞄准能力。
 *
 * AbilitySet 以当前 UShootRangedWeaponInstance 为 SourceObject 授予本能力；Started 激活、Released 结束。
 * Event.Movement.ADS 由 GAS 复制给动画蓝图，武器散布和移速在拥有端预测并由服务器执行同一能力。
 * 本地 CameraMode 与准星消息只作用于当前 LocalPlayer；狙击镜不由 Character 持有。
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Weapon_Aim : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Weapon_Aim();
	virtual void PostInitProperties() override;

protected:
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

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
	void HandleInputReleased(float TimeHeld);

private:
	UShootRangedWeaponInstance* GetWeaponInstance() const;
	void BeginAimTransition(float TargetAlpha);
	void TickAimTransition();
	void ApplyTransitionAlpha(float NewAlpha);
	void BroadcastADSMessage(bool bIsADS) const;

	TWeakObjectPtr<UShootRangedWeaponInstance> CachedWeapon;
	TWeakObjectPtr<AShootCharacter> CachedCharacter;
	FTimerHandle AimTransitionTimerHandle;
	float CurrentAimAlpha = 0.0f;
	float TargetAimAlpha = 0.0f;
	bool bControlsLocalCamera = false;
	bool bAppliedADSTag = false;
};
