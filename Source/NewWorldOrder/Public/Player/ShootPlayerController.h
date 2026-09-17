// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterGender.h"
#include "Camera/ShootCameraTypes.h"
#include "CommonPlayerController.h"
#include "GameplayTagContainer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Feedback/NumberPops/ShootNumberPopComponent.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "UI/Weapons/ShootHitMarkerTypes.h"
#include "ShootPlayerController.generated.h"

class UShootHUDReticleComponent;
class UShootHUDInteractionComponent;
class UShootHUDResourceToastComponent;
class UShootNumberPopComponent_NiagaraText;
class UShootPoseLibraryDataAsset;
class UShootQuickBarComponent;
class APlayerState;

struct FLyraInteractionDurationMessage;
struct FLyraVerbMessage;

/**
 * 
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class NEWWORLDORDER_API AShootPlayerController : public ACommonPlayerController, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCharacterSwitchResult, bool, bSuccess,
	                                                ECharacterGender, CurrentGender, FText, Message);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnCharacterSwitchHoldProgressChanged, bool, bVisible,
		ECharacterGender, TargetGender, float, NormalizedProgress, float, ElapsedSeconds, float, HoldDuration);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPoseLibraryRequestResult, bool, bSuccess, FText, Message);

public:

	AShootPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Controller 不拥有独立队伍状态，而是像 Lyra 一样代理关联 PlayerState，供 LocalPlayer、UI 和感知统一观察。
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void InitPlayerState() override;
	virtual void CleanupPlayerState() override;
	virtual void OnRep_PlayerState() override;
	
	/**
	 * Sync time between client and server
	 * Requests the current server time, passing in the client's time when the request was sent
	 */
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeOfClientRequest);

	// Reports the current server time to the client in response to ServerRequestServerTime
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeOfClientRequest, float TimeServerReceivedClientRequest);

	float ClientServerDelta = 0.f; // difference between client and server time

	UPROPERTY(EditAnywhere, Category = Time)
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;
	
	void CheckTimeSync(float DeltaTime);

public:
	virtual void Tick(float DeltaSeconds) override;

	// 救援进度变化通知（UI 可绑定）
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRescueDurationChanged, float, DurationSeconds);

	UPROPERTY(BlueprintAssignable, Category="UI|Rescue")
	FOnRescueDurationChanged OnRescueDurationChanged;

	UFUNCTION(BlueprintCallable, Category="UI|Rescue")
	float GetLastRescueDuration() const { return LastRescueDuration; }

	UFUNCTION(BlueprintPure, Category="UI|Interaction")
	bool IsInteractionHoldVisible() const { return InteractionHoldDuration > 0.0f; }

	UFUNCTION(BlueprintPure, Category="UI|Interaction")
	float GetInteractionHoldProgress() const { return InteractionHoldProgress; }
	
	UFUNCTION(Client, Reliable)
	void ClientReceiveReticleHitNotify(const FShootReticleHitNotifyMessage& Message);

	/** 服务器确认目标首次死亡后，只通知实际击杀者的拥有客户端；客户端再广播给该 LocalPlayer 的准星。 */
	UFUNCTION(Client, Reliable)
	void ClientReceiveReticleElimination(const FShootReticleEliminationMessage& Message);

	/** 服务器最终扣血后只通知造成伤害的拥有客户端；纯表现消息不占用 Reliable 队列。 */
	UFUNCTION(Client, Unreliable)
	void ClientAddDamageNumber(const FShootNumberPopRequest& Request);

	/**
	 * Lyra Respawn 消息桥：服务器只把倒计时发送给对应拥有客户端，客户端再广播到本地 GameplayMessageSubsystem。
	 * GameplayMessageSubsystem 本身不跨网络复制，不能从服务器 World 直接广播后期待客户端收到。
	 */
	UFUNCTION(Client, Reliable)
	void ClientReceiveRespawnDuration(const FLyraInteractionDurationMessage& Message);

	/** Lyra Verb Message 的 Respawn 完成桥，消息体保持与 W_RespawnTimer 的监听类型一致。 */
	UFUNCTION(Client, Reliable)
	void ClientReceiveRespawnCompleted(const FLyraVerbMessage& Message);

	UPROPERTY(BlueprintAssignable, Category="UI|CharacterSwitch")
	FOnCharacterSwitchResult OnCharacterSwitchResult;

	UPROPERTY(BlueprintAssignable, Category="UI|CharacterSwitch")
	FOnCharacterSwitchHoldProgressChanged OnCharacterSwitchHoldProgressChanged;

	UPROPERTY(BlueprintAssignable, Category="UI|PoseLibrary")
	FOnPoseLibraryRequestResult OnPoseLibraryRequestResult;

	UFUNCTION(BlueprintImplementableEvent, Category="UI|CharacterSwitch")
	void BP_OnCharacterSwitchResult(bool bSuccess, ECharacterGender CurrentGender, const FText& Message);

	UFUNCTION(BlueprintImplementableEvent, Category="UI|CharacterSwitch")
	void BP_OnCharacterSwitchHoldProgressChanged(bool bVisible, ECharacterGender TargetGender, float NormalizedProgress,
	                                             float ElapsedSeconds, float HoldDuration);

	UFUNCTION(BlueprintImplementableEvent, Category="UI|PoseLibrary")
	void BP_OnPoseLibraryRequestResult(bool bSuccess, const FText& Message);

	UFUNCTION(BlueprintCallable, Category="PoseLibrary")
	UShootPoseLibraryDataAsset* GetPoseLibrary() const;

	/** Controller 侧 QuickBar 主线入口，后续角色切换与 HUD 都通过此组件获取当前武器。 */
	UFUNCTION(BlueprintPure, Category="Inventory|Quickbar")
	UShootQuickBarComponent* GetGameplayQuickBarComponent() const { return GameplayQuickBarComponent; }

	/**
	 * 本地视角偏好与 PlayerController 同生命周期，死亡重生换 Pawn 时不会退回 Experience 默认值。
	 * 不复制：每个 LocalPlayer 独立保存自己的视觉选择，服务器不需要知道普通第一/第三人称偏好。
	 */
	bool GetPreferredCameraPerspective(EShootCameraPerspective& OutPerspective) const;
	void SetPreferredCameraPerspective(EShootCameraPerspective NewPerspective);

	/** PlayerCameraManager 与 Controller 同生命周期；俯仰原值必须保存在这里，不能保存在死亡时销毁的 Pawn。 */
	void ApplyFirstPersonPitchLimits(float ViewPitchMin, float ViewPitchMax);
	void RestoreCameraPitchLimits();

	/** 姿势库 Widget 点击条目后调用。客户端只传索引，服务器按 PoseLibrary DataAsset 校验。 */
	UFUNCTION(BlueprintCallable, Category="PoseLibrary")
	void RequestPlayPoseByIndex(int32 PoseIndex);

	/** 长按开始时由交互 GA 调用，UI 可直接绑定进度委托。这里只负责表现层状态，不负责真正切换。 */
	void BeginCharacterSwitchHold(ECharacterGender TargetGender, float HoldDurationSeconds);

	/** 长按中途松开或失去目标时调用，让进度条回落。 */
	void CancelCharacterSwitchHold();

	/** 长按完成后调用，先把进度补满，随后等待服务器切换结果回包再统一收尾。 */
	void CompleteCharacterSwitchHold();

	/** 统一的角色切换请求入口：菜单入口与世界入口都先调这里，再由控制器转发到服务器权威逻辑。 */
	UFUNCTION(BlueprintCallable, Category="Character|Switch")
	void RequestSwitchCharacter(ECharacterGender TargetGender, AActor* SourceActor);

	/** 大厅页面的设备无关 Ready 入口；拥有客户端只提交布尔意图，服务器从本 Controller 解析身份。 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Lobby")
	void SetExpeditionLobbyReady(bool bReady);

	UFUNCTION(BlueprintCallable, Category="Character|Switch")
	bool IsCharacterSwitchHoldVisible() const { return bCharacterSwitchHoldVisible || bCharacterSwitchHoldReturning; }

	UFUNCTION(BlueprintCallable, Category="Character|Switch")
	ECharacterGender GetCharacterSwitchHoldTarget() const { return CharacterSwitchHoldTargetGender; }

	UFUNCTION(BlueprintCallable, Category="Character|Switch")
	float GetCharacterSwitchHoldProgress() const { return CharacterSwitchHoldProgress; }

	UFUNCTION(BlueprintCallable, Category="Character|Switch")
	float GetCharacterSwitchHoldElapsedSeconds() const { return CharacterSwitchHoldElapsedSeconds; }

	UFUNCTION(BlueprintCallable, Category="Character|Switch")
	float GetCharacterSwitchHoldDurationSeconds() const { return CharacterSwitchHoldDurationSeconds; }

	float GetDefaultCharacterSwitchHoldDuration() const { return CharacterSwitchHoldTriggerDuration; }

private:
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	void BroadcastOnPlayerStateChanged();

	UFUNCTION(Server, Reliable)
	void ServerRequestSwitchCharacter(ECharacterGender TargetGender, AActor* SourceActor);

	UFUNCTION(Server, Reliable)
	void ServerRequestPlayPoseByIndex(int32 PoseIndex);

	UFUNCTION(Server, Reliable)
	void ServerSetExpeditionLobbyReady(bool bReady);

	/** 真正的服务器权威切换实现。外部不应直接依赖它，统一走 RequestSwitchCharacter。 */
	void TrySwitchCharacter(ECharacterGender TargetGender, AActor* SourceActor);

	void HandleRescueDurationMessage(const FGameplayTag Channel, const FLyraInteractionDurationMessage& Message);
	void TickInteractionHold(float DeltaSeconds);
	void TickCharacterSwitchHold(float DeltaSeconds);
	void BroadcastCharacterSwitchHoldProgress();
	void ResetCharacterSwitchHold(bool bBroadcastImmediately);

	UFUNCTION(Client, Reliable)
	void ClientReceiveCharacterSwitchResult(bool bSuccess, ECharacterGender CurrentGender, const FText& Message);

	UFUNCTION(Client, Reliable)
	void ClientReceivePoseLibraryRequestResult(bool bSuccess, const FText& Message);

	FGameplayMessageListenerHandle RescueDurationHandle;
	float LastRescueDuration = 0.f;
	float InteractionHoldDuration = 0.0f;
	float InteractionHoldElapsed = 0.0f;
	float InteractionHoldProgress = 0.0f;

	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;

	UPROPERTY(Transient)
	EShootCameraPerspective PreferredCameraPerspective = EShootCameraPerspective::ThirdPerson;

	UPROPERTY(Transient)
	bool bHasPreferredCameraPerspective = false;

	bool bHasCapturedCameraPitchLimits = false;
	float CapturedViewPitchMin = -89.0f;
	float CapturedViewPitchMax = 89.0f;

	UPROPERTY(EditDefaultsOnly, Category="Character|Switch")
	float CharacterSwitchHoldTriggerDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category="Character|Switch")
	float CharacterSwitchHoldRollbackDuration = 0.18f;

	/** 姿势库数据源。BP_ShootPlayerController 设置此 DataAsset；WBP_GameMenu 的 PoseLibraryUniformGrid 在蓝图中读取它生成姿势卡片。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|PoseLibrary", meta=(AllowPrivateAccess="true"))
	TSoftObjectPtr<UShootPoseLibraryDataAsset> PoseLibrary;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Switch", meta=(AllowPrivateAccess="true"))
	bool bCharacterSwitchHoldVisible = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Switch", meta=(AllowPrivateAccess="true"))
	bool bCharacterSwitchHoldReturning = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Switch", meta=(AllowPrivateAccess="true"))
	ECharacterGender CharacterSwitchHoldTargetGender = ECharacterGender::UNKNOWN;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Switch", meta=(AllowPrivateAccess="true"))
	float CharacterSwitchHoldProgress = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Switch", meta=(AllowPrivateAccess="true"))
	float CharacterSwitchHoldElapsedSeconds = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Character|Switch", meta=(AllowPrivateAccess="true"))
	float CharacterSwitchHoldDurationSeconds = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootHUDReticleComponent> ReticleComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootHUDInteractionComponent> InteractionHUDComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootHUDResourceToastComponent> ResourceToastComponent;

	/** 每个 PlayerController 独享的 Lyra Niagara 伤害数字入口，具体样式由 BP_ShootPlayerController 配置。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="UI", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootNumberPopComponent_NiagaraText> DamageNumberComponent;

	/**
	 * 与 PlayerController 同生命周期的玩法 QuickBar。
	 * 它只管理玩家槽位/装备；当前模式显示哪些 HUD 片段由 Experience DataAsset 决定。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory|Quickbar", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootQuickBarComponent> GameplayQuickBarComponent;
};
