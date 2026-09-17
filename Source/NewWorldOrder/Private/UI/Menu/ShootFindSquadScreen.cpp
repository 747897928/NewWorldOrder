// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootFindSquadScreen.h"

#include "CommonSessionSubsystem.h"
#include "Components/DynamicEntryBox.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "UI/Foundation/ShootObjectEntryButtonBase.h"
#include "UI/Menu/ShootSquadListItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootFindSquadScreen)

#define LOCTEXT_NAMESPACE "ShootFindSquadScreen"

void UShootFindSquadScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	Coordinator = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UShootSessionCoordinatorSubsystem>()
		: nullptr;
	if (Coordinator)
	{
		Coordinator->OnCoordinatorChanged.AddUObject(this, &ThisClass::RefreshFromCoordinator);
	}
	RefreshSquads();
}

void UShootFindSquadScreen::NativeOnDeactivated()
{
	if (Coordinator)
	{
		Coordinator->OnCoordinatorChanged.RemoveAll(this);
	}
	Coordinator = nullptr;
	SelectedResult = nullptr;
	SquadEntryBox = nullptr;
	SquadEntryClass = nullptr;
	Super::NativeOnDeactivated();
}

bool UShootFindSquadScreen::NativeOnHandleBackAction()
{
	CloseScreen();
	return true;
}

void UShootFindSquadScreen::PopulateSquadEntries(
	UDynamicEntryBox* EntryBox,
	TSubclassOf<UShootSquadListItem> EntryWidgetClass)
{
	SquadEntryBox = EntryBox;
	SquadEntryClass = EntryWidgetClass;
	if (!SquadEntryClass && SquadEntryBox)
	{
		UClass* ConfiguredClass = SquadEntryBox->GetEntryWidgetClass();
		if (ConfiguredClass && ConfiguredClass->IsChildOf(UShootSquadListItem::StaticClass()))
		{
			SquadEntryClass = ConfiguredClass;
		}
	}
	RebuildSquadEntries();
}

void UShootFindSquadScreen::RefreshSquads()
{
	if (!Coordinator)
	{
		BP_OnSquadSearchChanged(
			EShootSessionLifecycleState::Error,
			LOCTEXT("CoordinatorMissing", "Online services are unavailable."), 0, false);
		return;
	}

	SelectedResult = nullptr;
	Coordinator->FindSessions(GetOwningSessionPlayer(), ECommonSessionOnlineMode::Online);
	RefreshFromCoordinator();
}

bool UShootFindSquadScreen::JoinSelectedSquad()
{
	return Coordinator && CanJoinSelectedSquad() &&
		Coordinator->JoinSession(GetOwningSessionPlayer(), SelectedResult);
}

void UShootFindSquadScreen::CloseScreen()
{
	DeactivateWidget();
}

bool UShootFindSquadScreen::CanJoinSelectedSquad() const
{
	return SelectedResult && SelectedResult->GetNumOpenPublicConnections() > 0;
}

void UShootFindSquadScreen::RefreshFromCoordinator()
{
	if (!Coordinator)
	{
		return;
	}

	if (SelectedResult && !Coordinator->GetSearchResults().Contains(SelectedResult))
	{
		SelectedResult = nullptr;
	}
	RebuildSquadEntries();
	BP_OnSquadSearchChanged(
		Coordinator->GetState(), Coordinator->GetStatusText(),
		Coordinator->GetSearchResults().Num(), CanJoinSelectedSquad());
}

void UShootFindSquadScreen::RebuildSquadEntries()
{
	if (!SquadEntryBox || !SquadEntryClass)
	{
		return;
	}

	SquadEntryBox->Reset(false);
	if (Coordinator)
	{
		for (UCommonSession_SearchResult* SearchResult : Coordinator->GetSearchResults())
		{
			if (UShootSquadListItem* Entry =
				SquadEntryBox->CreateEntry<UShootSquadListItem>(SquadEntryClass))
			{
				Entry->OnEntryClicked().RemoveAll(this);
				Entry->OnEntryHovered().RemoveAll(this);
				Entry->OnEntryClicked().AddUObject(this, &ThisClass::HandleEntryClicked);
				Entry->OnEntryHovered().AddUObject(this, &ThisClass::HandleEntryHovered);
				Entry->InitializeSquadResult(SearchResult);
			}
		}
	}
	RefreshEntrySelection();
}

void UShootFindSquadScreen::HandleEntryClicked(UShootObjectEntryButtonBase*, UObject* EntryObject)
{
	SelectedResult = Cast<UCommonSession_SearchResult>(EntryObject);
	RefreshEntrySelection();
	if (Coordinator)
	{
		BP_OnSquadSearchChanged(
			Coordinator->GetState(), Coordinator->GetStatusText(),
			Coordinator->GetSearchResults().Num(), CanJoinSelectedSquad());
	}
}

void UShootFindSquadScreen::HandleEntryHovered(UShootObjectEntryButtonBase*, UObject* EntryObject)
{
	SelectedResult = Cast<UCommonSession_SearchResult>(EntryObject);
	RefreshEntrySelection();
	if (Coordinator)
	{
		BP_OnSquadSearchChanged(
			Coordinator->GetState(), Coordinator->GetStatusText(),
			Coordinator->GetSearchResults().Num(), CanJoinSelectedSquad());
	}
}

void UShootFindSquadScreen::RefreshEntrySelection()
{
	if (!SquadEntryBox)
	{
		return;
	}

	for (UUserWidget* Widget : SquadEntryBox->GetAllEntries())
	{
		if (UShootSquadListItem* Entry = Cast<UShootSquadListItem>(Widget))
		{
			if (Entry->GetSearchResult() == SelectedResult)
			{
				Entry->SetIsSelected(true);
			}
			else
			{
				Entry->ClearSelection();
			}
		}
	}
}

APlayerController* UShootFindSquadScreen::GetOwningSessionPlayer() const
{
	APlayerController* PlayerController = GetOwningPlayer();
	return PlayerController && PlayerController->IsLocalController() ? PlayerController : nullptr;
}

#undef LOCTEXT_NAMESPACE
