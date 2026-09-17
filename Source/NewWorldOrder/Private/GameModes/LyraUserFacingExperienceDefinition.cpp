// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameModes/LyraUserFacingExperienceDefinition.h"
#include "CommonSessionSubsystem.h"
#include "Containers/UnrealString.h"
#include "UObject/NameTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Replays/LyraReplaySubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraUserFacingExperienceDefinition)

UCommonSession_HostSessionRequest* ULyraUserFacingExperienceDefinition::CreateHostingRequest(const UObject* WorldContextObject) const
{
	return CreateHostingRequestWithOptions(WorldContextObject, ECommonSessionOnlineMode::Online,
		MaxPlayerCount, 1, true, false);
}

UCommonSession_HostSessionRequest* ULyraUserFacingExperienceDefinition::CreateHostingRequestWithOptions(
	const UObject* WorldContextObject, ECommonSessionOnlineMode OnlineMode, int32 RequestedMaxPlayers,
	int32 LocalPlayerCount, bool bAllowJoinInProgress, bool bFillEmptySlotsWithBots) const
{
	// 分屏专用地图不能被在线入口绕过限制；UI 会同步回退到 Local，这里保留最终防线。
	if (OnlineMode == ECommonSessionOnlineMode::Online && !bSupportsOnline)
	{
		return nullptr;
	}

	const FString ExperienceName = ExperienceID.PrimaryAssetName.ToString();
	const FString UserFacingExperienceName = GetPrimaryAssetId().PrimaryAssetName.ToString();

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	UCommonSession_HostSessionRequest* Result = nullptr;

	if (UCommonSessionSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UCommonSessionSubsystem>() : nullptr)
	{
		Result = Subsystem->CreateOnlineHostSessionRequest();
	}

	if (!Result)
	{
		// Couldn't use the subsystem so create one
		Result = NewObject<UCommonSession_HostSessionRequest>();
		Result->OnlineMode = OnlineMode;
		Result->bUseLobbies = OnlineMode == ECommonSessionOnlineMode::Online;
		Result->bUseLobbiesVoiceChat = false;
		// We always enable presence on this session because it is the primary session used for matchmaking. For online systems that care about presence, only the primary session should have presence enabled
		Result->bUsePresence = !IsRunningDedicatedServer();
	}
	Result->OnlineMode = OnlineMode;
	Result->bUseLobbies = OnlineMode == ECommonSessionOnlineMode::Online;
	Result->bUsePresence = OnlineMode == ECommonSessionOnlineMode::Online && !IsRunningDedicatedServer();
	Result->MapID = OnlineMode == ECommonSessionOnlineMode::Online && LobbyMapID.IsValid()
		? LobbyMapID
		: MapID;
	Result->ModeNameForAdvertisement = UserFacingExperienceName;
	Result->ExtraArgs = ExtraArgs;
	Result->MaxPlayerCount = FMath::Clamp(RequestedMaxPlayers, 1, FMath::Max(1, MaxPlayerCount));

	if (OnlineMode == ECommonSessionOnlineMode::Online && LobbyMapID.IsValid())
	{
		// FURL 会拒绝选项值里的冒号，不能把 "Map:/Game/..." 或
		// "ShootExperienceDefinition:DA_..." 直接写进 ServerTravel URL。类型由 Lobby GameMode
		// 按约定补回，URL 只传 PrimaryAssetName；Data Asset 内仍保留完整 PrimaryAssetId。
		Result->ExtraArgs.Add(TEXT("ExpeditionMap"), MapID.PrimaryAssetName.ToString());
		Result->ExtraArgs.Add(TEXT("ExpeditionExperience"), ExperienceID.PrimaryAssetName.ToString());
		Result->ExtraArgs.Add(TEXT("AllowJoinInProgress"), bAllowJoinInProgress ? TEXT("1") : TEXT("0"));
		Result->ExtraArgs.Add(TEXT("FillBots"), bFillEmptySlotsWithBots ? TEXT("1") : TEXT("0"));
	}
	else
	{
		Result->ExtraArgs.Add(TEXT("Experience"), ExperienceName);
		Result->ExtraArgs.Add(TEXT("LocalPlayers"), FString::FromInt(FMath::Clamp(LocalPlayerCount, 1, 2)));
	}

	if (ULyraReplaySubsystem::DoesPlatformSupportReplays())
	{
		if (bRecordReplay)
		{
			Result->ExtraArgs.Add(TEXT("DemoRec"), FString());
		}
	}

	return Result;
}
