// Copyright ZhaoYiJie


#include "Player/ShootPlayerController.h"

#include "Animation/ShootPoseLibraryDataAsset.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/ShootCharacter.h"
#include "Equipment/ShootQuickBarComponent.h"
#include "Feedback/NumberPops/ShootNumberPopComponent_NiagaraText.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Interaction/LyraInteractionDurationMessage.h"
#include "Messages/LyraVerbMessage.h"
#include "Player/ShootPlayerState.h"
#include "ShootGameplayTags.h"
#include "UI/Inventory/ShootHUDResourceToastComponent.h"
#include "UI/Interaction/ShootHUDInteractionComponent.h"
#include "UI/Weapons/ShootHUDReticleComponent.h"

#define LOCTEXT_NAMESPACE "ShootPlayerController"

AShootPlayerController::AShootPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ReticleComponent = CreateDefaultSubobject<UShootHUDReticleComponent>(TEXT("ReticleComponent"));
	ReticleComponent->SetIsReplicated(false);

	InteractionHUDComponent = CreateDefaultSubobject<UShootHUDInteractionComponent>(TEXT("InteractionHUDComponent"));
	InteractionHUDComponent->SetIsReplicated(false);

	ResourceToastComponent = CreateDefaultSubobject<UShootHUDResourceToastComponent>(TEXT("ResourceToastComponent"));
	ResourceToastComponent->SetIsReplicated(false);

	// 与 Lyra 的客户端 AddComponents 结果一致，但项目直接挂在自己的 Controller 上，确保每个 LocalPlayer 独享实例。
	DamageNumberComponent = CreateDefaultSubobject<UShootNumberPopComponent_NiagaraText>(TEXT("DamageNumberComponent"));
	DamageNumberComponent->SetIsReplicated(false);

	// 玩法 QuickBar 与 Controller 同生命周期；HUD 是否显示 QuickBar 由当前 Experience 资产决定。
	GameplayQuickBarComponent = CreateDefaultSubobject<UShootQuickBarComponent>(TEXT("GameplayQuickBarComponent"));
}

bool AShootPlayerController::GetPreferredCameraPerspective(EShootCameraPerspective& OutPerspective) const
{
	if (!bHasPreferredCameraPerspective)
	{
		return false;
	}

	OutPerspective = PreferredCameraPerspective;
	return true;
}

void AShootPlayerController::SetPreferredCameraPerspective(EShootCameraPerspective NewPerspective)
{
	PreferredCameraPerspective = NewPerspective;
	bHasPreferredCameraPerspective = true;
}

void AShootPlayerController::ApplyFirstPersonPitchLimits(float ViewPitchMin, float ViewPitchMax)
{
	if (!PlayerCameraManager)
	{
		return;
	}

	if (!bHasCapturedCameraPitchLimits)
	{
		CapturedViewPitchMin = PlayerCameraManager->ViewPitchMin;
		CapturedViewPitchMax = PlayerCameraManager->ViewPitchMax;
		bHasCapturedCameraPitchLimits = true;
	}
	PlayerCameraManager->ViewPitchMin = ViewPitchMin;
	PlayerCameraManager->ViewPitchMax = ViewPitchMax;
}

void AShootPlayerController::RestoreCameraPitchLimits()
{
	if (!PlayerCameraManager || !bHasCapturedCameraPitchLimits)
	{
		return;
	}

	PlayerCameraManager->ViewPitchMin = CapturedViewPitchMin;
	PlayerCameraManager->ViewPitchMax = CapturedViewPitchMax;
	bHasCapturedCameraPitchLimits = false;
}

void AShootPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// 队伍权威状态属于 PlayerState；Controller 只做代理，避免本地分屏和网络端产生两份状态。
	UE_LOG(LogTemp, Error, TEXT("Cannot set team directly on %s; set it on the associated PlayerState."),
		*GetPathNameSafe(this));
}

FGenericTeamId AShootPlayerController::GetGenericTeamId() const
{
	if (const ILyraTeamAgentInterface* PlayerStateTeamAgent = Cast<ILyraTeamAgentInterface>(PlayerState))
	{
		return PlayerStateTeamAgent->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

FOnLyraTeamIndexChangedDelegate* AShootPlayerController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

void AShootPlayerController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void AShootPlayerController::BroadcastOnPlayerStateChanged()
{
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (ILyraTeamAgentInterface* OldTeamAgent = Cast<ILyraTeamAgentInterface>(LastSeenPlayerState))
	{
		OldTeamID = OldTeamAgent->GetGenericTeamId();
		OldTeamAgent->GetTeamChangedDelegateChecked().RemoveAll(this);
	}

	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (ILyraTeamAgentInterface* NewTeamAgent = Cast<ILyraTeamAgentInterface>(PlayerState))
	{
		NewTeamID = NewTeamAgent->GetGenericTeamId();
		NewTeamAgent->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
	}

	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	LastSeenPlayerState = PlayerState;
}

void AShootPlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AShootPlayerController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AShootPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}

void AShootPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		// 监听救援交互时长消息，UI 可绑定委托显示进度条。
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		RescueDurationHandle = MessageSubsystem.RegisterListener<FLyraInteractionDurationMessage>(
			TAG_INTERACTION_DURATION_MESSAGE,
			this,
			&ThisClass::HandleRescueDurationMessage);
	}
}

void AShootPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// PlayerCameraManager 与 Controller 同生命周期。切图/退出 Experience 时即使 Pawn 已先被销毁，
	// 也要在 Controller 结束前归还第一人称临时俯仰限制，不能把它泄漏给后续视图目标。
	RestoreCameraPitchLimits();

	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		UGameplayMessageSubsystem::Get(this).UnregisterListener(RescueDurationHandle);
		RescueDurationHandle = FGameplayMessageListenerHandle();
	}

	ResetCharacterSwitchHold(false);
	Super::EndPlay(EndPlayReason);
}

void AShootPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (GameplayQuickBarComponent)
	{
		// Possess 完成后 Character 已绑定 ASC/外观；此处只恢复副本内已有的 RuntimeOnly 会话槽位。
		GameplayQuickBarComponent->RestoreRuntimeSessionToPawn(InPawn);
	}
}

void AShootPlayerController::OnUnPossess()
{
	if (GameplayQuickBarComponent)
	{
		// 必须在 Super 前读取当前 Pawn；Super 会清空 Controller 对旧 Pawn 的引用。
		GameplayQuickBarComponent->CaptureRuntimeSessionFromPawn(GetPawn());
	}

	Super::OnUnPossess();
}

void AShootPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void AShootPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	CheckTimeSync(DeltaSeconds);
	TickInteractionHold(DeltaSeconds);
	TickCharacterSwitchHold(DeltaSeconds);
}

void AShootPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
                                                                   float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

void AShootPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void AShootPlayerController::ClientReceiveReticleHitNotify_Implementation(const FShootReticleHitNotifyMessage& Message)
{
	if (UGameplayMessageSubsystem::HasInstance(this))
	{
		// 客户端收到服务器推送的命中点后，再次广播给 HUD 组件
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(
			FShootGameplayTags::Get().Msg_UI_Reticle_HitNotify, Message);
	}
}

void AShootPlayerController::ClientReceiveReticleElimination_Implementation(
	const FShootReticleEliminationMessage& Message)
{
	if (IsLocalController() && UGameplayMessageSubsystem::HasInstance(this))
	{
		// GameplayMessageSubsystem 不跨网络复制。服务器先把确认后的击杀送到拥有客户端，
		// 再在该客户端 World 内广播，避免分屏另一位 LocalPlayer 播放击杀准星动画。
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(
			FShootGameplayTags::Get().Msg_UI_Reticle_Elimination, Message);
	}
}

void AShootPlayerController::ClientAddDamageNumber_Implementation(const FShootNumberPopRequest& Request)
{
	if (IsLocalController() && DamageNumberComponent)
	{
		DamageNumberComponent->AddNumberPop(Request);
	}
}

void AShootPlayerController::ClientReceiveRespawnDuration_Implementation(
	const FLyraInteractionDurationMessage& Message)
{
	if (!IsLocalController() || !UGameplayMessageSubsystem::HasInstance(this))
	{
		return;
	}

	// Respawn 倒计时是玩家私有 HUD 消息。先通过拥有者 RPC 到达客户端，再在该客户端的 World 内广播，
	// 避免服务器广播污染其它分屏玩家，也避免把服务器 World 的 GameplayMessage 误当成网络复制。
	const FGameplayTag Channel = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Ability.Respawn.Duration.Message")), false);
	if (Channel.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Channel, Message);
	}
}

void AShootPlayerController::ClientReceiveRespawnCompleted_Implementation(
	const FLyraVerbMessage& Message)
{
	if (!IsLocalController() || !UGameplayMessageSubsystem::HasInstance(this))
	{
		return;
	}

	const FGameplayTag Channel = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Ability.Respawn.Completed.Message")), false);
	if (Channel.IsValid())
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Channel, Message);
	}
}

void AShootPlayerController::HandleRescueDurationMessage(const FGameplayTag Channel,
	const FLyraInteractionDurationMessage& Message)
{
	if (Message.Instigator != GetPawn())
	{
		// GameplayMessageSubsystem 是 World 级广播；分屏必须按本 Controller 的 Pawn 过滤，
		// 否则 P0 在补给站读条会同时驱动 P1 的进度 UI。
		return;
	}

	// Duration=0 表示交互结束，UI 需要隐藏进度条。
	LastRescueDuration = Message.Duration;
	InteractionHoldDuration = FMath::Max(0.0f, Message.Duration);
	InteractionHoldElapsed = 0.0f;
	InteractionHoldProgress = 0.0f;
	OnRescueDurationChanged.Broadcast(LastRescueDuration);
}

void AShootPlayerController::TickInteractionHold(float DeltaSeconds)
{
	if (!IsLocalController() || InteractionHoldDuration <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	InteractionHoldElapsed = FMath::Min(InteractionHoldElapsed + DeltaSeconds, InteractionHoldDuration);
	InteractionHoldProgress = FMath::Clamp(InteractionHoldElapsed / InteractionHoldDuration, 0.0f, 1.0f);
}

void AShootPlayerController::BeginCharacterSwitchHold(ECharacterGender TargetGender, float HoldDurationSeconds)
{
	if (!IsLocalController())
	{
		return;
	}

	bCharacterSwitchHoldVisible = true;
	bCharacterSwitchHoldReturning = false;
	CharacterSwitchHoldTargetGender = TargetGender;
	CharacterSwitchHoldProgress = 0.f;
	CharacterSwitchHoldElapsedSeconds = 0.f;
	CharacterSwitchHoldDurationSeconds = FMath::Max(HoldDurationSeconds, KINDA_SMALL_NUMBER);
	BroadcastCharacterSwitchHoldProgress();
}

void AShootPlayerController::CancelCharacterSwitchHold()
{
	if (!IsLocalController())
	{
		return;
	}

	if (!bCharacterSwitchHoldVisible)
	{
		return;
	}

	bCharacterSwitchHoldVisible = false;
	bCharacterSwitchHoldReturning = CharacterSwitchHoldProgress > 0.f;
	BroadcastCharacterSwitchHoldProgress();
}

void AShootPlayerController::CompleteCharacterSwitchHold()
{
	if (!IsLocalController())
	{
		return;
	}

	CharacterSwitchHoldProgress = 1.f;
	CharacterSwitchHoldElapsedSeconds = CharacterSwitchHoldDurationSeconds;
	BroadcastCharacterSwitchHoldProgress();

	// 读条完成后先把进度保持在满值，等服务器切换结果回来再统一隐藏 UI。
	bCharacterSwitchHoldVisible = false;
	bCharacterSwitchHoldReturning = false;
}

void AShootPlayerController::RequestSwitchCharacter(ECharacterGender TargetGender, AActor* SourceActor)
{
	// 菜单入口运行在本地客户端，世界入口未来也可能走本地前端；
	// 因此统一先走 Request，再由控制器决定是本地直执行业务还是发 Server RPC。
	if (HasAuthority())
	{
		TrySwitchCharacter(TargetGender, SourceActor);
		return;
	}

	ServerRequestSwitchCharacter(TargetGender, SourceActor);
}

UShootPoseLibraryDataAsset* AShootPlayerController::GetPoseLibrary() const
{
	return PoseLibrary.LoadSynchronous();
}

void AShootPlayerController::RequestPlayPoseByIndex(int32 PoseIndex)
{
	if (!IsLocalController())
	{
		return;
	}

	if (HasAuthority())
	{
		ServerRequestPlayPoseByIndex_Implementation(PoseIndex);
		return;
	}

	ServerRequestPlayPoseByIndex(PoseIndex);
}

void AShootPlayerController::ServerRequestSwitchCharacter_Implementation(ECharacterGender TargetGender, AActor* SourceActor)
{
	TrySwitchCharacter(TargetGender, SourceActor);
}

void AShootPlayerController::ServerRequestPlayPoseByIndex_Implementation(int32 PoseIndex)
{
	AShootPlayerState* ShootPS = GetPlayerState<AShootPlayerState>();
	if (!ShootPS)
	{
		ClientReceivePoseLibraryRequestResult(false, LOCTEXT("PoseCharacterDataNotReady", "Character data is not ready"));
		return;
	}

	UShootPoseLibraryDataAsset* LoadedPoseLibrary = GetPoseLibrary();
	if (!LoadedPoseLibrary || !LoadedPoseLibrary->IsPosePlayableForGender(PoseIndex, ShootPS->GetCharacterGender()))
	{
		ClientReceivePoseLibraryRequestResult(false,
			LOCTEXT("PoseUnavailableForCharacter", "The pose is unavailable or not compatible with the current character"));
		return;
	}

	const FShootPoseLibraryEntry* Entry = LoadedPoseLibrary->GetPoseEntry(PoseIndex);
	AShootCharacter* ShootCharacter = GetPawn<AShootCharacter>();
	if (!Entry || !Entry->Animation || !ShootCharacter)
	{
		ClientReceivePoseLibraryRequestResult(false, LOCTEXT("PoseResourcesNotReady", "The pose resources or character are not ready"));
		return;
	}

	// Listen Server/客户端都通过同一个多播看见姿势表现；服务器只接受 DataAsset 索引，不接受客户端随便传动画资源。
	ShootCharacter->MulticastPlayPose(Entry->Animation, Entry->SlotName, Entry->PlayRate, Entry->BlendInTime,
		Entry->BlendOutTime, Entry->LoopCount);
	ClientReceivePoseLibraryRequestResult(true, LOCTEXT("PosePlaybackStarted", "Pose playback started"));
}

void AShootPlayerController::TrySwitchCharacter(ECharacterGender TargetGender, AActor* SourceActor)
{
	if (!HasAuthority())
	{
		return;
	}

	// 当前占位入口既可能来自切换站，也可能未来来自“另一位主角 NPC”。
	// 这里先保留来源参数，后续若要补距离/朝向校验可直接用它。
	(void)SourceActor;

	AShootPlayerState* ShootPS = GetPlayerState<AShootPlayerState>();
	if (!ShootPS)
	{
		ClientReceiveCharacterSwitchResult(false, ECharacterGender::UNKNOWN,
			LOCTEXT("CharacterSwitchDataNotReady", "Character data is not ready"));
		return;
	}

	if (TargetGender == ECharacterGender::UNKNOWN || TargetGender == ShootPS->GetCharacterGender())
	{
		ClientReceiveCharacterSwitchResult(false, ShootPS->GetCharacterGender(), LOCTEXT("InvalidTargetCharacter", "Invalid target character"));
		return;
	}

	// 当前开发阶段先不把切换入口硬限制在 Hub/安全区。
	// 真正需要后续收口的是“战斗中/剧情中/特殊状态下是否允许切换”，
	// 因此这里先统一放开地图级限制，等战斗规则成型后再补更准确的门禁。

	const bool bSwitched = ShootPS->SwitchToCharacter(TargetGender);
	if (bSwitched)
	{
		// 切换成功后立即刷新快照并落盘，避免重进场景后又回到旧主角配置。
		ShootPS->CommitCurrentGenderLoadoutToSave();
		ClientReceiveCharacterSwitchResult(true, ShootPS->GetCharacterGender(), LOCTEXT("CharacterSwitchSucceeded", "Character switched successfully"));
	}
	else
	{
		ClientReceiveCharacterSwitchResult(false, ShootPS->GetCharacterGender(), LOCTEXT("CharacterSwitchFailed", "Failed to switch character"));
	}
}

void AShootPlayerController::ClientReceiveCharacterSwitchResult_Implementation(bool bSuccess, ECharacterGender CurrentGender, const FText& Message)
{
	ResetCharacterSwitchHold(true);

	OnCharacterSwitchResult.Broadcast(bSuccess, CurrentGender, Message);
	BP_OnCharacterSwitchResult(bSuccess, CurrentGender, Message);
}

void AShootPlayerController::ClientReceivePoseLibraryRequestResult_Implementation(bool bSuccess, const FText& Message)
{
	OnPoseLibraryRequestResult.Broadcast(bSuccess, Message);
	BP_OnPoseLibraryRequestResult(bSuccess, Message);
}

void AShootPlayerController::TickCharacterSwitchHold(float DeltaSeconds)
{
	if (!IsLocalController())
	{
		return;
	}

	if (bCharacterSwitchHoldVisible)
	{
		if (CharacterSwitchHoldDurationSeconds <= KINDA_SMALL_NUMBER)
		{
			CharacterSwitchHoldProgress = 1.f;
			CharacterSwitchHoldElapsedSeconds = CharacterSwitchHoldDurationSeconds;
		}
		else
		{
			CharacterSwitchHoldElapsedSeconds = FMath::Min(CharacterSwitchHoldElapsedSeconds + DeltaSeconds,
				CharacterSwitchHoldDurationSeconds);
			CharacterSwitchHoldProgress = FMath::Clamp(CharacterSwitchHoldElapsedSeconds / CharacterSwitchHoldDurationSeconds, 0.f, 1.f);
		}

		BroadcastCharacterSwitchHoldProgress();
		return;
	}

	if (bCharacterSwitchHoldReturning)
	{
		const float RollbackDuration = FMath::Max(CharacterSwitchHoldRollbackDuration, 0.01f);
		const float RollbackSpeed = 1.f / RollbackDuration;
		CharacterSwitchHoldProgress = FMath::Max(0.f, CharacterSwitchHoldProgress - DeltaSeconds * RollbackSpeed);
		CharacterSwitchHoldElapsedSeconds = CharacterSwitchHoldDurationSeconds * CharacterSwitchHoldProgress;
		BroadcastCharacterSwitchHoldProgress();

		if (CharacterSwitchHoldProgress <= 0.f)
		{
			ResetCharacterSwitchHold(true);
		}
	}
}

void AShootPlayerController::BroadcastCharacterSwitchHoldProgress()
{
	const bool bVisible = bCharacterSwitchHoldVisible || bCharacterSwitchHoldReturning;
	OnCharacterSwitchHoldProgressChanged.Broadcast(bVisible, CharacterSwitchHoldTargetGender, CharacterSwitchHoldProgress,
		CharacterSwitchHoldElapsedSeconds, CharacterSwitchHoldDurationSeconds);
	BP_OnCharacterSwitchHoldProgressChanged(bVisible, CharacterSwitchHoldTargetGender, CharacterSwitchHoldProgress,
		CharacterSwitchHoldElapsedSeconds, CharacterSwitchHoldDurationSeconds);
}

void AShootPlayerController::ResetCharacterSwitchHold(bool bBroadcastImmediately)
{
	bCharacterSwitchHoldVisible = false;
	bCharacterSwitchHoldReturning = false;
	CharacterSwitchHoldTargetGender = ECharacterGender::UNKNOWN;
	CharacterSwitchHoldProgress = 0.f;
	CharacterSwitchHoldElapsedSeconds = 0.f;
	CharacterSwitchHoldDurationSeconds = 0.f;

	if (bBroadcastImmediately && IsLocalController())
	{
		BroadcastCharacterSwitchHoldProgress();
	}
}

#undef LOCTEXT_NAMESPACE
