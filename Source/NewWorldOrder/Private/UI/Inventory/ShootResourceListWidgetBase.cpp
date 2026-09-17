// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Inventory/ShootResourceListWidgetBase.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/ResourceInventoryBlueprintLibrary.h"
#include "Player/ShootPlayerState.h"
#include "ShootGameplayTags.h"

UShootResourceListWidgetBase::UShootResourceListWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootResourceListWidgetBase::NativeOnActivated()
{
	Super::NativeOnActivated();
	RegisterMessageListener();
	RefreshResourceEntries();
}

void UShootResourceListWidgetBase::NativeOnDeactivated()
{
	UnregisterMessageListener();
	Super::NativeOnDeactivated();
}

void UShootResourceListWidgetBase::RefreshResourceEntries()
{
	ResourceEntries.Reset();

	const APlayerController* PC = GetOwningPlayer();
	const UObject* WorldContext = PC ? Cast<UObject>(PC->PlayerState) : nullptr;
	if (!WorldContext)
	{
		return;
	}

	if (const UResourceInventoryComponent* ResourceInventory = UResourceInventoryBlueprintLibrary::GetResourceInventory(WorldContext))
	{
		ResourceInventory->GetAllResources(ResourceEntries);
	}

	HandleResourceEntriesUpdated();
}

void UShootResourceListWidgetBase::RegisterMessageListener()
{
	if (ResourceListHandle.IsValid())
	{
		return;
	}

	const APlayerController* PC = GetOwningPlayer();
	if (!PC || !PC->IsLocalPlayerController())
	{
		return;
	}

	if (!UGameplayMessageSubsystem::HasInstance(this))
	{
		return;
	}

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	ResourceListHandle = MessageSubsystem.RegisterListener<FResourceChangedMessage>(
		Tags.Inventory_Resource_Message_Changed, this, &ThisClass::HandleResourceChanged);
}

void UShootResourceListWidgetBase::UnregisterMessageListener()
{
	if (!ResourceListHandle.IsValid())
	{
		return;
	}

	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(ResourceListHandle);
	}
	ResourceListHandle = FGameplayMessageListenerHandle();
}

void UShootResourceListWidgetBase::HandleResourceChanged(FGameplayTag Channel, const FResourceChangedMessage& Message)
{
	// HUD/背包 UI 只关心本地账号数据
	const APlayerController* PC = GetOwningPlayer();
	const AShootPlayerState* LocalPlayerState = PC ? PC->GetPlayerState<AShootPlayerState>() : nullptr;
	const AShootPlayerState* MessageState = Cast<AShootPlayerState>(Message.Owner);
	if (!LocalPlayerState || MessageState != LocalPlayerState)
	{
		return;
	}

	// 根据消息更新 ResourceEntries
	bool bFound = false;
	for (FResourceEntry& Entry : ResourceEntries)
	{
		if (Entry.ItemDef == Message.ItemDef)
		{
			Entry.Count = Message.NewCount;
			bFound = true;
			break;
		}
	}

	if (!bFound && Message.ItemDef)
	{
		FResourceEntry& NewEntry = ResourceEntries.AddDefaulted_GetRef();
		NewEntry.ItemDef = Message.ItemDef;
		NewEntry.Count = Message.NewCount;
	}

	HandleResourceEntriesUpdated();
}
