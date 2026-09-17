// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootSquadListItem.h"

#include "CommonSessionSubsystem.h"
#include "Engine/AssetManager.h"
#include "GameModes/LyraUserFacingExperienceDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootSquadListItem)

#define LOCTEXT_NAMESPACE "ShootSquadListItem"

void UShootSquadListItem::InitializeSquadResult(UCommonSession_SearchResult* InSearchResult)
{
	SearchResult = InSearchResult;
	SetEntryObject(InSearchResult);

	if (!SearchResult)
	{
		BP_OnSquadResultChanged(false, LOCTEXT("UnknownExpedition", "Unknown Expedition"),
			FString(), 0, 0, 0);
		return;
	}

	FString AdvertisedExperience;
	FString MapName;
	bool bFoundExperience = false;
	bool bFoundMap = false;
	SearchResult->GetStringSetting(TEXT("GAMEMODE"), AdvertisedExperience, bFoundExperience);
	SearchResult->GetStringSetting(TEXT("MAPNAME"), MapName, bFoundMap);

	FText ExpeditionName = bFoundExperience
		? FText::FromString(AdvertisedExperience)
		: LOCTEXT("UnknownExpedition", "Unknown Expedition");
	if (bFoundExperience)
	{
		const FPrimaryAssetId DefinitionId(
			TEXT("LyraUserFacingExperienceDefinition"), FName(*AdvertisedExperience));
		const FSoftObjectPath DefinitionPath = UAssetManager::Get().GetPrimaryAssetPath(DefinitionId);
		if (const ULyraUserFacingExperienceDefinition* Definition =
			Cast<ULyraUserFacingExperienceDefinition>(DefinitionPath.TryLoad()))
		{
			ExpeditionName = Definition->TileTitle;
		}
	}

	const int32 MaxPlayers = SearchResult->GetMaxPublicConnections();
	const int32 CurrentPlayers = FMath::Max(
		0, MaxPlayers - SearchResult->GetNumOpenPublicConnections());
	BP_OnSquadResultChanged(
		SearchResult->GetNumOpenPublicConnections() > 0,
		ExpeditionName,
		bFoundMap ? MapName : FString(),
		CurrentPlayers,
		MaxPlayers,
		SearchResult->GetPingInMs());
}

#undef LOCTEXT_NAMESPACE
