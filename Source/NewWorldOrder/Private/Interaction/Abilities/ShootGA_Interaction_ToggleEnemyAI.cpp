// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_Interaction_ToggleEnemyAI.h"

#include "GameFramework/Pawn.h"
#include "Testing/ShootEnemyAIPauseSwitch.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Interaction_ToggleEnemyAI)

UShootGA_Interaction_ToggleEnemyAI::UShootGA_Interaction_ToggleEnemyAI(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UShootGA_Interaction_ToggleEnemyAI::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	APawn* InstigatorPawn = TriggerEventData
		? const_cast<APawn*>(Cast<APawn>(TriggerEventData->Instigator.Get()))
		: nullptr;
	AShootEnemyAIPauseSwitch* PauseSwitch = TriggerEventData
		? const_cast<AShootEnemyAIPauseSwitch*>(Cast<AShootEnemyAIPauseSwitch>(TriggerEventData->Target.Get()))
		: nullptr;

	if (ActorInfo && ActorInfo->IsNetAuthority() && PauseSwitch
		&& PauseSwitch->CanToggleForPawn(InstigatorPawn))
	{
		PauseSwitch->ToggleEnemyBehavior();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
