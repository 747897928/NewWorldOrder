#include "AbilitySystem/Abilities/ShootGA_ToggleCameraPerspective.h"

#include "Camera/ShootCameraModeStackComponent.h"
#include "Character/ShootCharacter.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_ToggleCameraPerspective)

UShootGA_ToggleCameraPerspective::UShootGA_ToggleCameraPerspective()
{
	ActivationPolicy = EShootAbilityActivationPolicy::OnInputTriggered;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UShootGA_ToggleCameraPerspective::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo)
	{
		return false;
	}

	const AShootCharacter* Character = ActorInfo
		? Cast<AShootCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	const UShootCameraModeStackComponent* CameraModes = Character
		? Character->FindComponentByClass<UShootCameraModeStackComponent>()
		: nullptr;
	return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)
		&& ActorInfo->IsLocallyControlled()
		&& CameraModes
		&& CameraModes->CanTogglePerspective();
}

void UShootGA_ToggleCameraPerspective::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AShootCharacter* Character = ActorInfo
		? Cast<AShootCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	if (UShootCameraModeStackComponent* CameraModes = Character
		? Character->FindComponentByClass<UShootCameraModeStackComponent>()
		: nullptr)
	{
		CameraModes->TogglePerspective();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
