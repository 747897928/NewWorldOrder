// Copyright ZhaoYiJie

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_Weapon_AutoReload.generated.h"

class UShootRangedWeaponInstance;
class UAbilitySystemComponent;

/**
 * 对齐 Lyra GA_Weapon_AutoReload 的本地 OnSpawn 被动能力。
 *
 * AbilitySet 以当前 UShootRangedWeaponInstance 作为 SourceObject 授予本能力；本能力从它读取弹药，
 * 条件满足后只激活同一 SourceObject 的项目正式弹匣式或逐发式 Reload AbilitySpec。
 * 它不读取 QuickBar、Character 或武器 Actor，也不伪造键盘/手柄输入状态。
 * 弹药仍以 Inventory ItemInstance 的 StatTags 为唯一数据源，实际装填仍由 Reload GA 在服务器结算。
 * 弹匣式武器和 Shotgun_A 共用本能力；本能力只负责在空仓且有备弹时激活同一 SourceObject 的
 * 正式 Reload GA。Shotgun_A 的 InsertShell、循环装填和火力打断仍完全由逐发装填 GA 负责。
 */
UCLASS()
class NEWWORLDORDER_API UShootGA_Weapon_AutoReload : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_Weapon_AutoReload();
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

	/** 与 Lyra 默认值一致：首次和后续检查都间隔 0.25 秒。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Auto Reload", meta=(ClampMin="0.05", ForceUnits=s))
	float PollInterval = 0.25f;

	/** 与 Lyra 默认值一致：刚装备或开火后等待 0.66 秒，避免打空瞬间抢占最后一帧表现。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Auto Reload", meta=(ClampMin="0.0", ForceUnits=s))
	float TimeSinceActivityToReload = 0.66f;

private:
	void CheckAutoReload();
	bool TryActivateReloadAbility(
		UAbilitySystemComponent& AbilitySystemComponent,
		const UShootRangedWeaponInstance& Weapon) const;
	UShootRangedWeaponInstance* GetWeaponInstance() const;

	FTimerHandle PollTimerHandle;
};
