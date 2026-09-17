// Copyright ZhaoYiJie


#include "AbilitySystem/Abilities/ShootGA_Reload_Sniper.h"

void UShootGA_Reload_Sniper::PostInitProperties()
{
	Super::PostInitProperties();
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.Reload"))));
		ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Movement.Reload")));
		StartupInputTag = FGameplayTag::RequestGameplayTag(FName("InputTag.R"));
	}
}
