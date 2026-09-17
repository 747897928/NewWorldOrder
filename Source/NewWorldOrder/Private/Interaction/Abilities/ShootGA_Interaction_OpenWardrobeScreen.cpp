// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_Interaction_OpenWardrobeScreen.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Interaction/ShootWardrobeInteractionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Interaction_OpenWardrobeScreen)

UShootGA_Interaction_OpenWardrobeScreen::UShootGA_Interaction_OpenWardrobeScreen(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UShootGA_Interaction_OpenWardrobeScreen::ActivateAbility(
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

	APawn* Pawn = ActorInfo ? Cast<APawn>(ActorInfo->AvatarActor.Get()) : nullptr;
	AActor* TargetActor = TriggerEventData
		? const_cast<AActor*>(Cast<AActor>(TriggerEventData->Target.Get()))
		: nullptr;
	UShootWardrobeInteractionComponent* WardrobeInteraction = TargetActor
		? TargetActor->FindComponentByClass<UShootWardrobeInteractionComponent>()
		: nullptr;
	const bool bOpened = WardrobeInteraction && WardrobeInteraction->OpenWardrobeScreenForPawn(Pawn);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bOpened);
}
