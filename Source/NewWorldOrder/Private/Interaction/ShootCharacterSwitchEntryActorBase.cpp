// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/ShootCharacterSwitchEntryActorBase.h"

#include "Abilities/GameplayAbility.h"
#include "GameFramework/Controller.h"
#include "Interaction/Abilities/ShootGA_WorldCharacterSwitchRequest.h"

#define LOCTEXT_NAMESPACE "CharacterSwitchEntry"

AShootCharacterSwitchEntryActorBase::AShootCharacterSwitchEntryActorBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	InteractionAbilityClass = UShootGA_WorldCharacterSwitchRequest::StaticClass();
}

void AShootCharacterSwitchEntryActorBase::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
                                                                   FInteractionOptionBuilder& OptionBuilder)
{
	if (TriggerMode != EShootInteractionTriggerMode::PressToInteract)
	{
		return;
	}

	APawn* RequestingPawn = Cast<APawn>(InteractQuery.RequestingAvatar.Get());
	if (!RequestingPawn)
	{
		const AController* RequestingController = InteractQuery.RequestingController.Get();
		RequestingPawn = RequestingController ? RequestingController->GetPawn() : nullptr;
	}

	ECharacterGender TargetGender = ECharacterGender::UNKNOWN;
	if (!RequestingPawn || !CanBeTriggeredBy(RequestingPawn) || !ResolveSwitchTargetGender(RequestingPawn, TargetGender))
	{
		return;
	}

	FInteractionOption Option;
	Option.Text = !InteractionText.IsEmpty() ? InteractionText : BuildDefaultInteractionText(TargetGender);
	Option.SubText = !InteractionSubText.IsEmpty() ? InteractionSubText : BuildDefaultInteractionSubText();
	Option.InteractionAbilityToGrant = InteractionAbilityClass
		                                   ? InteractionAbilityClass
		                                   : TSubclassOf<UGameplayAbility>(UShootGA_WorldCharacterSwitchRequest::StaticClass());

	OptionBuilder.AddInteractionOption(Option);
}

void AShootCharacterSwitchEntryActorBase::CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
                                                                        FGameplayEventData& InOutEventData)
{
	InOutEventData.Target = this;

	const APawn* RequestingPawn = Cast<APawn>(InOutEventData.Instigator.Get());
	if (!RequestingPawn)
	{
		if (const AController* RequestingController = Cast<AController>(InOutEventData.Instigator.Get()))
		{
			RequestingPawn = RequestingController->GetPawn();
		}
	}

	ECharacterGender TargetGender = ECharacterGender::UNKNOWN;
	if (ResolveSwitchTargetGender(RequestingPawn, TargetGender))
	{
		// EventMagnitude 只做跨层透传，服务端仍会按入口接口再次解析目标性别。
		InOutEventData.EventMagnitude = (TargetGender == ECharacterGender::MALE) ? 0.f : 1.f;
	}
}

bool AShootCharacterSwitchEntryActorBase::ResolveSwitchTargetGender(const APawn* RequestingPawn, ECharacterGender& OutTargetGender) const
{
	// 这个基类只负责共性骨架，不知道具体应该切到谁。
	// 派生类若忘记覆写，这里直接返回失败，避免把错误目标送进切换后端。
	OutTargetGender = ECharacterGender::UNKNOWN;
	return false;
}

FText AShootCharacterSwitchEntryActorBase::BuildDefaultInteractionText(ECharacterGender TargetGender) const
{
	const FText TargetText = (TargetGender == ECharacterGender::MALE)
		                         ? LOCTEXT("MaleProtagonist", "Male Protagonist")
		                         : LOCTEXT("FemaleProtagonist", "Female Protagonist");
	return FText::Format(LOCTEXT("SwitchToProtagonist", "Switch to {0}"), TargetText);
}

FText AShootCharacterSwitchEntryActorBase::BuildDefaultInteractionSubText() const
{
	return LOCTEXT("SwitchAndSaveLoadout", "Switch after interacting and save the current character configuration");
}

bool AShootCharacterSwitchEntryActorBase::CanBeTriggeredBy(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return false;
	}

	const bool bIsPlayerControlled = IsPlayerActor(Pawn);
	switch (UserFilter)
	{
	case EShootInteractionUserFilter::PlayerOnly:
		return bIsPlayerControlled;
	case EShootInteractionUserFilter::AIOnly:
		return !bIsPlayerControlled;
	case EShootInteractionUserFilter::PlayerAndAI:
	default:
		return true;
	}
}

bool AShootCharacterSwitchEntryActorBase::IsPlayerActor(const APawn* Pawn) const
{
	return Pawn && Pawn->IsPlayerControlled();
}

#undef LOCTEXT_NAMESPACE
