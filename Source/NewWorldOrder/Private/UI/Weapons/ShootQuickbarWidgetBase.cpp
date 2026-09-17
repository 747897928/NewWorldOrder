// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Weapons/ShootQuickbarWidgetBase.h"

#include "Equipment/ShootQuickBarComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Player/ShootPlayerController.h"
#include "ShootGameplayTags.h"
#include "UI/Weapons/ShootQuickbarSlotWidgetBase.h"
#include "UI/Weapons/ShootWeaponAmmoWidgetBase.h"

UShootQuickbarWidgetBase::UShootQuickbarWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootQuickbarWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	RegisterQuickbarListeners();
	RequestQuickbarRefresh();
}

void UShootQuickbarWidgetBase::NativeDestruct()
{
	UnregisterQuickbarListeners();
	Super::NativeDestruct();
}

void UShootQuickbarWidgetBase::RequestQuickbarRefresh()
{
	QuickbarSlots.Reset();
	ActiveSlotIndex = INDEX_NONE;

	if (UShootQuickBarComponent* QuickBar = GetQuickBarComponent())
	{
		QuickBar->GetQuickbarSlotsData(QuickbarSlots);
		ActiveSlotIndex = QuickBar->GetActiveSlotIndex();
	}

	HandleQuickbarSlotsUpdated();
	HandleQuickbarActiveIndexUpdated(ActiveSlotIndex);
}

void UShootQuickbarWidgetBase::HandleQuickbarSlotsUpdated_Implementation()
{
	RefreshNativeWidgets();
}

void UShootQuickbarWidgetBase::HandleQuickbarActiveIndexUpdated_Implementation(int32 NewIndex)
{
	ActiveSlotIndex = NewIndex;
	RefreshNativeWidgets();
}

void UShootQuickbarWidgetBase::RefreshNativeWidgets()
{
	UShootQuickbarSlotWidgetBase* SlotWidgets[] = { WeaponSlot1, WeaponSlot2, WeaponSlot3 };
	for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(SlotWidgets); ++SlotIndex)
	{
		if (UShootQuickbarSlotWidgetBase* SlotWidget = SlotWidgets[SlotIndex])
		{
			if (QuickbarSlots.IsValidIndex(SlotIndex))
			{
				SlotWidget->SetSlotData(QuickbarSlots[SlotIndex], SlotIndex, ActiveSlotIndex == SlotIndex);
			}
			else
			{
				SlotWidget->ClearSlot(SlotIndex, ActiveSlotIndex == SlotIndex);
			}
		}
	}

	if (WeaponAmmoAndName)
	{
		WeaponAmmoAndName->SetActiveWeaponData(
			QuickbarSlots.IsValidIndex(ActiveSlotIndex) ? &QuickbarSlots[ActiveSlotIndex] : nullptr);
	}
}

void UShootQuickbarWidgetBase::RegisterQuickbarListeners()
{
	if (SlotsChangedHandle.IsValid())
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
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

	SlotsChangedHandle = MessageSubsystem.RegisterListener<FQuickbarSlotsChangedMessage>(
		Tags.Msg_Quickbar_SlotsChanged, this, &ThisClass::HandleSlotsChanged);
	ActiveIndexHandle = MessageSubsystem.RegisterListener<FQuickbarActiveIndexChangedMessage>(
		Tags.Msg_Quickbar_ActiveIndexChanged, this, &ThisClass::HandleActiveIndexChanged);
	AmmoChangedHandle = MessageSubsystem.RegisterListener<FWeaponAmmoChangedMessage>(
		Tags.Msg_Weapon_AmmoChanged, this, &ThisClass::HandleWeaponAmmoChanged);
}

void UShootQuickbarWidgetBase::UnregisterQuickbarListeners()
{
	if (!UGameplayMessageSubsystem::HasInstance(this))
	{
		return;
	}

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	if (SlotsChangedHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(SlotsChangedHandle);
		SlotsChangedHandle = FGameplayMessageListenerHandle();
	}
	if (ActiveIndexHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(ActiveIndexHandle);
		ActiveIndexHandle = FGameplayMessageListenerHandle();
	}
	if (AmmoChangedHandle.IsValid())
	{
		MessageSubsystem.UnregisterListener(AmmoChangedHandle);
		AmmoChangedHandle = FGameplayMessageListenerHandle();
	}
}

void UShootQuickbarWidgetBase::HandleSlotsChanged(FGameplayTag, const FQuickbarSlotsChangedMessage& Message)
{
	APawn* OwnerPawn = GetOwningPlayerPawnChecked();
	if (!OwnerPawn || Message.Owner.Get() != OwnerPawn)
	{
		return;
	}

	QuickbarSlots = Message.Slots;
	HandleQuickbarSlotsUpdated();
}

void UShootQuickbarWidgetBase::HandleActiveIndexChanged(FGameplayTag, const FQuickbarActiveIndexChangedMessage& Message)
{
	APawn* OwnerPawn = GetOwningPlayerPawnChecked();
	if (!OwnerPawn || Message.Owner.Get() != OwnerPawn)
	{
		return;
	}

	ActiveSlotIndex = Message.ActiveIndex;
	HandleQuickbarActiveIndexUpdated(ActiveSlotIndex);
}

void UShootQuickbarWidgetBase::HandleWeaponAmmoChanged(FGameplayTag, const FWeaponAmmoChangedMessage& Message)
{
	APawn* OwnerPawn = GetOwningPlayerPawnChecked();
	if (!OwnerPawn || Message.Owner.Get() != OwnerPawn)
	{
		return;
	}

	bool bUpdated = false;
	for (FQuickbarSlotData& SlotData : QuickbarSlots)
	{
		if (SlotData.WeaponId == Message.WeaponId)
		{
			if (SlotData.Ammo != Message.Ammo || SlotData.Reserve != Message.Reserve)
			{
				SlotData.Ammo = Message.Ammo;
				SlotData.Reserve = Message.Reserve;
				bUpdated = true;
			}
		}
	}

	if (bUpdated)
	{
		HandleQuickbarSlotsUpdated();
	}
}

APawn* UShootQuickbarWidgetBase::GetOwningPlayerPawnChecked() const
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		return PC->GetPawn();
	}
	return nullptr;
}

UShootQuickBarComponent* UShootQuickbarWidgetBase::GetQuickBarComponent() const
{
	if (const AShootPlayerController* ShootPlayerController = Cast<AShootPlayerController>(GetOwningPlayer()))
	{
		return ShootPlayerController->GetGameplayQuickBarComponent();
	}

	return nullptr;
}
