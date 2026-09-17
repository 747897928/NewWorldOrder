// Copyright ZhaoYiJie


#include "AbilitySystem/Abilities/LyraGameplayAbility_Jump.h"

#include "Character/ShootCharacter.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameplayAbility_Jump)

struct FGameplayTagContainer;


ULyraGameplayAbility_Jump::ULyraGameplayAbility_Jump(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

bool ULyraGameplayAbility_Jump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	const ACharacter* LyraCharacter = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!LyraCharacter || !LyraCharacter->CanJump())
	{
		return false;
	}

	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	return true;
}

void ULyraGameplayAbility_Jump::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Stop jumping in case the ability blueprint doesn't call it.
	CharacterJumpStop();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULyraGameplayAbility_Jump::CharacterJumpStart()
{
	if (AShootCharacter* ShootCharacter = GetShootCharacterFromActorInfo())
	{
		if (ShootCharacter->IsLocallyControlled() && !ShootCharacter->bPressedJump)
		{
			ShootCharacter->UnCrouch();
			ShootCharacter->Jump();
		}
	}
}

void ULyraGameplayAbility_Jump::CharacterJumpStop()
{
	if (AShootCharacter* ShootCharacter = GetShootCharacterFromActorInfo())
	{
		if (ShootCharacter->IsLocallyControlled() && ShootCharacter->bPressedJump)
		{
			ShootCharacter->StopJumping();
		}
	}
}
