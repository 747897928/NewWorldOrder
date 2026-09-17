#pragma once

#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "ShootGA_ToggleCameraPerspective.generated.h"

/** Experience 授予的本地视角切换能力；实际键位只配在 Enhanced Input 资产。 */
UCLASS()
class NEWWORLDORDER_API UShootGA_ToggleCameraPerspective : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGA_ToggleCameraPerspective();

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
};
