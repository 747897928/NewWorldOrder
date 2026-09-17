// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootSessionBrowserEntry.h"

#include "CommonSessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"

void UShootSessionBrowserEntry::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	// 接口默认实现负责继续触发 Widget Blueprint 的 OnListItemObjectSet 图表；
	// UCommonUserWidget 本身不是该接口的父实现，不能通过 Super 调用。
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	SearchResult = Cast<UCommonSession_SearchResult>(ListItemObject);
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

void UShootSessionBrowserEntry::JoinStoredSession()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!SearchResult || !PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	if (UShootSessionCoordinatorSubsystem* Coordinator =
		GetGameInstance()->GetSubsystem<UShootSessionCoordinatorSubsystem>())
	{
		Coordinator->JoinSession(PlayerController, SearchResult);
	}
}
