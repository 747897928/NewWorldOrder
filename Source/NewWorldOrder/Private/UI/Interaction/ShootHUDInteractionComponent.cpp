// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Interaction/ShootHUDInteractionComponent.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "ShootGameplayTags.h"
#include "UI/Interaction/ShootInteractionProgressWidget.h"
#include "UIExtensionSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootHUDInteractionComponent)

UShootHUDInteractionComponent::UShootHUDInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UShootHUDInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	CachedPlayerController = Cast<APlayerController>(GetOwner());
	if (!TryRegisterWidget())
	{
		SetComponentTickEnabled(true);
	}
}

void UShootHUDInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (TryRegisterWidget())
	{
		SetComponentTickEnabled(false);
	}
}

bool UShootHUDInteractionComponent::TryRegisterWidget()
{
	if (ExtensionHandle.IsValid())
	{
		return true;
	}
	if (!CachedPlayerController.IsValid() || !CachedPlayerController->IsLocalController())
	{
		return false;
	}

	ULocalPlayer* LocalPlayer = CachedPlayerController->GetLocalPlayer();
	UUIExtensionSubsystem* ExtensionSubsystem = GetWorld()->GetSubsystem<UUIExtensionSubsystem>();
	if (LocalPlayer && ExtensionSubsystem && InteractionProgressWidgetClass)
	{
		ExtensionHandle = ExtensionSubsystem->RegisterExtensionAsWidgetForContext(
			FShootGameplayTags::Get().HUD_Slot_Interaction,
			LocalPlayer,
			InteractionProgressWidgetClass,
			20);
	}
	return ExtensionHandle.IsValid();
}

void UShootHUDInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ExtensionHandle.IsValid())
	{
		ExtensionHandle.Unregister();
	}
	CachedPlayerController.Reset();
	Super::EndPlay(EndPlayReason);
}
