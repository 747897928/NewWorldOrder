// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Online/ShootSessionCoordinatorSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Parse.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSubsystemUtils.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

#define LOCTEXT_NAMESPACE "ShootSessionCoordinator"

DEFINE_LOG_CATEGORY_STATIC(LogShootSessionCoordinator, Log, All);

namespace
{
	// CommonUser 把这个 OSSv1 协议键定义在插件私有 cpp 中，项目层不能引用其符号；
	// 扩展搜索必须使用相同键名，才能只匹配 CommonSession 创建的 Lobby。
	const FName ShootOnlineSubsystemVersionSetting(TEXT("OSSv1"));
}

#if WITH_DEV_AUTOMATION_TESTS
DEFINE_LOG_CATEGORY_STATIC(LogShootSessionAutomation, Log, All);
#endif

void UShootSessionCoordinatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	Collection.InitializeDependency<UCommonSessionSubsystem>();
	CommonSessionSubsystem = GetGameInstance()->GetSubsystem<UCommonSessionSubsystem>();
	if (!ensure(CommonSessionSubsystem))
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("MissingCommonSession", "Session service unavailable"));
		return;
	}

	CreateSessionCompleteHandle = CommonSessionSubsystem->OnCreateSessionCompleteEvent.AddUObject(
		this, &ThisClass::HandleCreateSessionComplete);
	JoinSessionCompleteHandle = CommonSessionSubsystem->OnJoinSessionCompleteEvent.AddUObject(
		this, &ThisClass::HandleJoinSessionComplete);
	SessionInformationChangedHandle = CommonSessionSubsystem->OnSessionInformationChangedEvent.AddUObject(
		this, &ThisClass::HandleSessionInformationChanged);
	PreClientTravelHandle = CommonSessionSubsystem->OnPreClientTravelEvent.AddUObject(
		this, &ThisClass::HandlePreClientTravel);
	SetState(EShootSessionLifecycleState::Idle, LOCTEXT("Idle", "Not yet joined the session"));

#if WITH_DEV_AUTOMATION_TESTS
	InitializeCommandLineAutomation();
#endif
}

void UShootSessionCoordinatorSubsystem::Deinitialize()
{
#if WITH_DEV_AUTOMATION_TESTS
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(AutomationPostLoadMapHandle);
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(AutomationTimerHandle);
	}
#endif

	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		World->GetTimerManager().ClearTimer(CleanupPollTimerHandle);
	}

	if (ActiveSearchRequest)
	{
		ActiveSearchRequest->OnSearchFinished.RemoveAll(this);
	}
	ClearExpandedSteamSearchDelegate();
	ActiveExpandedSteamSearch.Reset();

	if (CommonSessionSubsystem)
	{
		CommonSessionSubsystem->OnCreateSessionCompleteEvent.Remove(CreateSessionCompleteHandle);
		CommonSessionSubsystem->OnJoinSessionCompleteEvent.Remove(JoinSessionCompleteHandle);
		CommonSessionSubsystem->OnSessionInformationChangedEvent.Remove(SessionInformationChangedHandle);
		CommonSessionSubsystem->OnPreClientTravelEvent.Remove(PreClientTravelHandle);
	}

	CommonSessionSubsystem = nullptr;
	ActiveSearchRequest = nullptr;
	SearchResults.Reset();
	Super::Deinitialize();
}

#if WITH_DEV_AUTOMATION_TESTS
void UShootSessionCoordinatorSubsystem::InitializeCommandLineAutomation()
{
	FString MapIdString;
	if (!FParse::Value(FCommandLine::Get(), TEXT("ShootSessionAutomation="), AutomationRole) ||
		!FParse::Value(FCommandLine::Get(), TEXT("ShootSessionAutomationMap="), MapIdString))
	{
		return;
	}

	AutomationMapId = FPrimaryAssetId::FromString(MapIdString);
	if (!AutomationMapId.IsValid())
	{
		UE_LOG(LogShootSessionAutomation, Error, TEXT("InvalidMapId Role=%s Map=%s"), *AutomationRole, *MapIdString);
		return;
	}

	const bool bKnownRole = AutomationRole == TEXT("HostHold") || AutomationRole == TEXT("ClientLeave") ||
		AutomationRole == TEXT("HostDestroy") || AutomationRole == TEXT("ClientHold");
	if (!bKnownRole)
	{
		UE_LOG(LogShootSessionAutomation, Error, TEXT("UnknownRole=%s"), *AutomationRole);
		return;
	}

	UE_LOG(LogShootSessionAutomation, Display, TEXT("Enabled Role=%s Map=%s"),
		*AutomationRole, *AutomationMapId.ToString());
	AutomationPostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &ThisClass::HandleAutomationPostLoadMap);
}

void UShootSessionCoordinatorSubsystem::HandleAutomationPostLoadMap(UWorld* LoadedWorld)
{
	if (AutomationRole.IsEmpty() || !LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}

	// Join 的 ClientTravel 完成时协调器仍是 InSession；退出/主机销毁后的返回图加载前，
	// ShootGameInstance 已把协调器复位为 Idle。用两者组合区分“加入目标图”和“返回家园地图”。
	const FString LoadedPackage = UWorld::RemovePIEPrefix(LoadedWorld->GetPackage()->GetName());
	if (bAutomationWasInSession && State == EShootSessionLifecycleState::Idle &&
		LoadedPackage == AutomationMapId.PrimaryAssetName.ToString() &&
		(bAutomationLeaveRequested || AutomationRole == TEXT("ClientHold")))
	{
		UE_LOG(LogShootSessionAutomation, Display, TEXT("ReturnedToMap Role=%s World=%s"),
			*AutomationRole, *UWorld::RemovePIEPrefix(LoadedWorld->GetPackage()->GetName()));
		FinishAutomation(TEXT("ReturnedToLobby"));
		return;
	}

	LoadedWorld->GetTimerManager().ClearTimer(AutomationTimerHandle);
	LoadedWorld->GetTimerManager().SetTimer(
		AutomationTimerHandle, this, &ThisClass::RunAutomationStep, 0.5f, true, 3.0f);
}

void UShootSessionCoordinatorSubsystem::RunAutomationStep()
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	ULocalPlayer* TestLocalPlayer = GetGameInstance() ? GetGameInstance()->GetLocalPlayerByIndex(0) : nullptr;
	APlayerController* PlayerController = TestLocalPlayer ? TestLocalPlayer->PlayerController : nullptr;
	if (!World || !PlayerController)
	{
		return;
	}

	const bool bHostRole = AutomationRole == TEXT("HostHold") || AutomationRole == TEXT("HostDestroy");
	if (bHostRole && !bAutomationHostRequested && State == EShootSessionLifecycleState::Idle)
	{
		bAutomationHostRequested = HostSession(
			PlayerController, ECommonSessionOnlineMode::LAN, AutomationMapId, 4, TEXT("SessionAutomation"));
		UE_LOG(LogShootSessionAutomation, Display, TEXT("HostRequested Success=%d"), bAutomationHostRequested);
		if (bAutomationHostRequested)
		{
			const bool bDuplicateAccepted = HostSession(
				PlayerController, ECommonSessionOnlineMode::LAN, AutomationMapId, 4, TEXT("SessionAutomation"));
			UE_LOG(LogShootSessionAutomation, Display, TEXT("DuplicateHostExpectedRejected Accepted=%d"),
				bDuplicateAccepted);
		}
		return;
	}

	if (!bHostRole && !bAutomationSearchRequested && State == EShootSessionLifecycleState::Idle)
	{
		bAutomationSearchRequested = FindSessions(PlayerController, ECommonSessionOnlineMode::LAN);
		UE_LOG(LogShootSessionAutomation, Display, TEXT("FindRequested Success=%d"), bAutomationSearchRequested);
		if (bAutomationSearchRequested)
		{
			const bool bDuplicateAccepted = FindSessions(PlayerController, ECommonSessionOnlineMode::LAN);
			UE_LOG(LogShootSessionAutomation, Display, TEXT("DuplicateFindExpectedRejected Accepted=%d"),
				bDuplicateAccepted);
		}
		return;
	}

	if (!bHostRole && State == EShootSessionLifecycleState::Results && !bAutomationJoinRequested)
	{
		if (!SearchResults.IsEmpty())
		{
			bAutomationJoinRequested = JoinSession(PlayerController, SearchResults[0]);
			UE_LOG(LogShootSessionAutomation, Display, TEXT("JoinRequested Success=%d Results=%d"),
				bAutomationJoinRequested, SearchResults.Num());
		}
		else if (++AutomationRetryCount % 4 == 0)
		{
			bAutomationSearchRequested = FindSessions(PlayerController, ECommonSessionOnlineMode::LAN);
			UE_LOG(LogShootSessionAutomation, Display, TEXT("FindRetried Attempt=%d"), AutomationRetryCount / 4);
		}
		return;
	}

	if (State == EShootSessionLifecycleState::Error && !bAutomationWasInSession && ++AutomationRetryCount % 4 == 0)
	{
		if (!bHostRole)
		{
			bAutomationSearchRequested = FindSessions(PlayerController, ECommonSessionOnlineMode::LAN);
			UE_LOG(LogShootSessionAutomation, Display, TEXT("FindRetriedAfterError Attempt=%d"), AutomationRetryCount / 4);
		}
		return;
	}

	if (State != EShootSessionLifecycleState::InSession)
	{
		return;
	}

	if (!bAutomationWasInSession)
	{
		bAutomationWasInSession = true;
		AutomationActionNotBeforeSeconds = FPlatformTime::Seconds() + 3.0;
		UE_LOG(LogShootSessionAutomation, Display, TEXT("InSession Role=%s NetMode=%d"),
			*AutomationRole, static_cast<int32>(World->GetNetMode()));
	}

	if (FPlatformTime::Seconds() < AutomationActionNotBeforeSeconds || bAutomationLeaveRequested)
	{
		return;
	}

	if (AutomationRole == TEXT("ClientLeave"))
	{
		bAutomationLeaveRequested = LeaveOrDestroySession(PlayerController);
		UE_LOG(LogShootSessionAutomation, Display, TEXT("ClientLeaveRequested Success=%d"), bAutomationLeaveRequested);
	}
	else if (AutomationRole == TEXT("HostDestroy"))
	{
		const AGameStateBase* GameState = World->GetGameState();
		if (GameState && GameState->PlayerArray.Num() >= 2)
		{
			if (!bAutomationRemotePlayerObserved)
			{
				bAutomationRemotePlayerObserved = true;
				AutomationActionNotBeforeSeconds = FPlatformTime::Seconds() + 3.0;
				return;
			}
			if (FPlatformTime::Seconds() < AutomationActionNotBeforeSeconds)
			{
				return;
			}
			bAutomationLeaveRequested = LeaveOrDestroySession(PlayerController);
			UE_LOG(LogShootSessionAutomation, Display, TEXT("HostDestroyRequested Success=%d Players=%d"),
				bAutomationLeaveRequested, GameState->PlayerArray.Num());
		}
	}
}

void UShootSessionCoordinatorSubsystem::FinishAutomation(const TCHAR* Result)
{
	UE_LOG(LogShootSessionAutomation, Display, TEXT("Complete Role=%s Result=%s"), *AutomationRole, Result);
	FPlatformMisc::RequestExit(false);
}
#endif

bool UShootSessionCoordinatorSubsystem::HostSession(APlayerController* PlayerController,
	ECommonSessionOnlineMode OnlineMode, const FPrimaryAssetId& MapId, int32 MaxPlayers,
	const FString& AdvertisedMode)
{
	if (!CanStartSessionRequest())
	{
		UE_LOG(LogShootSessionCoordinator, Warning,
			TEXT("Host request ignored because lifecycle state %d or the platform session is busy"),
			static_cast<int32>(State));
		return false;
	}

	if (!CommonSessionSubsystem || !PlayerController || !PlayerController->GetLocalPlayer())
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InvalidHostPlayer", "Unable to determine the local player who created the session"));
		return false;
	}

	if (!MapId.IsValid())
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InvalidHostMap", "The session creation page does not yet have a valid Map Primary Asset configured."));
		return false;
	}

	UCommonSession_HostSessionRequest* Request = CommonSessionSubsystem->CreateOnlineHostSessionRequest();
	if (!ensure(Request))
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("HostRequestFailed", "Unable to create session request"));
		return false;
	}

	Request->OnlineMode = OnlineMode;
	Request->bUseLobbies = OnlineMode == ECommonSessionOnlineMode::Online;
	Request->bUsePresence = OnlineMode == ECommonSessionOnlineMode::Online;
	Request->MapID = MapId;
	Request->MaxPlayerCount = FMath::Max(1, MaxPlayers);
	Request->ModeNameForAdvertisement = AdvertisedMode;
	return HostSessionRequest(PlayerController, Request);
}

bool UShootSessionCoordinatorSubsystem::HostSessionRequest(APlayerController* PlayerController,
	UCommonSession_HostSessionRequest* Request)
{
	if (!CanStartSessionRequest())
	{
		UE_LOG(LogShootSessionCoordinator, Warning,
			TEXT("Host request ignored because lifecycle state %d or the platform session is busy"),
			static_cast<int32>(State));
		return false;
	}

	if (!CommonSessionSubsystem || !PlayerController || !PlayerController->GetLocalPlayer() || !Request)
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InvalidHostRequest", "Invalid session creation request"));
		return false;
	}

	FText ValidationError;
	if (!Request->ValidateAndLogErrors(ValidationError))
	{
		SetState(EShootSessionLifecycleState::Error,
			ValidationError.IsEmpty() ? LOCTEXT("InvalidHostRequestData", "Invalid session creation parameters") : ValidationError);
		return false;
	}

	CurrentOnlineMode = Request->OnlineMode;
	Role = EShootSessionRole::Host;
	SearchResults.Reset();
	const FText HostingStatus =
		Request->OnlineMode == ECommonSessionOnlineMode::Offline ? LOCTEXT("HostingOffline", "Entering the local expedition…") :
		Request->OnlineMode == ECommonSessionOnlineMode::LAN ? LOCTEXT("HostingLan", "Creating a LAN session…") :
		LOCTEXT("HostingOnline", "Creating a Steam session…");
	SetState(EShootSessionLifecycleState::Hosting, HostingStatus);
	CommonSessionSubsystem->HostSession(PlayerController, Request);
	return true;
}

bool UShootSessionCoordinatorSubsystem::SetJoinInProgressAllowed(bool bAllowed)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	IOnlineSubsystem* OnlineSubsystem = World ? Online::GetSubsystem(World) : nullptr;
	const IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (!SessionInterface.IsValid())
	{
		return false;
	}

	const FName SessionName(NAME_GameSession);
	FNamedOnlineSession* Session = SessionInterface->GetNamedSession(SessionName);
	if (!Session || !Session->bHosting)
	{
		return false;
	}

	Session->SessionSettings.bAllowJoinInProgress = bAllowed;
	Session->SessionSettings.Set(FName(TEXT("ALLOW_JIP")), bAllowed ? 1 : 0,
		EOnlineDataAdvertisementType::ViaOnlineService);
	return SessionInterface->UpdateSession(SessionName, Session->SessionSettings, true);
}

bool UShootSessionCoordinatorSubsystem::FindSessions(APlayerController* PlayerController,
	ECommonSessionOnlineMode OnlineMode)
{
	if (!CanStartSessionRequest())
	{
		UE_LOG(LogShootSessionCoordinator, Warning,
			TEXT("Find request ignored because lifecycle state %d or the platform session is busy"),
			static_cast<int32>(State));
		return false;
	}

	if (!CommonSessionSubsystem || !PlayerController || !PlayerController->GetLocalPlayer())
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InvalidSearchPlayer", "Unable to determine the local player searching for sessions"));
		return false;
	}

	if (ActiveSearchRequest)
	{
		ActiveSearchRequest->OnSearchFinished.RemoveAll(this);
	}

	ActiveSearchRequest = CommonSessionSubsystem->CreateOnlineSearchSessionRequest();
	if (!ensure(ActiveSearchRequest))
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("SearchRequestFailed", "Unable to create the session search request"));
		return false;
	}

	ActiveSearchRequest->OnlineMode = OnlineMode;
	ActiveSearchRequest->bUseLobbies = OnlineMode == ECommonSessionOnlineMode::Online;
	ActiveSearchRequest->OnSearchFinished.AddUObject(this, &ThisClass::HandleSearchFinished);
	CurrentOnlineMode = OnlineMode;
	SearchResults.Reset();
	SetState(EShootSessionLifecycleState::Searching,
		OnlineMode == ECommonSessionOnlineMode::LAN ? LOCTEXT("SearchingLan", "Searching for LAN sessions…") : LOCTEXT("SearchingOnline", "Searching for Steam sessions…"));
	if (OnlineMode == ECommonSessionOnlineMode::Online && StartExpandedSteamSearch(PlayerController))
	{
		return true;
	}
	CommonSessionSubsystem->FindSessions(PlayerController, ActiveSearchRequest);
	return true;
}

bool UShootSessionCoordinatorSubsystem::StartExpandedSteamSearch(APlayerController* PlayerController)
{
	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	IOnlineSubsystem* OnlineSubsystem = World ? Online::GetSubsystem(World) : nullptr;
	if (!OnlineSubsystem || OnlineSubsystem->GetSubsystemName() != STEAM_SUBSYSTEM)
	{
		return false;
	}

	IOnlineSessionPtr SessionInterface = OnlineSubsystem->GetSessionInterface();
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	const FUniqueNetIdRepl UserId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();
	if (!SessionInterface.IsValid() || !UserId.IsValid())
	{
		SetState(EShootSessionLifecycleState::Error,
			LOCTEXT("SteamSearchUnavailable", "Steam session search is unavailable. Confirm that Steam is signed in, then restart the game."));
		return true;
	}

	ClearExpandedSteamSearchDelegate();
	ActiveExpandedSteamSearch = MakeShared<FOnlineSessionSearch>();
	ActiveExpandedSteamSearch->bIsLanQuery = false;
	ActiveExpandedSteamSearch->MaxSearchResults = FMath::Max(10, SteamSearchMaxResults);
	ActiveExpandedSteamSearch->PingBucketSize = 50;
	ActiveExpandedSteamSearch->QuerySettings.Set(
		ShootOnlineSubsystemVersionSetting, true, EOnlineComparisonOp::Equals);
	ActiveExpandedSteamSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	ActiveExpandedSteamSearch->QuerySettings.Set(
		SETTING_SESSION_TEMPLATE_NAME, FString(TEXT("GameSession")), EOnlineComparisonOp::Equals);

	ExpandedSteamSearchCompleteHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleExpandedSteamSearchComplete));
	UE_LOG(LogShootSessionCoordinator, Display,
		TEXT("Starting expanded Steam lobby search MaxResults=%d BuildId=%d"),
		ActiveExpandedSteamSearch->MaxSearchResults, GetBuildUniqueId());
	const TSharedPtr<FOnlineSessionSearch> DispatchedSearch = ActiveExpandedSteamSearch;
	if (!SessionInterface->FindSessions(*UserId.GetUniqueNetId(), DispatchedSearch.ToSharedRef()) &&
		ActiveExpandedSteamSearch == DispatchedSearch)
	{
		// OSSv1 的部分失败会在 FindSessions 返回 false 前同步广播完成委托。
		// 回调已经消费并 Reset 请求时不能再次 Notify，否则 UI 会收到两次终态。
		ClearExpandedSteamSearchDelegate();
		ActiveExpandedSteamSearch.Reset();
		ActiveSearchRequest->NotifySearchFinished(
			false, LOCTEXT("SteamSearchDispatchFailed", "Steam could not start the session search. Check your network connection and sign-in status."));
	}
	return true;
}

void UShootSessionCoordinatorSubsystem::HandleExpandedSteamSearchComplete(bool bWasSuccessful)
{
	ClearExpandedSteamSearchDelegate();
	if (!ActiveSearchRequest || !ActiveExpandedSteamSearch)
	{
		ActiveExpandedSteamSearch.Reset();
		return;
	}

	ActiveSearchRequest->Results.Reset();
	if (bWasSuccessful)
	{
		for (const FOnlineSessionSearchResult& PlatformResult : ActiveExpandedSteamSearch->SearchResults)
		{
			if (!PlatformResult.IsValid())
			{
				continue;
			}

			UCommonSession_SearchResult* Result = NewObject<UCommonSession_SearchResult>(ActiveSearchRequest);
			Result->Result = PlatformResult;
			ActiveSearchRequest->Results.Add(Result);
		}
	}

	UE_LOG(LogShootSessionCoordinator, Display,
		TEXT("Expanded Steam lobby search completed Success=%d CompatibleResults=%d BuildId=%d"),
		bWasSuccessful, ActiveSearchRequest->Results.Num(), GetBuildUniqueId());
	ActiveExpandedSteamSearch.Reset();
	ActiveSearchRequest->NotifySearchFinished(
		bWasSuccessful,
		bWasSuccessful ? FText() : LOCTEXT("SteamSearchFailed", "Steam session search failed. Please try again later."));
}

void UShootSessionCoordinatorSubsystem::ClearExpandedSteamSearchDelegate()
{
	if (!ExpandedSteamSearchCompleteHandle.IsValid())
	{
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	IOnlineSubsystem* OnlineSubsystem = World ? Online::GetSubsystem(World) : nullptr;
	IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(ExpandedSteamSearchCompleteHandle);
	}
	ExpandedSteamSearchCompleteHandle.Reset();
}

bool UShootSessionCoordinatorSubsystem::JoinSession(APlayerController* PlayerController,
	UCommonSession_SearchResult* SearchResult)
{
	if (!CanStartSessionRequest())
	{
		UE_LOG(LogShootSessionCoordinator, Warning,
			TEXT("Join request ignored because lifecycle state %d or the platform session is busy"),
			static_cast<int32>(State));
		return false;
	}

	if (!CommonSessionSubsystem || !PlayerController || !PlayerController->GetLocalPlayer() || !SearchResult)
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InvalidJoin", "The selected session is no longer available. Search again."));
		return false;
	}

	Role = EShootSessionRole::Client;
	bool bFoundMapName = false;
	SearchResult->GetStringSetting(TEXT("MAPNAME"), PendingJoinMapName, bFoundMapName);
	if (!bFoundMapName || !PendingJoinMapName.StartsWith(TEXT("/")))
	{
		PendingJoinMapName.Reset();
	}
	// Find 会保留 Online/LAN；平台邀请没有先执行 Find，Offline 在此代表 Steam 在线邀请。
	if (CurrentOnlineMode == ECommonSessionOnlineMode::Offline)
	{
		CurrentOnlineMode = ECommonSessionOnlineMode::Online;
	}
	SetState(EShootSessionLifecycleState::Joining, LOCTEXT("Joining", "Joining the session…"));
	CommonSessionSubsystem->JoinSession(PlayerController, SearchResult);
	return true;
}

bool UShootSessionCoordinatorSubsystem::LeaveOrDestroySession(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InvalidLeavePlayer", "Unable to determine the local player leaving the session"));
		return false;
	}

	SetState(EShootSessionLifecycleState::Leaving,
		Role == EShootSessionRole::Host ? LOCTEXT("Destroying", "Ending the session and returning all players to the Home Base…") : LOCTEXT("Leaving", "Leaving the session and returning to the Home Base…"));

	// 主机必须通过 GameSession 通知每个远程客户端；每台机器最终都会进入 ShootGameInstance::ReturnToMainMenu，
	// 在那里调用 CommonSession CleanUp 并进入家园地图。客户端只处理自己的返回流程。
	if (UWorld* World = PlayerController->GetWorld(); World && World->GetNetMode() != NM_Client)
	{
		if (AGameModeBase* GameMode = World->GetAuthGameMode())
		{
			GameMode->ReturnToMainMenuHost();
			return true;
		}
	}

	if (UGameInstance* GameInstance = PlayerController->GetGameInstance())
	{
		GameInstance->ReturnToMainMenu();
		return true;
	}

	SetState(EShootSessionLifecycleState::Error, LOCTEXT("LeaveFailed", "Unable to start the return-to-home flow"));
	return false;
}

bool UShootSessionCoordinatorSubsystem::CleanUpResidualSession()
{
	if (!CommonSessionSubsystem)
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("CleanupUnavailable", "Session cleanup service unavailable"));
		return false;
	}

	Role = EShootSessionRole::None;
	CurrentOnlineMode = ECommonSessionOnlineMode::Offline;
	SearchResults.Reset();
	PendingJoinMapName.Reset();

	// CommonUser 的 Lyra 上游实现会在 NoSession 时仍留下内部 pending 标记。项目层先检查 OSSv1
	// 状态：没有会话就不调用 CleanUpSessions；真有会话才调用，并一直等到平台状态变为
	// NoSession 后再开放 Host/Find。这样无需修改 Plugins/CommonUser 的官方示例代码。
	if (!HasPlatformSession())
	{
		if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
		{
			World->GetTimerManager().ClearTimer(CleanupPollTimerHandle);
		}
		SetState(EShootSessionLifecycleState::Idle, LOCTEXT("CleanupNotNeeded", "No residual session found on this machine"));
		return true;
	}

	const bool bCleanupAlreadyRunning = State == EShootSessionLifecycleState::CleaningUp;
	SetState(EShootSessionLifecycleState::CleaningUp, LOCTEXT("CleaningUp", "Cleaning up local session state…"));
	if (!bCleanupAlreadyRunning)
	{
		CommonSessionSubsystem->CleanUpSessions();
	}

	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		CleanupPollCount = 0;
		World->GetTimerManager().SetTimer(
			CleanupPollTimerHandle, this, &ThisClass::PollCleanupCompletion, 0.1f, true);
	}
	return true;
}

void UShootSessionCoordinatorSubsystem::PollCleanupCompletion()
{
	if (State != EShootSessionLifecycleState::CleaningUp)
	{
		return;
	}

	UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!HasPlatformSession())
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(CleanupPollTimerHandle);
		}
		SetState(EShootSessionLifecycleState::Idle, LOCTEXT("CleanupComplete", "Local session state cleaned up"));
		return;
	}

	// Steam 销毁通常很快；三十秒仍未回到 NoSession 时给出可见错误，避免菜单永久锁死。
	if (++CleanupPollCount >= 300)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(CleanupPollTimerHandle);
		}
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("CleanupTimedOut", "Session cleanup timed out. Return home and try again."));
	}
}

bool UShootSessionCoordinatorSubsystem::HasPlatformSession() const
{
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	IOnlineSubsystem* OnlineSubsystem = World ? Online::GetSubsystem(World) : nullptr;
	const IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	return SessionInterface.IsValid() &&
		SessionInterface->GetSessionState(NAME_GameSession) != EOnlineSessionState::NoSession;
}

bool UShootSessionCoordinatorSubsystem::CanStartSessionRequest() const
{
	// UI 禁用只是表现保护，真正的并发门禁必须在协调层。这样快速双击、重复蓝图事件和平台邀请
	// 都无法在上一个 Host/Find/Join/Cleanup 尚未结束时再次进入 CommonSession。
	const bool bStableMenuState = State == EShootSessionLifecycleState::Idle ||
		State == EShootSessionLifecycleState::Results || State == EShootSessionLifecycleState::Error;
	return bStableMenuState && !HasPlatformSession();
}

bool UShootSessionCoordinatorSubsystem::RefreshPlatformSessionState()
{
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	IOnlineSubsystem* OnlineSubsystem = World ? Online::GetSubsystem(World) : nullptr;
	const IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const FNamedOnlineSession* NamedSession = SessionInterface.IsValid() ?
		SessionInterface->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession || NamedSession->SessionState == EOnlineSessionState::NoSession)
	{
		return false;
	}

	// 地图切换与 Shipping 优化不能让 UI 只依赖内存状态。Steam/Null 实际 NamedSession 才是
	// “能否邀请、是否已在会话”的权威事实；这里仅把事实同步回项目协调器，不持有 OSS 接口。
	CurrentOnlineMode = NamedSession->SessionSettings.bIsLANMatch ?
		ECommonSessionOnlineMode::LAN : ECommonSessionOnlineMode::Online;
	Role = World && World->GetNetMode() == NM_Client ? EShootSessionRole::Client : EShootSessionRole::Host;

	if (State != EShootSessionLifecycleState::Hosting && State != EShootSessionLifecycleState::Joining &&
		State != EShootSessionLifecycleState::Leaving && State != EShootSessionLifecycleState::CleaningUp)
	{
		SetState(EShootSessionLifecycleState::InSession,
			CurrentOnlineMode == ECommonSessionOnlineMode::LAN ?
			LOCTEXT("LanSessionActive", "Currently in a LAN session") :
			LOCTEXT("OnlineSessionActive", "Currently in a Steam session"));
	}
	return true;
}

FShootSessionRuntimeInfo UShootSessionCoordinatorSubsystem::GetRuntimeSessionInfo() const
{
	FShootSessionRuntimeInfo Result;
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	IOnlineSubsystem* OnlineSubsystem = World ? Online::GetSubsystem(World) : nullptr;
	const IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const FNamedOnlineSession* NamedSession = SessionInterface.IsValid() ?
		SessionInterface->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession || NamedSession->SessionState == EOnlineSessionState::NoSession)
	{
		return Result;
	}

	Result.bIsInSession = true;
	Result.PlatformName = OnlineSubsystem ? OnlineSubsystem->GetSubsystemName().ToString() : FString();
	Result.SessionName = NamedSession->SessionName.ToString();
	Result.SessionState = EOnlineSessionState::ToString(NamedSession->SessionState);
	Result.RoleName = World && World->GetNetMode() == NM_Client ? TEXT("Client") : TEXT("Host");
	NamedSession->SessionSettings.Get(SETTING_MAPNAME, Result.MapName);
	Result.BuildId = NamedSession->SessionSettings.BuildUniqueId;
	return Result;
}

TArray<FShootSessionPlayerInfo> UShootSessionCoordinatorSubsystem::GetRuntimePlayerInfo() const
{
	TArray<FShootSessionPlayerInfo> Result;
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	const AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return Result;
	}

	int32 DisplayIndex = 1;
	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (!PlayerState)
		{
			continue;
		}

		FShootSessionPlayerInfo& PlayerInfo = Result.AddDefaulted_GetRef();
		PlayerInfo.DisplayIndex = DisplayIndex++;
		PlayerInfo.PlayerName = PlayerState->GetPlayerName();
		PlayerInfo.PingMilliseconds = PlayerState->GetPingInMilliseconds();
	}
	return Result;
}

bool UShootSessionCoordinatorSubsystem::ShowInviteUI(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->GetLocalPlayer())
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InviteInvalidPlayer", "Unable to determine the local player sending the invite"));
		return false;
	}

	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(PlayerController->GetWorld());
	const IOnlineSessionPtr SessionInterface = OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
	const FNamedOnlineSession* NamedSession = SessionInterface.IsValid() ?
		SessionInterface->GetNamedSession(NAME_GameSession) : nullptr;
	if (!NamedSession || NamedSession->SessionState == EOnlineSessionState::NoSession)
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InviteNoSession", "There is no platform session available for inviting friends"));
		return false;
	}
	if (NamedSession->SessionSettings.bIsLANMatch)
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InviteLanSession", "LAN sessions do not support platform friend invites"));
		return false;
	}

	RefreshPlatformSessionState();
	IOnlineExternalUIPtr ExternalUI = OnlineSubsystem->GetExternalUIInterface();
	if (!ExternalUI.IsValid() || !ExternalUI->ShowInviteUI(
		PlayerController->GetLocalPlayer()->GetLocalPlayerIndex(), NAME_GameSession))
	{
		SetState(EShootSessionLifecycleState::Error, LOCTEXT("InviteOverlayFailed", "The platform friend invite UI could not be opened. Check the overlay and friend status."));
		return false;
	}

	SetState(EShootSessionLifecycleState::InSession, LOCTEXT("InviteOverlayOpened", "Platform friend invite UI opened"));
	return true;
}

void UShootSessionCoordinatorSubsystem::ReportInvitationJoinFailure(const FText& ErrorMessage)
{
	Role = EShootSessionRole::None;
	PendingJoinMapName.Reset();
	if (!HasPlatformSession())
	{
		CurrentOnlineMode = ECommonSessionOnlineMode::Offline;
	}
	SetState(EShootSessionLifecycleState::Error,
		ErrorMessage.IsEmpty() ? LOCTEXT("InvitationJoinFailed", "Failed to accept the platform invite. Return to the online page and try again.") : ErrorMessage);
}

bool UShootSessionCoordinatorSubsystem::IsInSessionOrTransitioning() const
{
	return State == EShootSessionLifecycleState::CleaningUp || State == EShootSessionLifecycleState::Hosting ||
		State == EShootSessionLifecycleState::Joining ||
		State == EShootSessionLifecycleState::InSession || State == EShootSessionLifecycleState::Leaving ||
		HasPlatformSession();
}

void UShootSessionCoordinatorSubsystem::SetState(EShootSessionLifecycleState NewState, const FText& NewStatus)
{
	const EShootSessionLifecycleState PreviousState = State;
	State = NewState;
	StatusText = NewStatus;
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	UE_LOG(LogShootSessionCoordinator, Display,
		TEXT("Lifecycle %d -> %d Map=%s NetMode=%d Status=%s"),
		static_cast<int32>(PreviousState), static_cast<int32>(NewState),
		World ? *UWorld::RemovePIEPrefix(World->GetPackage()->GetName()) : TEXT("None"),
		World ? static_cast<int32>(World->GetNetMode()) : INDEX_NONE,
		*NewStatus.ToString());
	OnCoordinatorChanged.Broadcast();
}

void UShootSessionCoordinatorSubsystem::HandleSearchFinished(bool bSucceeded, const FText& ErrorMessage)
{
	if (!ActiveSearchRequest)
	{
		return;
	}

	ActiveSearchRequest->OnSearchFinished.RemoveAll(this);
	SearchResults = ActiveSearchRequest->Results;
	if (!bSucceeded)
	{
		SetState(EShootSessionLifecycleState::Error,
			ErrorMessage.IsEmpty() ? LOCTEXT("SearchFailed", "Session search failed") : ErrorMessage);
		return;
	}

	SetState(EShootSessionLifecycleState::Results,
		SearchResults.IsEmpty() ? LOCTEXT("NoResults", "No sessions available to join") :
		FText::Format(LOCTEXT("ResultCount", "Found {0} sessions. Select one to join."), SearchResults.Num()));
}

void UShootSessionCoordinatorSubsystem::HandleCreateSessionComplete(const FOnlineResultInformation& Result)
{
	if (Result.bWasSuccessful)
	{
		Role = EShootSessionRole::Host;
		SetState(EShootSessionLifecycleState::InSession, LOCTEXT("HostReady", "Session created. Entering the expedition lobby…"));
	}
	else
	{
		Role = EShootSessionRole::None;
		SetState(EShootSessionLifecycleState::Error,
			Result.ErrorText.IsEmpty() ? LOCTEXT("HostFailed", "Failed to create session") : Result.ErrorText);
	}
}

void UShootSessionCoordinatorSubsystem::HandleJoinSessionComplete(const FOnlineResultInformation& Result)
{
	if (Result.bWasSuccessful)
	{
		Role = EShootSessionRole::Client;
		SetState(EShootSessionLifecycleState::InSession, LOCTEXT("JoinReady", "Joined successfully. Connecting to the host…"));
	}
	else
	{
		Role = EShootSessionRole::None;
		PendingJoinMapName.Reset();
		SetState(EShootSessionLifecycleState::Error,
			Result.ErrorText.IsEmpty() ? LOCTEXT("JoinFailed", "Failed to join the session. Refresh and try again.") : Result.ErrorText);
	}
}

void UShootSessionCoordinatorSubsystem::HandleSessionInformationChanged(ECommonSessionInformationState SessionStatus,
	const FString& GameMode, const FString& MapName)
{
	if (SessionStatus == ECommonSessionInformationState::OutOfGame && State == EShootSessionLifecycleState::Leaving)
	{
		Role = EShootSessionRole::None;
		CurrentOnlineMode = ECommonSessionOnlineMode::Offline;
		SetState(EShootSessionLifecycleState::Idle, LOCTEXT("ReturnedToLobby", "Returned to the Home Base"));
	}
}

void UShootSessionCoordinatorSubsystem::HandlePreClientTravel(FString& URL)
{
	if (PendingJoinMapName.IsEmpty() || URL.IsEmpty())
	{
		return;
	}

	// OSSv1 的 ResolveConnectString 可能只返回主机地址。UE 用它构造 FURL 时会自动补上
	// GameDefaultMap（本项目是 FrontEndMap），导致握手前先 Browse 到错误地图；连接失败时玩家还会
	// 留在那里。CommonSession 提供此项目级钩子，使用 Session 广告的 MAPNAME 补全地址。
	const FString OriginalURL = URL;
	const int32 OptionsIndex = URL.Find(TEXT("?"));
	const FString Options = OptionsIndex == INDEX_NONE ? FString() : URL.Mid(OptionsIndex);
	FString AddressAndMap = OptionsIndex == INDEX_NONE ? URL : URL.Left(OptionsIndex);
	const int32 ExistingMapIndex = AddressAndMap.Find(TEXT("/Game/"));
	if (ExistingMapIndex != INDEX_NONE)
	{
		AddressAndMap.LeftInline(ExistingMapIndex, EAllowShrinking::No);
	}

	URL = AddressAndMap + PendingJoinMapName + Options;
	PendingJoinMapName.Reset();
	UE_LOG(LogShootSessionCoordinator, Display, TEXT("Completed session travel URL from '%s' to '%s'"),
		*OriginalURL, *URL);
}

#undef LOCTEXT_NAMESPACE
