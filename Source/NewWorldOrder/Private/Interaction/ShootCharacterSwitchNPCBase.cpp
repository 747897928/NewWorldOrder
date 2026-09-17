// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/ShootCharacterSwitchNPCBase.h"

#include "Abilities/GameplayAbility.h"
#include "GameFramework/Controller.h"
#include "Interaction/Abilities/ShootGA_WorldCharacterSwitchRequest.h"
#include "Player/ShootPlayerState.h"

#define LOCTEXT_NAMESPACE "CharacterSwitchNPC"

AShootCharacterSwitchNPCBase::AShootCharacterSwitchNPCBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionAbilityClass = UShootGA_WorldCharacterSwitchRequest::StaticClass();
}

void AShootCharacterSwitchNPCBase::GatherInteractionOptions(const FInteractionQuery& InteractQuery,
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

void AShootCharacterSwitchNPCBase::CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
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
		// EventMagnitude 只是兜底透传；真正执行时服务端仍会重新调用入口接口解析目标性别。
		InOutEventData.EventMagnitude = (TargetGender == ECharacterGender::MALE) ? 0.f : 1.f;
	}
}

bool AShootCharacterSwitchNPCBase::ResolveSwitchTargetGender(const APawn* RequestingPawn, ECharacterGender& OutTargetGender) const
{
	OutTargetGender = ECharacterGender::UNKNOWN;
	if (!RequestingPawn || RepresentedGender == ECharacterGender::UNKNOWN)
	{
		return false;
	}

	const ECharacterGender RequestingGender = ResolveRequesterCurrentGender(RequestingPawn);
	if (RequestingGender == ECharacterGender::UNKNOWN)
	{
		return false;
	}

	// 另一位主角 NPC 入口的默认语义是“切到我代表的主角”。
	// 如果当前已经是同一主角，就不再暴露交互选项，避免世界入口出现无意义提示。
	if (RequestingGender == RepresentedGender)
	{
		return false;
	}

	OutTargetGender = RepresentedGender;
	return true;
}

FText AShootCharacterSwitchNPCBase::BuildDefaultInteractionText(ECharacterGender TargetGender) const
{
	const FText TargetText = (TargetGender == ECharacterGender::MALE)
		                         ? LOCTEXT("MaleProtagonist", "Male Protagonist")
		                         : LOCTEXT("FemaleProtagonist", "Female Protagonist");
	return FText::Format(LOCTEXT("SwitchToProtagonist", "Switch to {0}"), TargetText);
}

FText AShootCharacterSwitchNPCBase::BuildDefaultInteractionSubText() const
{
	return LOCTEXT("SwitchToOtherProtagonist", "Interact with the other protagonist to switch characters");
}

bool AShootCharacterSwitchNPCBase::CanBeTriggeredBy(const APawn* Pawn) const
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

bool AShootCharacterSwitchNPCBase::IsPlayerActor(const APawn* Pawn) const
{
	return Pawn && Pawn->IsPlayerControlled();
}

ECharacterGender AShootCharacterSwitchNPCBase::ResolveRequesterCurrentGender(const APawn* RequestingPawn) const
{
	if (!RequestingPawn)
	{
		return ECharacterGender::UNKNOWN;
	}

	const AShootPlayerState* ShootPS = RequestingPawn->GetPlayerState<AShootPlayerState>();
	if (!ShootPS)
	{
		if (const AController* RequestingController = RequestingPawn->GetController())
		{
			ShootPS = RequestingController->GetPlayerState<AShootPlayerState>();
		}
	}

	return ShootPS ? ShootPS->GetCharacterGender() : ECharacterGender::UNKNOWN;
}

#undef LOCTEXT_NAMESPACE
