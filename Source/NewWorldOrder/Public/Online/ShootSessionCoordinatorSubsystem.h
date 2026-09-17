// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonSessionSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShootSessionCoordinatorSubsystem.generated.h"

class APlayerController;
class FOnlineSessionSearch;
class UCommonSession_SearchResult;
class UCommonSession_SearchSessionRequest;
struct FOnlineResultInformation;

/**
 * 会话页面使用的原始运行时数据。这里只描述平台事实，不包含展示标签或排版；
 * WBP_SessionScreen 决定文本组合、控件和响应式布局。
 */
USTRUCT(BlueprintType)
struct FShootSessionRuntimeInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Session")
	bool bIsInSession = false;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	FString PlatformName;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	FString SessionName;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	FString SessionState;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	FString RoleName;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	FString MapName;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	int32 BuildId = 0;
};

/** GameState.PlayerArray 的单个玩家原始数据；玩家行视觉完全由 Widget Blueprint 决定。 */
USTRUCT(BlueprintType)
struct FShootSessionPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Session")
	int32 DisplayIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	float PingMilliseconds = 0.0f;
};

UENUM(BlueprintType)
enum class EShootSessionLifecycleState : uint8
{
	Idle,
	CleaningUp,
	Hosting,
	Searching,
	Results,
	Joining,
	InSession,
	Leaving,
	Error
};

UENUM(BlueprintType)
enum class EShootSessionRole : uint8
{
	None,
	Host,
	Client
};

DECLARE_MULTICAST_DELEGATE(FShootSessionCoordinatorChanged);

/**
 * 当前项目对 UCommonSessionSubsystem 的 UI 状态适配层。
 *
 * 调用链：MainMenu -> SessionScreen -> 本协调层 -> UCommonSessionSubsystem。
 * 本类保存 UObject 请求和可展示状态，不长期持有 IOnlineSessionPtr 或 IOnlineFriendsPtr。
 * Host、Join、销毁与邀请接受仍由 CommonUser 负责；项目层只为 SteamDevAppId 480 的公共 Lobby 池
 * 临时注册一次 FindSessions 委托，把扩大后的结果重新包装为 CommonSession SearchResult。
 */
UCLASS(Config=Game, DefaultConfig)
class NEWWORLDORDER_API UShootSessionCoordinatorSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	bool HostSession(APlayerController* PlayerController, ECommonSessionOnlineMode OnlineMode,
	                 const FPrimaryAssetId& MapId, int32 MaxPlayers, const FString& AdvertisedMode);
	/** 接收 UserFacingExperience 生成的完整请求，保留 LobbyMap、Experience 和额外 URL 参数。 */
	bool HostSessionRequest(APlayerController* PlayerController, UCommonSession_HostSessionRequest* Request);
	/** 房主开始副本前更新平台会话的中途加入策略；不长期持有 OSS interface。 */
	bool SetJoinInProgressAllowed(bool bAllowed);
	bool FindSessions(APlayerController* PlayerController, ECommonSessionOnlineMode OnlineMode);
	bool JoinSession(APlayerController* PlayerController, UCommonSession_SearchResult* SearchResult);
	bool LeaveOrDestroySession(APlayerController* PlayerController);
	bool CleanUpResidualSession();
	bool ShowInviteUI(APlayerController* PlayerController);
	bool RefreshPlatformSessionState();
	/** 平台邀请在 Join 发起前失败或等待超时时，把错误收敛回同一 Session UI 状态机。 */
	void ReportInvitationJoinFailure(const FText& ErrorMessage);

	UFUNCTION(BlueprintPure, Category="Session|Status")
	FShootSessionRuntimeInfo GetRuntimeSessionInfo() const;

	UFUNCTION(BlueprintPure, Category="Session|Status")
	TArray<FShootSessionPlayerInfo> GetRuntimePlayerInfo() const;

	EShootSessionLifecycleState GetState() const { return State; }
	EShootSessionRole GetRole() const { return Role; }
	ECommonSessionOnlineMode GetOnlineMode() const { return CurrentOnlineMode; }
	const FText& GetStatusText() const { return StatusText; }
	const TArray<TObjectPtr<UCommonSession_SearchResult>>& GetSearchResults() const { return SearchResults; }
	bool IsInSessionOrTransitioning() const;

	FShootSessionCoordinatorChanged OnCoordinatorChanged;

private:
	void SetState(EShootSessionLifecycleState NewState, const FText& NewStatus);
	void HandleSearchFinished(bool bSucceeded, const FText& ErrorMessage);
	void HandleCreateSessionComplete(const FOnlineResultInformation& Result);
	void HandleJoinSessionComplete(const FOnlineResultInformation& Result);
	void HandleSessionInformationChanged(ECommonSessionInformationState SessionStatus, const FString& GameMode,
	                                     const FString& MapName);
	void HandlePreClientTravel(FString& URL);
	bool StartExpandedSteamSearch(APlayerController* PlayerController);
	void HandleExpandedSteamSearchComplete(bool bWasSuccessful);
	void ClearExpandedSteamSearchDelegate();
	void PollCleanupCompletion();
	bool HasPlatformSession() const;
	bool CanStartSessionRequest() const;

#if WITH_DEV_AUTOMATION_TESTS
	/**
	 * Development/Editor 独立进程回归入口。
	 * 仅当命令行显式提供 -ShootSessionAutomation 与 -ShootSessionAutomationMap 时启动，
	 * 通过本协调器的公开主线验证 Host/Find/Join/Leave/Destroy，不进入 Shipping。
	 */
	void InitializeCommandLineAutomation();
	void HandleAutomationPostLoadMap(UWorld* LoadedWorld);
	void RunAutomationStep();
	void FinishAutomation(const TCHAR* Result);

	FString AutomationRole;
	FPrimaryAssetId AutomationMapId;
	FDelegateHandle AutomationPostLoadMapHandle;
	FTimerHandle AutomationTimerHandle;
	double AutomationActionNotBeforeSeconds = 0.0;
	int32 AutomationRetryCount = 0;
	bool bAutomationHostRequested = false;
	bool bAutomationSearchRequested = false;
	bool bAutomationJoinRequested = false;
	bool bAutomationWasInSession = false;
	bool bAutomationRemotePlayerObserved = false;
	bool bAutomationLeaveRequested = false;
#endif

	UPROPERTY(Transient)
	TObjectPtr<UCommonSessionSubsystem> CommonSessionSubsystem;

	UPROPERTY(Transient)
	TObjectPtr<UCommonSession_SearchSessionRequest> ActiveSearchRequest;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCommonSession_SearchResult>> SearchResults;

	/**
	 * SteamDevAppId 480 被所有 Spacewar 测试项目共享；CommonUser OSSv1 固定只取 10 条时，
	 * 当前项目 Lobby 很容易被公共结果挤出。这里只扩展搜索窗口，不改变 Host/Join/Cleanup 的 CommonSession 主线。
	 */
	UPROPERTY(Config)
	int32 SteamSearchMaxResults = 1000;

	TSharedPtr<FOnlineSessionSearch> ActiveExpandedSteamSearch;

	EShootSessionLifecycleState State = EShootSessionLifecycleState::Idle;
	EShootSessionRole Role = EShootSessionRole::None;
	ECommonSessionOnlineMode CurrentOnlineMode = ECommonSessionOnlineMode::Offline;
	FText StatusText;

	FDelegateHandle CreateSessionCompleteHandle;
	FDelegateHandle JoinSessionCompleteHandle;
	FDelegateHandle SessionInformationChangedHandle;
	FDelegateHandle PreClientTravelHandle;
	/** 仅在一次扩展 Steam 搜索存活期内有效，完成、失败或 Deinitialize 都会立即解除。 */
	FDelegateHandle ExpandedSteamSearchCompleteHandle;
	FTimerHandle CleanupPollTimerHandle;
	int32 CleanupPollCount = 0;
	/** 当前 Join 结果广告的地图包名；只用于补全 OSS 返回的无地图连接地址。 */
	FString PendingJoinMapName;
};
