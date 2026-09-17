// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"

#include "ShootGA_QuickbarCycle.generated.h"

/**
 * 副本内切换 RuntimeOnly 武器槽的通用 GAS 入口。
 * 方向由蓝图子类配置；InputAction 与具体设备映射保持在 InputConfig/IMC，不写入 C++。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootGA_QuickbarCycle : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_QuickbarCycle(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

protected:
	/** true 为下一把，false 为上一把；分别由 GA 蓝图默认值配置。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quickbar")
	bool bCycleForward = true;
};
