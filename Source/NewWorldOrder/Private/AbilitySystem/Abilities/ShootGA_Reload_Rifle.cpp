// Copyright ZhaoYiJie


#include "AbilitySystem/Abilities/ShootGA_Reload_Rifle.h"

UShootGA_Reload_Rifle::UShootGA_Reload_Rifle()
{
	
}

void UShootGA_Reload_Rifle::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		//AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.Reload")));
		SetAssetTags(FGameplayTagContainer(
			FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.Reload"))));
		ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.Movement.Reload")));
		StartupInputTag = FGameplayTag::RequestGameplayTag(FName("InputTag.R"));
	}
}
