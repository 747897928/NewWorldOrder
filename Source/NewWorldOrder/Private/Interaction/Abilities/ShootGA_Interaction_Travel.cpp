// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_Interaction_Travel.h"

#include "Interaction/ShootMapTravelPortal.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Interaction_Travel)

UShootGA_Interaction_Travel::UShootGA_Interaction_Travel(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UShootGA_Interaction_Travel::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	APawn* InstigatorPawn = TriggerEventData
		? const_cast<APawn*>(Cast<APawn>(TriggerEventData->Instigator.Get()))
		: nullptr;
	if (!InstigatorPawn && ActorInfo)
	{
		InstigatorPawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
	}

	AShootMapTravelPortal* Portal = TriggerEventData
		? const_cast<AShootMapTravelPortal*>(Cast<AShootMapTravelPortal>(TriggerEventData->Target.Get()))
		: nullptr;

	const bool bIsAuthority = ActorInfo && ActorInfo->IsNetAuthority();
	const bool bTraveled = bIsAuthority && Portal && Portal->HandleTravel(InstigatorPawn);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bTraveled && bIsAuthority);
}
