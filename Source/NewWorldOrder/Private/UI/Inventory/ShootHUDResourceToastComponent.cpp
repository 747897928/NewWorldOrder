// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Inventory/ShootHUDResourceToastComponent.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "ShootGameplayTags.h"
#include "UI/Inventory/ShootResourceToastWidgetBase.h"

UShootHUDResourceToastComponent::UShootHUDResourceToastComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
	SetIsReplicated(false);
}

void UShootHUDResourceToastComponent::BeginPlay()
{
	Super::BeginPlay();

	CachedPlayerController = Cast<APlayerController>(GetOwner());
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		CachedMessageSubsystem = &UGameplayMessageSubsystem::Get(this);
	}

	if (CachedMessageSubsystem.IsValid())
	{
		const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
		ResourceToastHandle = CachedMessageSubsystem->RegisterListener<FResourceChangedMessage>(
			GameplayTags.UI_Toast_ResourcePickup, this, &ThisClass::HandleResourceToast);
	}

	CreateToastWidget();
}

void UShootHUDResourceToastComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CachedMessageSubsystem.IsValid() && ResourceToastHandle.IsValid())
	{
		CachedMessageSubsystem->UnregisterListener(ResourceToastHandle);
	}

	DestroyToastWidget();

	Super::EndPlay(EndPlayReason);
}

void UShootHUDResourceToastComponent::HandleResourceToast(FGameplayTag Channel, const FResourceChangedMessage& Message)
{
	if (!IsLocalPlayerController() || Message.Delta <= 0)
	{
		// 仅提示正向增长（拾取/制作奖励）
		return;
	}

	if (!ResourceToastWidget.IsValid())
	{
		CreateToastWidget();
		if (!ResourceToastWidget.IsValid())
		{
			return;
		}
	}

	// 过滤非本地账号的消息：ResourceInventory 永远挂在 PlayerState
	const APlayerController* PC = CachedPlayerController.Get();
	const APlayerState* MessageState = Cast<APlayerState>(Message.Owner);
	const APlayerState* LocalPlayerState = PC ? PC->PlayerState : nullptr;
	if (MessageState && MessageState != LocalPlayerState)
	{
		return;
	}

	ResourceToastWidget->PushResourceToast(Message);
}

void UShootHUDResourceToastComponent::CreateToastWidget()
{
	if (ResourceToastWidget.IsValid() || !ResourceToastWidgetClass || !CachedPlayerController.IsValid())
	{
		return;
	}

	if (UShootResourceToastWidgetBase* Widget = CreateWidget<UShootResourceToastWidgetBase>(
		CachedPlayerController.Get(), ResourceToastWidgetClass))
	{
		Widget->AddToViewport(ResourceToastZOrder);
		Widget->ActivateWidget();
		ResourceToastWidget = Widget;
	}
}

void UShootHUDResourceToastComponent::DestroyToastWidget()
{
	if (ResourceToastWidget.IsValid())
	{
		if (ResourceToastWidget->IsInViewport())
		{
			ResourceToastWidget->RemoveFromParent();
		}
		ResourceToastWidget->DeactivateWidget();
		ResourceToastWidget.Reset();
	}
}

bool UShootHUDResourceToastComponent::IsLocalPlayerController() const
{
	const APlayerController* PC = CachedPlayerController.Get();
	return PC && PC->IsLocalController();
}
