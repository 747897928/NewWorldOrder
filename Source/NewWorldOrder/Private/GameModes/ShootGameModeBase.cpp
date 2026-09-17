#include "GameModes/ShootGameModeBase.h"
#include "EngineUtils.h"

#include "GameModes/ShootExperienceManagerComponent.h"
#include "GameModes/ShootExperienceDefinition.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameState/ShootGameStateBase.h"
#include "AI/EnemyBotCharacter.h"
#include "Character/ShootCharacterBase.h"
#include "Engine/World.h"
#include "Engine/AssetManager.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "System/SaveGameSubsystem.h"
#include "System/ShootGameInstance.h"
#include "Player/ShootPlayerState.h"
#include "Player/ShootPlayerController.h"
#include "Interaction/LyraInteractionDurationMessage.h"
#include "Messages/LyraVerbMessage.h"
#include "UI/Weapons/ShootHitMarkerTypes.h"
#include "TimerManager.h"
#include "GameModes/ShootExpeditionLobbyComponent.h"

AShootGameModeBase::AShootGameModeBase()
{
	LevelStartingTime = 0.f;
	CurrentDifficulty = EShootDifficulty::Normal;
	FriendlyFireScalar = 0.f; // 默认无友伤
}

void AShootGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	ResolvedExperienceDefinition = ExperienceDefinition;
	const FString ExperienceName = UGameplayStatics::ParseOption(Options, TEXT("Experience"));
	if (!ExperienceName.IsEmpty())
	{
		const FPrimaryAssetId RequestedExperience(TEXT("ShootExperienceDefinition"), FName(*ExperienceName));
		FAssetData ExperienceAssetData;
		if (UAssetManager::Get().GetPrimaryAssetData(RequestedExperience, ExperienceAssetData))
		{
			ResolvedExperienceDefinition = TSoftObjectPtr<UShootExperienceDefinition>(
				FSoftObjectPath(ExperienceAssetData.GetSoftObjectPath()));
		}
		else
		{
			ResolvedExperienceDefinition.Reset();
			ErrorMessage = FString::Printf(TEXT("无法解析 URL 指定的 Experience：%s"), *ExperienceName);
		}
	}

	const FString ExpeditionMapName = UGameplayStatics::ParseOption(Options, TEXT("ExpeditionMap"));
	PendingExpeditionMapId = ExpeditionMapName.IsEmpty()
		? FPrimaryAssetId()
		: FPrimaryAssetId(TEXT("Map"), FName(*ExpeditionMapName));
	const FString ExpeditionExperienceName =
		UGameplayStatics::ParseOption(Options, TEXT("ExpeditionExperience"));
	PendingExpeditionExperienceId = ExpeditionExperienceName.IsEmpty()
		? FPrimaryAssetId()
		: FPrimaryAssetId(TEXT("ShootExperienceDefinition"), FName(*ExpeditionExperienceName));
	bPendingAllowJoinInProgress =
		FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("AllowJoinInProgress"))) != 0;
	bPendingFillEmptySlotsWithBots =
		FCString::Atoi(*UGameplayStatics::ParseOption(Options, TEXT("FillBots"))) != 0;

	const FString LocalPlayerCountOption = UGameplayStatics::ParseOption(Options, TEXT("LocalPlayers"));
	if (!LocalPlayerCountOption.IsEmpty() && !UGameplayStatics::HasOption(Options, TEXT("listen")))
	{
		LocalPlayerMapPolicy = FCString::Atoi(*LocalPlayerCountOption) >= 2
			? EShootLocalPlayerMapPolicy::SplitProtagonists
			: EShootLocalPlayerMapPolicy::PrimaryOnly;
	}

	// Lyra 保留 AGameModeBase -> AGameSession 的官方登录链：PreLogin 与 Login 都会调用
	// AGameSession::ApproveLogin。项目只在地图策略层补充容量，不手工 Spawn/Destroy 网络玩家。
	// 将参数放在最前面可覆盖 PIE 或外部启动参数里可能残留的 MaxPlayers；
	// TestMap_SplitScreen 的 SplitProtagonists 与 TestMap_ListenServer 的 KeepCurrent
	// 不改写该选项，继续维持各自产品边界。
	const FString EffectiveOptions =
		LocalPlayerMapPolicy == EShootLocalPlayerMapPolicy::PrimaryOnly &&
		!UGameplayStatics::HasOption(Options, TEXT("listen"))
			? FString(TEXT("?MaxPlayers=1")) + Options
			: Options;

	Super::InitGame(MapName, EffectiveOptions, ErrorMessage);
}

void AShootGameModeBase::InitGameState()
{
	Super::InitGameState();

	if (AShootGameStateBase* ShootGameState = GetGameState<AShootGameStateBase>())
	{
		if (UShootExperienceManagerComponent* ExperienceManager = ShootGameState->GetExperienceManagerComponent())
		{
			ExperienceManager->SetCurrentExperience(ResolvedExperienceDefinition);
		}

		if (UShootExpeditionLobbyComponent* LobbyComponent = ShootGameState->GetExpeditionLobbyComponent())
		{
			LobbyComponent->ConfigureLobby(PendingExpeditionMapId, PendingExpeditionExperienceId,
				bPendingAllowJoinInProgress, bPendingFillEmptySlotsWithBots);
		}
	}
}

void AShootGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 队伍分配属于玩法规则。PlayerState 只保存并复制 GameMode 给出的结果，不能自行猜测阵营。
	if (AShootPlayerState* ShootPlayerState = NewPlayer ? NewPlayer->GetPlayerState<AShootPlayerState>() : nullptr)
	{
		ShootPlayerState->SetGenericTeamId(FGenericTeamId(DefaultPlayerTeamId));
		// Experience 可能早于迟到玩家完成加载。此入口与 Manager 的批量授予互补，方法本身幂等。
		ShootPlayerState->ApplyExperienceAbilitySets();
		if (AShootGameStateBase* ShootGameState = GetGameState<AShootGameStateBase>())
		{
			if (UShootExpeditionLobbyComponent* Lobby = ShootGameState->GetExpeditionLobbyComponent())
			{
				Lobby->RegisterLobbyPlayer(ShootPlayerState);
			}
		}
	}
}

void AShootGameModeBase::Logout(AController* Exiting)
{
	APlayerState* ExitingPlayerState = Exiting ? Exiting->GetPlayerState<APlayerState>() : nullptr;
	if (AShootGameStateBase* ShootGameState = GetGameState<AShootGameStateBase>())
	{
		if (UShootExpeditionLobbyComponent* Lobby = ShootGameState->GetExpeditionLobbyComponent())
		{
			Lobby->UnregisterLobbyPlayer(ExitingPlayerState);
		}
	}
	Super::Logout(Exiting);
}

ECharacterGender AShootGameModeBase::ResolveInitialCharacterGender_Implementation(
	const AShootPlayerState* PlayerState, ECharacterGender SavedGender) const
{
	// 基类是空规则：Hub、联机模式和未声明规则的地图都尊重账号存档。
	return SavedGender;
}

bool AShootGameModeBase::ShouldPersistLastActiveGender_Implementation(
	const AShootPlayerState* PlayerState) const
{
	return true;
}

void AShootGameModeBase::PlayerDied(ACharacter* DeadCharacter)
{
	if (!HasAuthority() || !DeadCharacter)
	{
		return;
	}

	AEnemyBotCharacter* DeadEnemy = Cast<AEnemyBotCharacter>(DeadCharacter);
	if (DeadEnemy)
	{
		// 敌人复活属于地图玩法规则。Character 只负责进入死亡状态并提供初始出生点，
		// GameMode 决定本地图是否清理尸体、何时重新生成，不把重生策略塞进伤害 GA。
		if (bRespawnEnemyBots)
		{
			const TSubclassOf<AEnemyBotCharacter> EnemyClass = DeadEnemy->GetClass();
			const FTransform SpawnTransform = DeadEnemy->GetInitialSpawnTransform();
			FTimerDelegate RespawnDelegate = FTimerDelegate::CreateUObject(
				this, &ThisClass::RespawnEnemyBot, EnemyClass, SpawnTransform);
			FTimerHandle RespawnTimerHandle;
			GetWorldTimerManager().SetTimer(
				RespawnTimerHandle, RespawnDelegate, FMath::Max(EnemyRespawnDelay, 0.01f), false);
		}

		DeadEnemy->FinishDeath();
		if (EnemyCorpseLifeSpan <= 0.0f)
		{
			DeadEnemy->Destroy();
		}
		else
		{
			DeadEnemy->SetLifeSpan(EnemyCorpseLifeSpan);
		}
		return;
	}

	AController* DeadController = DeadCharacter->GetController();
	if (!DeadController || !DeadController->IsPlayerController())
	{
		return;
	}

	if (AShootCharacterBase* ShootCharacter = Cast<AShootCharacterBase>(DeadCharacter))
	{
		ShootCharacter->FinishDeath();
	}

	if (!bRespawnPlayers)
	{
		return;
	}

	const float RespawnDelay = FMath::Max(PlayerRespawnDelay, 0.01f);
	BroadcastRespawnDuration(DeadController, RespawnDelay);

	// OnUnPossess 会先把 RuntimeOnly QuickBar Guid 暂存到 PlayerController；RestartPlayer 完成 Possess 后
	// OnPossess 再恢复到新 Pawn，避免死亡清理把本局拾取武器误当成账号存档或直接丢失。
	DeadController->UnPossess();
	if (PlayerCorpseLifeSpan <= 0.0f)
	{
		DeadCharacter->Destroy();
	}
	else
	{
		DeadCharacter->SetLifeSpan(PlayerCorpseLifeSpan);
	}

	const TWeakObjectPtr<AController> WeakController(DeadController);
	FTimerDelegate RespawnDelegate = FTimerDelegate::CreateWeakLambda(this, [this, WeakController]()
	{
		// 对齐 Lyra：延迟结束后再排到下一帧执行 Restart，给死亡 Pawn 的网络清理和 Ability 结束留出边界。
		RequestPlayerRestartNextFrame(WeakController.Get(), false);
	});
	FTimerHandle RespawnTimerHandle;
	GetWorldTimerManager().SetTimer(
		RespawnTimerHandle, RespawnDelegate, RespawnDelay, false);
}

void AShootGameModeBase::RequestPlayerRestartNextFrame(AController* Controller, bool bForceReset)
{
	if (!HasAuthority() || !IsValid(Controller))
	{
		return;
	}

	if (bForceReset)
	{
		Controller->Reset();
	}

	const TWeakObjectPtr<AController> WeakController(Controller);
	GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this, WeakController]()
		{
			RespawnPlayer(WeakController.Get());
		}));
}

bool AShootGameModeBase::RespawnPlayer(AController* PlayerController)
{
	if (!HasAuthority() || !IsValid(PlayerController) || PlayerController->GetPawn())
	{
		return false;
	}

	RestartPlayer(PlayerController);
	if (!PlayerController->GetPawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("Respawn failed for %s: RestartPlayer produced no Pawn."),
			*GetNameSafe(PlayerController));
		return false;
	}

	// ASC 与 AttributeSet 位于 PlayerState，会跨 Pawn 重生继续存在。必须在新 Avatar 已绑定后恢复当前值，
	// 否则新 Pawn 会继承死亡时的 Health=0；这里只恢复战斗即时值，不重读或改写 SaveGame 属性成长。
	if (AShootPlayerState* ShootPlayerState = PlayerController->GetPlayerState<AShootPlayerState>())
	{
		if (UAbilitySystemComponent* ASC = ShootPlayerState->GetAbilitySystemComponent())
		{
			const UShootAttributeSet* Attributes = Cast<UShootAttributeSet>(ShootPlayerState->GetAttributeSet());
			if (Attributes)
			{
				ASC->SetNumericAttributeBase(UShootAttributeSet::GetHealthAttribute(), Attributes->GetMaxHealth());
				ASC->SetNumericAttributeBase(UShootAttributeSet::GetShieldAttribute(), Attributes->GetShieldCapacity());
				ASC->SetNumericAttributeBase(UShootAttributeSet::GetIncomingDamageAttribute(), 0.0f);
			}
		}
	}

	BroadcastRespawnCompleted(PlayerController);
	return true;
}

void AShootGameModeBase::BroadcastRespawnDuration(AController* PlayerController, float Duration)
{
	AShootPlayerController* ShootPlayerController = Cast<AShootPlayerController>(PlayerController);
	AShootPlayerState* ShootPlayerState = PlayerController
		? PlayerController->GetPlayerState<AShootPlayerState>()
		: nullptr;
	if (!ShootPlayerController || !ShootPlayerState)
	{
		return;
	}

	FLyraInteractionDurationMessage Message;
	Message.Instigator = ShootPlayerState;
	Message.Duration = Duration;
	ShootPlayerController->ClientReceiveRespawnDuration(Message);
}

void AShootGameModeBase::BroadcastRespawnCompleted(AController* PlayerController)
{
	AShootPlayerController* ShootPlayerController = Cast<AShootPlayerController>(PlayerController);
	AShootPlayerState* ShootPlayerState = PlayerController
		? PlayerController->GetPlayerState<AShootPlayerState>()
		: nullptr;
	if (!ShootPlayerController || !ShootPlayerState)
	{
		return;
	}

	FLyraVerbMessage Message;
	Message.Verb = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Ability.Respawn.Completed.Message")), false);
	Message.Instigator = ShootPlayerState;
	Message.Magnitude = 1.0;
	ShootPlayerController->ClientReceiveRespawnCompleted(Message);
}

void AShootGameModeBase::RespawnEnemyBot(
	TSubclassOf<AEnemyBotCharacter> EnemyClass, FTransform SpawnTransform)
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || !EnemyClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AEnemyBotCharacter* RespawnedEnemy = World->SpawnActor<AEnemyBotCharacter>(
		EnemyClass, SpawnTransform, SpawnParameters);
	if (RespawnedEnemy && !RespawnedEnemy->GetController())
	{
		// 蓝图的 Auto Possess AI 通常会在生成时创建 Controller；无 Controller 时补齐同一条 AI 初始化链。
		RespawnedEnemy->SpawnDefaultController();
	}
}

UShootSaveGame* AShootGameModeBase::RetrieveInGameSaveData()
{
	USaveGameSubsystem* SaveGameSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>();
	UShootSaveGame* CurrentSaveGame = SaveGameSubsystem->GetCurrentSaveGame();
	if (CurrentSaveGame)
	{
		return CurrentSaveGame;
	}
	else
	{
		return SaveGameSubsystem->LoadPlayerSaveGame(SaveGameSubsystem->GetCurrentSlotIndex());
	}
}

void AShootGameModeBase::InitializeRoundForAll()
{
	if (!HasAuthority())
	{
		return;
	}

	// 玩家：只重置回合属性。Experience 技能属于 Match，不能每回合撤销重授而重置冷却/被动状态。
	// 敌人：整场生成的敌人按初始属性重新初始化(回合制副本每次开局)。
	if (const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		for (const TObjectPtr<APlayerState>& PS : GS->PlayerArray)
		{
			if (const AShootPlayerState* ShootPS = Cast<AShootPlayerState>(PS))
			{
				if (AShootCharacterBase* Base = Cast<AShootCharacterBase>(ShootPS->GetPawn()))
				{
					Base->ResetAndInitializeDefaultAttributes();
				}
			}
		}
	}

	for (TActorIterator<AEnemyBotCharacter> It(GetWorld()); It; ++It)
	{
		It->ResetAndInitializeDefaultAttributes();
	}
}

float AShootGameModeBase::GetFriendlyFireScalar() const
{
	return FriendlyFireScalar;
}

float AShootGameModeBase::GetFriendlyFireScalarForActors(const AActor* InstigatorActor, const AActor* TargetActor) const
{
	if (!InstigatorActor || !TargetActor)
	{
		return 1.0f;
	}

	// 队伍解析统一走项目已有 AbilitySystemLibrary；GameMode 只决定友伤规则，不再维护第二套解析器。
	if (UShootAbilitySystemLibrary::GetTeamAttitudeForActors(InstigatorActor, TargetActor) == ETeamAttitude::Friendly)
	{
		return FriendlyFireScalar;
	}

	return 1.0f;
}

void AShootGameModeBase::NotifyCharacterKilled_Implementation(AActor* Killer, AActor* Victim)
{
	if (!HasAuthority() || !Victim)
	{
		return;
	}

	// OnCharacterKilled 是服务器规则委托；准星是玩家私有 UI，必须经击杀者 Controller 的
	// Client RPC 进入正确客户端，不能直接在服务器 World 的 GameplayMessageSubsystem 广播。
	APawn* KillerPawn = Cast<APawn>(Killer);
	AController* KillerController = Cast<AController>(Killer);
	if (!KillerController && KillerPawn)
	{
		KillerController = KillerPawn->GetController();
	}
	if (!KillerPawn && KillerController)
	{
		KillerPawn = KillerController->GetPawn();
	}

	if (AShootPlayerController* ShootPlayerController = Cast<AShootPlayerController>(KillerController))
	{
		FShootReticleEliminationMessage Message;
		Message.Instigator = KillerPawn;
		Message.Victim = Victim;
		ShootPlayerController->ClientReceiveReticleElimination(Message);
	}

	OnCharacterKilled.Broadcast(Killer, Victim);
}

void AShootGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	// 正常 Travel 由 GameInstance 的 PostLoadMapWithWorld 执行地图策略；PIE 通过复制编辑器世界创建，
	// UE 5.8 不保证触发该委托，因此权威 GameMode 在 BeginPlay 再补一次幂等调用。
	// Player02 仍由 CreatePlayer 进入引擎 Login/RestartPlayer，不在 GameMode 手工 Spawn/Possess。
	if (UShootGameInstance* ShootGameInstance = GetGameInstance<UShootGameInstance>())
	{
		ShootGameInstance->ApplyLocalPlayerMapPolicy(GetWorld());
	}

	//获取我们的关卡开始时间，也就是我们刚进入关卡时的时间
	LevelStartingTime = GetWorld()->GetTimeSeconds();

	if (FriendlyFireByDifficulty.Contains(CurrentDifficulty))
	{
		FriendlyFireScalar = FriendlyFireByDifficulty[CurrentDifficulty];
	}

}
