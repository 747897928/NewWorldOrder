// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/ShootWardrobeInteractionComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/Abilities/ShootGA_Interaction_OpenWardrobeScreen.h"
#include "NativeGameplayTags.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootWardrobeInteractionComponent)

namespace ShootWardrobeInteractionTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_GAME_MENU, "UI.Layer.GameMenu");
}

UShootWardrobeInteractionComponent::UShootWardrobeInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	InteractionText = NSLOCTEXT("ShootWardrobeInteraction", "InteractionText", "Open Wardrobe");
	InteractionSubText = NSLOCTEXT("ShootWardrobeInteraction", "InteractionSubText", "Change the current character's appearance");
	InteractionAbilityClass = UShootGA_Interaction_OpenWardrobeScreen::StaticClass();
}

void UShootWardrobeInteractionComponent::GatherInteractionOptions(
	const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder)
{
	APawn* Pawn = Cast<APawn>(InteractQuery.RequestingAvatar.Get());
	if (!Pawn || !Pawn->IsPlayerControlled() || WardrobeScreenClass.IsNull() || !InteractionAbilityClass)
	{
		return;
	}
	if (AActor* Owner = GetOwner())
	{
		if (FVector::DistSquared2D(Pawn->GetActorLocation(), Owner->GetActorLocation()) > FMath::Square(InteractionRange))
		{
			return;
		}
	}

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.SubText = InteractionSubText;
	Option.InteractionAbilityToGrant = InteractionAbilityClass;
	OptionBuilder.AddInteractionOption(Option);
}

bool UShootWardrobeInteractionComponent::OpenWardrobeScreenForPawn(APawn* InstigatorPawn) const
{
	APlayerController* PlayerController = InstigatorPawn
		? Cast<APlayerController>(InstigatorPawn->GetController())
		: nullptr;
	ULocalPlayer* LocalPlayer = PlayerController && PlayerController->IsLocalController()
		? PlayerController->GetLocalPlayer()
		: nullptr;
	UPrimaryGameLayout* RootLayout = LocalPlayer
		? UPrimaryGameLayout::GetPrimaryGameLayout(LocalPlayer)
		: nullptr;
	if (!RootLayout || WardrobeScreenClass.IsNull())
	{
		return false;
	}

	RootLayout->PushWidgetToLayerStackAsync(
		ShootWardrobeInteractionTags::TAG_UI_LAYER_GAME_MENU, true, WardrobeScreenClass);
	return true;
}
