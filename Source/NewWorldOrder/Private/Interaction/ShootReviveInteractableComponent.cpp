// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/ShootReviveInteractableComponent.h"

#include "Interaction/InteractionQuery.h"
#include "Interaction/Abilities/ShootGA_Interaction_Revive.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "GameFramework/Controller.h"

#define LOCTEXT_NAMESPACE "ReviveInteractable"

UShootReviveInteractableComponent::UShootReviveInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InteractionAbilityClass = UShootGA_Interaction_Revive::StaticClass();
	TriggerMode = EShootInteractionTriggerMode::PressToInteract;
	UserFilter = EShootInteractionUserFilter::PlayerOnly;
	InteractionText = LOCTEXT("Revive", "Revive");
	InteractionSubText = LOCTEXT("HoldToInteract", "Hold the interact button");
	bReviveEnabled = false;
}

void UShootReviveInteractableComponent::SetReviveEnabled(bool bEnabled)
{
	bReviveEnabled = bEnabled;
}

void UShootReviveInteractableComponent::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
                                                                 FInteractionOptionBuilder& OptionBuilder)
{
	if (!bReviveEnabled)
	{
		return;
	}

	if (TriggerMode != EShootInteractionTriggerMode::PressToInteract)
	{
		return;
	}

	APawn* RequestingPawn = Cast<APawn>(InteractQuery.RequestingAvatar.Get());
	if (!RequestingPawn)
	{
		AController* RequestingController = InteractQuery.RequestingController.Get();
		RequestingPawn = RequestingController ? RequestingController->GetPawn() : nullptr;
	}

	if (!RequestingPawn || !CanBeTriggeredBy(RequestingPawn))
	{
		return;
	}

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.SubText = InteractionSubText;
	Option.InteractionAbilityToGrant = InteractionAbilityClass
		? InteractionAbilityClass
		: TSubclassOf<UGameplayAbility>(UShootGA_Interaction_Revive::StaticClass());

	OptionBuilder.AddInteractionOption(Option);
}

void UShootReviveInteractableComponent::CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
                                                                      FGameplayEventData& InOutEventData)
{
	// 目标默认是组件所属的 Actor（倒地目标）
	InOutEventData.Target = GetOwner();
}

bool UShootReviveInteractableComponent::CanBeTriggeredBy(const APawn* Pawn) const
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

bool UShootReviveInteractableComponent::IsPlayerActor(const APawn* Pawn) const
{
	return Pawn && Pawn->IsPlayerControlled();
}

#undef LOCTEXT_NAMESPACE
