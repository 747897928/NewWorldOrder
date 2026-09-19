// Copyright ZhaoYiJie


#include "System/ShootGameInstance.h"

#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "CommonSessionSubsystem.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameModes/ShootGameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "Settings/LyraSettingsShared.h"
#include "TimerManager.h"
#include "UI/CustomGameViewportClient.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogShootGameInstance, Log, All);

UShootGameInstance::UShootGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootGameInstance::SetDisableSplitScreen(bool bDisableSplitScreen)
{
	if (UCustomGameViewportClient* CustomGameViewportClient = Cast<UCustomGameViewportClient>(
			GetGameViewportClient()))
	{
		CustomGameViewportClient->SetDisableSplitScreen(bDisableSplitScreen);
	}
}

void UShootGameInstance::SetPendingLocalCoopProtagonists(
	ECharacterGender Player01Gender, ECharacterGender Player02Gender)
{
	const bool bValidPair = Player01Gender != ECharacterGender::UNKNOWN &&
		Player02Gender != ECharacterGender::UNKNOWN && Player01Gender != Player02Gender;
	PendingPlayer01Gender = bValidPair ? Player01Gender : ECharacterGender::UNKNOWN;
	PendingPlayer02Gender = bValidPair ? Player02Gender : ECharacterGender::UNKNOWN;
	bHasPendingLocalCoopProtagonists = bValidPair;
}

bool UShootGameInstance::GetPendingLocalCoopProtagonist(
	int32 LocalPlayerIndex, ECharacterGender& OutGender) const
{
	if (!bHasPendingLocalCoopProtagonists || !FMath::IsWithinInclusive(LocalPlayerIndex, 0, 1))
	{
		return false;
	}

	OutGender = LocalPlayerIndex == 0 ? PendingPlayer01Gender : PendingPlayer02Gender;
	return OutGender != ECharacterGender::UNKNOWN;
}

void UShootGameInstance::ClearPendingLocalCoopProtagonists()
{
	PendingPlayer01Gender = ECharacterGender::UNKNOWN;
	PendingPlayer02Gender = ECharacterGender::UNKNOWN;
	bHasPendingLocalCoopProtagonists = false;
}

void UShootGameInstance::Init()
{
	Super::Init();

	// 没有玩家主动保存的 Culture 时，首启语言按 Steam -> 系统 -> 简体中文回退解析。
	// 自动解析只设置当前进程，不写入用户配置；设置页 Apply 才会持久化玩家的明确选择。
	if (!GIsEditor && !IsRunningCommandlet() && !IsRunningDedicatedServer())
	{
		FString SavedCulture;
		const bool bHasSavedCulture = GConfig->GetString(
			TEXT("Internationalization"), TEXT("Culture"), SavedCulture, GGameUserSettingsIni)
			&& !SavedCulture.IsEmpty();

		if (!bHasSavedCulture)
		{
			const FString DefaultCulture = ULyraSettingsShared::ResolveDefaultCulture();
			if (FInternationalization::Get().GetCurrentCulture().Get().GetName() != DefaultCulture)
			{
				FInternationalization::Get().SetCurrentCulture(DefaultCulture);
			}
		}
	}

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this, &ThisClass::HandlePostLoadMap);
	if (GEngine)
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(
			this, &ThisClass::HandleNetworkFailure);
	}
}

void UShootGameInstance::Shutdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(JoinRequestedSessionRetryTimer);
	}
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
	if (GEngine)
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
	}
	Super::Shutdown();
}

void UShootGameInstance::ReturnToMainMenu()
{
	bSessionReturnTravelPending = true;
	// GameSession::ReturnToMainMenuHost 会在主机与每个客户端各自调用这里。
	// 每台机器都先清理自己的 CommonSession 状态，再独立进入 HomeMap；CommonUser 登录状态保留，
	// 因此回到大厅后可以继续 Host/Find/Join，不需要重新登录 Steam。
	// 所有返回大厅入口都必须同时清理 CommonSession 和本项目的 UI 状态机。
	// 平台邀请拒绝、外部销毁请求等路径并不一定先经过 SessionScreen，直接只调用
	// UCommonSessionSubsystem::CleanUpSessions 会让菜单仍显示 InSession，因此优先交给协调器统一复位。
	if (UShootSessionCoordinatorSubsystem* Coordinator = GetSubsystem<UShootSessionCoordinatorSubsystem>())
	{
		Coordinator->CleanUpResidualSession();
	}
	else if (UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>())
	{
		SessionSubsystem->CleanUpSessions();
	}

	if (TravelToSessionReturnMap(GetWorld()))
	{
		return;
	}

	Super::ReturnToMainMenu();
}

bool UShootGameInstance::IsSessionReturnMap(const UWorld* World) const
{
	// PIE 世界的完整包名形如 /Game/Maps/UEDPIE_0_HomeMap。这里统一比较去掉 PIE 前缀后的
	// 关卡短名，避免 UI 把 HomeMap 误判为副本并显示“退出副本”。
	const FString CurrentMapName = World ? UGameplayStatics::GetCurrentLevelName(World, true) : FString();
	const FString ReturnMapName = FPackageName::GetShortName(SessionReturnMap.ToSoftObjectPath().GetLongPackageName());
	return !CurrentMapName.IsEmpty() && !ReturnMapName.IsEmpty() && CurrentMapName == ReturnMapName;
}

void UShootGameInstance::OnUserRequestedSession(const FPlatformUserId& PlatformUserId,
	UCommonSession_SearchResult* InRequestedSession, const FOnlineResultInformation& RequestedSessionResult)
{
	// CommonSession 会告诉我们是哪一个平台用户接受了邀请。先保存归属再调用父类，因为父类
	// SetRequestedSession 可能立即回调本类 JoinRequestedSession；顺序反过来会错误使用主玩家。
	RequestedSessionPlatformUser = PlatformUserId;
	JoinRequestedSessionRetryCount = 0;
	Super::OnUserRequestedSession(PlatformUserId, InRequestedSession, RequestedSessionResult);
	if (!InRequestedSession)
	{
		RequestedSessionPlatformUser = FPlatformUserId();
		if (UShootSessionCoordinatorSubsystem* Coordinator = GetSubsystem<UShootSessionCoordinatorSubsystem>())
		{
			// CommonGame 会显示系统消息；协调器同时保存错误，玩家稍后打开 SessionScreen 时仍能看到原因。
			Coordinator->ReportInvitationJoinFailure(RequestedSessionResult.ErrorText);
		}
	}
}

bool UShootGameInstance::CanJoinRequestedSession() const
{
	const UWorld* World = GetWorld();
	const ULocalPlayer* LocalPlayer = ResolveRequestedSessionLocalPlayer();
	const FString CurrentMap = World ? UWorld::RemovePIEPrefix(World->GetPackage()->GetName()) : FString();
	const FString ReturnMap = SessionReturnMap.ToSoftObjectPath().GetLongPackageName();
	if (!World || !LocalPlayer || !LocalPlayer->PlayerController || CurrentMap != ReturnMap)
	{
		// CommonGame 默认会在邀请抵达时立即 Join。平台冷启动时 PlayerController 常常尚未存在，
		// FrontEndMap 也没有联机会话 UI；先进入可配置 HomeMap，等本地玩家和 UI 根布局准备好再 Join。
		return false;
	}

	if (const UShootSessionCoordinatorSubsystem* Coordinator =
		GetSubsystem<UShootSessionCoordinatorSubsystem>())
	{
		return !Coordinator->IsInSessionOrTransitioning();
	}
	return Super::CanJoinRequestedSession();
}
void UShootGameInstance::JoinRequestedSession()
{
	UCommonSession_SearchResult* LocalRequestedSession = GetRequestedSession();
	if (!LocalRequestedSession)
	{
		return;
	}

	if (!CanJoinRequestedSession())
	{
		bJoinRequestedSessionAfterTravel = true;
		TryJoinRequestedSessionAfterTravel();
		return;
	}

	ULocalPlayer* LocalPlayer = ResolveRequestedSessionLocalPlayer();
	if (!LocalPlayer || !LocalPlayer->PlayerController)
	{
		bJoinRequestedSessionAfterTravel = true;
		TryJoinRequestedSessionAfterTravel();
		return;
	}

	bool bJoinAccepted = false;
	if (UShootSessionCoordinatorSubsystem* Coordinator = GetSubsystem<UShootSessionCoordinatorSubsystem>())
	{
		// 平台邀请也必须经过项目协调器。这样 Joining/InSession、网络失败回 HomeMap、邀请按钮和
		// SessionScreen 使用同一份状态，不再出现“实际 Join 了但 UI 仍显示尚未加入”的分叉。
		bJoinAccepted = Coordinator->JoinSession(LocalPlayer->PlayerController, LocalRequestedSession);
	}
	else if (UCommonSessionSubsystem* SessionSubsystem = GetSubsystem<UCommonSessionSubsystem>())
	{
		SessionSubsystem->JoinSession(LocalPlayer->PlayerController, LocalRequestedSession);
		bJoinAccepted = true;
	}

	if (bJoinAccepted)
	{
		UE_LOG(LogShootGameInstance, Display, TEXT("Platform invitation join dispatched from map %s"),
			*UWorld::RemovePIEPrefix(GetWorld()->GetPackage()->GetName()));
		SetRequestedSession(nullptr);
		RequestedSessionPlatformUser = FPlatformUserId();
		bJoinRequestedSessionAfterTravel = false;
	}
}
void UShootGameInstance::ResetGameAndJoinRequestedSession()
{
	// Steam 覆盖层邀请可能在玩家已经处于另一个会话时到达。
	// 保留 CommonGameInstance 中的 RequestedSession，先走双方完整退出流程；HomeMap 加载完成后再 Join。
	bJoinRequestedSessionAfterTravel = GetRequestedSession() != nullptr;
	JoinRequestedSessionRetryCount = 0;
	ReturnToMainMenu();
}

void UShootGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	ApplyLocalPlayerMapPolicy(LoadedWorld);

	if (LoadedWorld && UWorld::RemovePIEPrefix(LoadedWorld->GetPackage()->GetName()) ==
		SessionReturnMap.ToSoftObjectPath().GetLongPackageName())
	{
		bSessionReturnTravelPending = false;
	}

	if (LoadedWorld == GetWorld() && bJoinRequestedSessionAfterTravel)
	{
		TryJoinRequestedSessionAfterTravel();
	}
}

void UShootGameInstance::ApplyLocalPlayerMapPolicy(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetGameInstance() != this)
	{
		return;
	}

	const AShootGameModeBase* ShootGameMode = LoadedWorld->GetAuthGameMode<AShootGameModeBase>();
	if (!ShootGameMode)
	{
		// 客户端世界没有 AuthGameMode。它不应自行删除连接到服务器的分屏子玩家；
		// 会话退出并加载本机 HomeMap 后，新世界拥有权威 GameMode，再执行 PrimaryOnly。
		return;
	}

	const EShootLocalPlayerMapPolicy LocalPlayerPolicy = ShootGameMode->GetLocalPlayerMapPolicy();
	if (LocalPlayerPolicy == EShootLocalPlayerMapPolicy::KeepCurrent)
	{
		// KeepCurrent 不改变当前机器已有的 LocalPlayer 数量与分屏开关。
		return;
	}

	const bool bPrimaryOnly = LocalPlayerPolicy == EShootLocalPlayerMapPolicy::PrimaryOnly;
	const bool bSplitProtagonists =
		LocalPlayerPolicy == EShootLocalPlayerMapPolicy::SplitProtagonists;
	SetDisableSplitScreen(!bSplitProtagonists);
	if (bPrimaryOnly)
	{
		// 返回 HomeMap 后不允许旧的双人选角结果泄漏到下一次本地合作启动。
		ClearPendingLocalCoopProtagonists();
	}

	if (bSplitProtagonists && GetNumLocalPlayers() < 2)
	{
		if (LoadedWorld->GetNetMode() == NM_Standalone)
		{
			// LocalPlayer 由 GameInstance 持有，但是否启用本地双人由当前 GameMode 配置。
			// 这里只创建第二位本地玩家；Pawn、PlayerState、HUD 与输入继续走引擎的
			// Login/RestartPlayer 和 Experience/GameFrameworkComponentManager 生命周期。
			// 禁止像旧 BP_TestGameMode 图表那样手工 Spawn/Possess Pawn 或 AddToPlayerScreen。
			while (GetNumLocalPlayers() < 2)
			{
				if (!UGameplayStatics::CreatePlayer(LoadedWorld, INDEX_NONE, true))
				{
					UE_LOG(LogShootGameInstance, Error,
						TEXT("Failed to create Player02 for SplitProtagonists map %s"),
						*UWorld::RemovePIEPrefix(LoadedWorld->GetPackage()->GetName()));
					break;
				}
			}
		}
		else
		{
			// 产品边界：Player02 只属于同机分屏。Listen Host 与网络客户端每台机器只有
			// 一个 LocalPlayer；远端玩家通过网络 Login/PostLogin 加入，不能在 Host 上补造。
			UE_LOG(LogShootGameInstance, Warning,
				TEXT("SplitProtagonists ignored for non-standalone world %s (NetMode=%d)"),
				*UWorld::RemovePIEPrefix(LoadedWorld->GetPackage()->GetName()),
				static_cast<int32>(LoadedWorld->GetNetMode()));
		}
	}

	const int32 MaximumLocalPlayers =
		bPrimaryOnly ? 1 :
		2;
	if (GetNumLocalPlayers() <= MaximumLocalPlayers)
	{
		return;
	}

	// PostLoadMapWithWorld 在 UE 5.8 中发生于 SpawnPlayActor 和 World::BeginPlay 之后；
	// 此时按倒序删除超出地图策略上限的 LocalPlayer，避免数组重排，并让官方 RemovePlayer
	// 同时销毁 Pawn/Controller。SplitProtagonists 最多保留两个，产品边界与双人成行一致。
	const TArray<ULocalPlayer*> LocalPlayersSnapshot = GetLocalPlayers();
	for (int32 Index = LocalPlayersSnapshot.Num() - 1; Index >= MaximumLocalPlayers; --Index)
	{
		if (ULocalPlayer* LocalPlayer = LocalPlayersSnapshot[Index])
		{
			if (APlayerController* PlayerController = LocalPlayer->GetPlayerController(LoadedWorld))
			{
				UGameplayStatics::RemovePlayer(PlayerController, true);
			}
			else
			{
				RemoveLocalPlayer(LocalPlayer);
			}
		}
	}
}

void UShootGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver,
	ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (!World || World->GetGameInstance() != this)
	{
		return;
	}

	const UShootSessionCoordinatorSubsystem* Coordinator = GetSubsystem<UShootSessionCoordinatorSubsystem>();
	if (!bSessionReturnTravelPending && (!Coordinator || !Coordinator->IsInSessionOrTransitioning()))
	{
		return;
	}

	// ReturnToMainMenuHost 关闭远端连接时，UE 默认 HandleDisconnect 会使用 GameDefaultMap（当前为
	// FrontEndMap）。同一故障流程可能在本回调之后再次覆盖 pending travel，因此下一帧再使用同一个
	// 可配置返回图，确保正常主机销毁和意外断线都回到 HomeMap 会话入口。
	if (UShootSessionCoordinatorSubsystem* MutableCoordinator =
		GetSubsystem<UShootSessionCoordinatorSubsystem>())
	{
		MutableCoordinator->CleanUpResidualSession();
	}
	if (!bSessionReturnRedirectScheduled)
	{
		bSessionReturnRedirectScheduled = true;
		TWeakObjectPtr<UShootGameInstance> WeakThis(this);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
			[WeakThis](float)
			{
				if (UShootGameInstance* GameInstance = WeakThis.Get())
				{
					GameInstance->bSessionReturnRedirectScheduled = false;
					GameInstance->TravelToSessionReturnMap(GameInstance->GetWorld());
				}
				return false;
			}));
	}
}

bool UShootGameInstance::TravelToSessionReturnMap(UWorld* World)
{
	const FString ReturnMapPackage = SessionReturnMap.ToSoftObjectPath().GetLongPackageName();
	if (!GEngine || !World || ReturnMapPackage.IsEmpty())
	{
		return false;
	}

	GEngine->SetClientTravel(World, *ReturnMapPackage, TRAVEL_Absolute);
	return true;
}

void UShootGameInstance::TryJoinRequestedSessionAfterTravel()
{
	if (!bJoinRequestedSessionAfterTravel || !GetRequestedSession())
	{
		bJoinRequestedSessionAfterTravel = false;
		RequestedSessionPlatformUser = FPlatformUserId();
		return;
	}

	// HomeMap 加载、PlayerController 恢复以及上一个会话的异步清理必须全部完成后才能 Join。
	// 直接在 PostLoadMap 回调中 Join 会让 Steam 的旧 GameSession 与新邀请相互竞争。
	if (CanJoinRequestedSession())
	{
		JoinRequestedSessionRetryCount = 0;
		JoinRequestedSession();
		return;
	}

	if (++JoinRequestedSessionRetryCount <= 40 && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			JoinRequestedSessionRetryTimer, this, &ThisClass::TryJoinRequestedSessionAfterTravel, 0.25f, false);
	}
	else
	{
		UE_LOG(LogShootGameInstance, Error, TEXT("Platform invitation join timed out while waiting for HomeMap/session cleanup"));
		bJoinRequestedSessionAfterTravel = false;
		SetRequestedSession(nullptr);
		RequestedSessionPlatformUser = FPlatformUserId();
		if (UShootSessionCoordinatorSubsystem* Coordinator = GetSubsystem<UShootSessionCoordinatorSubsystem>())
		{
			Coordinator->ReportInvitationJoinFailure(
				NSLOCTEXT("ShootGameInstance", "InvitationJoinTimeout", "The platform invite timed out. Confirm that the previous session has ended, then try again."));
		}
	}
}
ULocalPlayer* UShootGameInstance::ResolveRequestedSessionLocalPlayer() const
{
	// 平台邀请提供有效 PlatformUserId 时优先匹配对应 LocalPlayer。Steam 桌面端通常只有一个
	// 在线用户；平台没有提供映射或目标玩家尚不存在时，保留 CommonGame 的主玩家兜底行为。
	if (ULocalPlayer* RequestedPlayer = FindLocalPlayerFromPlatformUserId(RequestedSessionPlatformUser))
	{
		return RequestedPlayer;
	}
	return GetFirstGamePlayer();
}
