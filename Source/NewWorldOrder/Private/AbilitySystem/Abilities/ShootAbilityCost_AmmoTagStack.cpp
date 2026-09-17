// 弹药成本：基于 ItemTagStack，每次开火固定消耗一个弹药单位。
#include "AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.h"

#include "ShootGameplayTags.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "AbilitySystemComponent.h"

UShootAbilityCost_AmmoTagStack::UShootAbilityCost_AmmoTagStack()
{
	// Shotgun 的多弹丸由 BulletsPerCartridge 表达，不增加一次开火的弹药成本。
	Quantity.SetValue(1.f);
}

void UShootAbilityCost_AmmoTagStack::ConfigureFixedAmmoCost() const
{
	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	if (!Tag.IsValid())
	{
		const_cast<UShootAbilityCost_AmmoTagStack*>(this)->Tag = Tags.Inventory_Ammo_Magazine;
	}
}

bool UShootAbilityCost_AmmoTagStack::CheckCost(const UShootGameplayAbility* Ability,
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Ability || !ActorInfo)
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const FShootGameplayTags& Tags = FShootGameplayTags::Get();

	// 过载状态下跳过弹药检查，允许开火
	if (ASC && ASC->HasMatchingGameplayTag(Tags.Status_Overload))
	{
		return true;
	}

	ConfigureFixedAmmoCost();
	return Super::CheckCost(Ability, Handle, ActorInfo, OptionalRelevantTags);
}

void UShootAbilityCost_AmmoTagStack::ApplyCost(const UShootGameplayAbility* Ability,
	const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!Ability || !ActorInfo)
	{
		return;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const FShootGameplayTags& Tags = FShootGameplayTags::Get();

	// 过载状态下不扣减弹药 Tag，实现无限弹药
	if (ASC && ASC->HasMatchingGameplayTag(Tags.Status_Overload))
	{
		return;
	}

	ConfigureFixedAmmoCost();
	Super::ApplyCost(Ability, Handle, ActorInfo, ActivationInfo);
}
