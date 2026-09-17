// Copyright ZhaoYiJie
#pragma once
#include "CoreMinimal.h"
#include "ShootGameplayAbility_Weapon_Fire.h"
#include "ShootGA_Weapon_Fire_Shotgun.generated.h"

UCLASS()
class UShootGA_Weapon_Fire_Shotgun : public UShootGameplayAbility_Weapon_Fire
{
	GENERATED_BODY()

public:
	UShootGA_Weapon_Fire_Shotgun(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	// ShootGA_Weapon_Fire_Shotgun.h / ShootGA_Weapon_Fire_Sniper.h
	// 只重写 _Implementation；不要再写UFUNCTION
	virtual void OnRangedWeaponTargetDataReady_Implementation(const FGameplayAbilityTargetDataHandle& TargetData) override;

public:
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Effects")
	FGameplayTag FireGameplayCueTag;
	UPROPERTY(EditDefaultsOnly, Category="Weapon|Effects")
	FGameplayTag ImpactGameplayCueTag;

	UPROPERTY(EditAnywhere, Category="Weapon|Fire")
	float AutoRate = 1.0f;
	UPROPERTY(EditAnywhere, Category="Weapon|Fire")
	float FireDelayTimeSecs = 0.2f;

private:
	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* MontageTask = nullptr;
};
