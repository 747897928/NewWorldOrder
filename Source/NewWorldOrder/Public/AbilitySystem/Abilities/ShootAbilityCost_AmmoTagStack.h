// 弹药成本：基于 ItemTagStack，一次开火固定消耗一个弹药单位。
#pragma once

#include "AbilitySystem/Abilities/ShootAbilityCost_ItemTagStack.h"
#include "ShootAbilityCost_AmmoTagStack.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootAbilityCost_AmmoTagStack : public UShootAbilityCost_ItemTagStack
{
	GENERATED_BODY()

public:
	UShootAbilityCost_AmmoTagStack();

	//~UShootAbilityCost interface
	virtual bool CheckCost(const UShootGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle,
	                       const FGameplayAbilityActorInfo* ActorInfo,
	                       FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void ApplyCost(const UShootGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle,
	                       const FGameplayAbilityActorInfo* ActorInfo,
	                       const FGameplayAbilityActivationInfo ActivationInfo) override;
	//~End of UShootAbilityCost interface

private:
	void ConfigureFixedAmmoCost() const;
};
