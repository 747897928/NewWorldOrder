// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/ShootExpeditionTerminal.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/Abilities/ShootGA_Interaction_OpenExpeditionScreen.h"
#include "Interaction/ShootLocalInteractionPrompt.h"
#include "NativeGameplayTags.h"
#include "PrimaryGameLayout.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootExpeditionTerminal)

namespace ShootExpeditionTerminalTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_LAYER_GAME_MENU, "UI.Layer.GameMenu");
}

AShootExpeditionTerminal::AShootExpeditionTerminal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(160.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("Interactable_OverlapDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(true);

	VisualComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	VisualComponent->SetupAttachment(CollisionComponent);
	VisualComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionPrompt = CreateDefaultSubobject<UShootLocalInteractionPrompt>(TEXT("InteractionPrompt"));
	InteractionPrompt->SetupAttachment(CollisionComponent);

	InteractionText = NSLOCTEXT("ShootExpeditionTerminal", "InteractionText", "Select Expedition");
	InteractionSubText = NSLOCTEXT("ShootExpeditionTerminal", "InteractionSubText", "Play locally or create an online session");
	InteractionAbilityClass = UShootGA_Interaction_OpenExpeditionScreen::StaticClass();
}

void AShootExpeditionTerminal::GatherInteractionOptions(
	const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder)
{
	APawn* Pawn = Cast<APawn>(InteractQuery.RequestingAvatar.Get());
	if (!Pawn || !Pawn->IsPlayerControlled() || ExpeditionScreenClass.IsNull())
	{
		return;
	}
	if (CollisionComponent &&
		FVector::DistSquared2D(Pawn->GetActorLocation(), GetActorLocation()) >
		FMath::Square(CollisionComponent->GetScaledSphereRadius()))
	{
		return;
	}

	FInteractionOption Option;
	Option.Text = InteractionText;
	Option.SubText = InteractionSubText;
	Option.InteractionAbilityToGrant = InteractionAbilityClass;
	OptionBuilder.AddInteractionOption(Option);
}

void AShootExpeditionTerminal::CustomizeInteractionEventData(
	const FGameplayTag& InteractionEventTag, FGameplayEventData& InOutEventData)
{
	InOutEventData.Target = this;
}

bool AShootExpeditionTerminal::OpenExpeditionScreenForPawn(APawn* InstigatorPawn)
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
	if (!RootLayout || ExpeditionScreenClass.IsNull())
	{
		return false;
	}

	RootLayout->PushWidgetToLayerStackAsync(
		ShootExpeditionTerminalTags::TAG_UI_LAYER_GAME_MENU, true, ExpeditionScreenClass);
	return true;
}
