// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootSessionResultButton.h"

#include "CommonSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"

void UShootSessionResultButton::InitializeSearchResult(UCommonSession_SearchResult* InSearchResult)
{
	SearchResult = InSearchResult;
	if (!SearchResult)
	{
		BP_OnSearchResultDataChanged(false, false, FString(), FString(), 0, 0, 0);
		return;
	}

	FString GameMode;
	FString MapName;
	bool bFoundGameMode = false;
	bool bFoundMapName = false;
	SearchResult->GetStringSetting(TEXT("GAMEMODE"), GameMode, bFoundGameMode);
	SearchResult->GetStringSetting(TEXT("MAPNAME"), MapName, bFoundMapName);
	const int32 MaxPlayers = SearchResult->GetMaxPublicConnections();
	const int32 CurrentPlayers = FMath::Max(0, MaxPlayers - SearchResult->GetNumOpenPublicConnections());

	BP_OnSearchResultDataChanged(true, SearchResult->GetNumOpenPublicConnections() > 0,
		bFoundGameMode ? GameMode : FString(), bFoundMapName ? MapName : FString(),
		CurrentPlayers, MaxPlayers, SearchResult->GetPingInMs());
}

void UShootSessionResultButton::JoinStoredSession()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (SearchResult && PlayerController && PlayerController->IsLocalController())
	{
		if (UShootSessionCoordinatorSubsystem* Coordinator =
			GetGameInstance()->GetSubsystem<UShootSessionCoordinatorSubsystem>())
		{
			Coordinator->JoinSession(PlayerController, SearchResult);
		}
	}
}
