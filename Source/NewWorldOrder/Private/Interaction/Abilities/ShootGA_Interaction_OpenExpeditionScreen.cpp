// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_Interaction_OpenExpeditionScreen.h"

#include "GameFramework/Pawn.h"
#include "Interaction/ShootExpeditionTerminal.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Interaction_OpenExpeditionScreen)

UShootGA_Interaction_OpenExpeditionScreen::UShootGA_Interaction_OpenExpeditionScreen(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

void UShootGA_Interaction_OpenExpeditionScreen::ActivateAbility(
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
	AShootExpeditionTerminal* Terminal = TriggerEventData
		? const_cast<AShootExpeditionTerminal*>(Cast<AShootExpeditionTerminal>(TriggerEventData->Target.Get()))
		: nullptr;
	const bool bOpened = Terminal && Terminal->OpenExpeditionScreenForPawn(Pawn);
	EndAbility(Handle, ActorInfo, ActivationInfo, true, !bOpened);
}
