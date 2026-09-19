// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "CommonSessionSubsystem.h"
#include "GameplayTagContainer.h"

#include "LyraUserFacingExperienceDefinition.generated.h"

class FString;
class UCommonSession_HostSessionRequest;
class UObject;
class UTexture2D;
class UUserWidget;
struct FFrame;

/** Description of settings used to display experiences in the UI and start a new session */
UCLASS(BlueprintType)
class ULyraUserFacingExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** The specific map to load */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience, meta=(AllowedTypes="Map"))
	FPrimaryAssetId MapID;

	/** Online 创建房间时先进入的等待大厅；Offline/本地分屏直接使用 MapID。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience, meta=(AllowedTypes="Map"))
	FPrimaryAssetId LobbyMapID;

	/** The gameplay experience to load */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience, meta=(AllowedTypes="ShootExperienceDefinition"))
	FPrimaryAssetId ExperienceID;

	/** Extra arguments passed as URL options to the game */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	TMap<FString, FString> ExtraArgs;

	/** Primary title in the UI */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileTitle;

	/** Secondary title */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileSubTitle;

	/** Full description */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	FText TileDescription;

	/** 目录卡片与详情 Hero 使用的主图；具体裁切方式由 Widget Blueprint 决定。 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	TObjectPtr<UTexture2D> TileIcon;

	/** The loading screen widget to show when loading into (or back out of) a given experience */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=LoadingScreen)
	TSoftClassPtr<UUserWidget> LoadingScreenWidget;

	/** If true, this is a default experience that should be used for quick play and given priority in the UI */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bIsDefaultExperience = false;

	/** If true, this will show up in the experiences list in the front-end */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bShowInFrontEnd = true;

	/**
	 * 该可启动 Experience 支持哪些游玩方式。
	 *
	 * Single Player、Local Co-op、Online Co-op 描述玩家如何参与，不是大灾变、生化感染等
	 * 玩法模式。一个 UFE 可以支持多种参与方式，选择页只过滤同一份目录。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience,
		meta=(Categories="Experience.Participation"))
	FGameplayTagContainer SupportedParticipationModes;

	/**
	 * 该 Experience 使用的玩法规则，例如大灾变或生化感染。
	 * 地图、玩法模式和 Experience 是三层概念：同名地图的不同玩法变体应使用独立 UFE，
	 * 它们可以指向不同 MapID，也可以复用场景并由不同 ExperienceID 装配规则。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience,
		meta=(Categories="Experience.GameMode"))
	FGameplayTag GameplayMode;

	/**
	 * 是否允许把该副本创建为 Listen Server。
	 * 分屏专用测试图保持关闭，避免在线房主进入地图后又按地图策略创建第二个本地玩家。
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bSupportsOnline = true;

	/** If true, a replay will be recorded of the game */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	bool bRecordReplay = false;

	/** Max number of players for this session */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Experience)
	int32 MaxPlayerCount = 16;

public:
	/** Create a request object that is used to actually start a session with these settings */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta = (WorldContext = "WorldContextObject"))
	UCommonSession_HostSessionRequest* CreateHostingRequest(const UObject* WorldContextObject) const;

	/**
	 * HomeMap 副本页面使用的完整请求入口。
	 * Online 先前往 LobbyMap；Offline/LAN 由调用方选择是否直接进入副本。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta = (WorldContext = "WorldContextObject"))
	UCommonSession_HostSessionRequest* CreateHostingRequestWithOptions(
		const UObject* WorldContextObject,
		ECommonSessionOnlineMode OnlineMode,
		int32 RequestedMaxPlayers,
		int32 LocalPlayerCount,
		bool bAllowJoinInProgress,
		bool bFillEmptySlotsWithBots) const;
};
