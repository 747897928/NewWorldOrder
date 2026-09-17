// Copyright ZhaoYiJie


#include "AI/EnemyBotController.h"

#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AI/EnemyBotCharacter.h"
#include "AI/Zombie/ShootMeleeEngagementSubsystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Interface/CombatInterface.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
// EnemyBotController.cpp

const FName AEnemyBotController::TargetActorBlackboardKey(TEXT("TargetActor"));
const FName AEnemyBotController::TargetApproachLocationBlackboardKey(TEXT("TargetApproachLocation"));
const FName AEnemyBotController::HomeLocationBlackboardKey(TEXT("HomeLocation"));
const FName AEnemyBotController::PatrolLocationBlackboardKey(TEXT("PatrolLocation"));

AEnemyBotController::AEnemyBotController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// AI 不需要 PlayerState
	bWantsPlayerState = false;

	// 参考 Lyra ShooterCore Bot：控制器负责感知，Behavior Tree 只消费黑板结果。
	// 感知先接收全部阵营，再由统一 Team Attitude 解析器筛出 Hostile，避免 NoTeam 被误判为敌人。
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	// TestMap 是空旷平面；过大的视野会让远处整群僵尸同时产生攻击欲望。
	// 感知只负责候选收集，具体敌人仍受各 archetype AggroRange 的二次限制；
	// LoseSight 保留少量迟滞，避免玩家在边缘移动时黑板目标反复抖动。
	SightConfig->SightRadius = 2200.0f;
	SightConfig->LoseSightRadius = 2500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 90.0f;
	SightConfig->SetMaxAge(3.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerceptionComponent->ConfigureSense(*SightConfig);
	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(3.0f);
	AIPerceptionComponent->ConfigureSense(*DamageConfig);
	AIPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
	SetPerceptionComponent(*AIPerceptionComponent);
}

void AEnemyBotController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	const FGenericTeamId Old = CachedTeamID;
	CachedTeamID = NewTeamID;

	// 广播（Lyra 风格）
	ILyraTeamAgentInterface::ConditionalBroadcastTeamChanged(this, Old, CachedTeamID);
	if (AIPerceptionComponent && Old != CachedTeamID)
	{
		// 阵营变化后重新评估已缓存刺激，避免感知组件继续使用旧的 affiliation 结果。
		AIPerceptionComponent->RequestStimuliListenerUpdate();
	}
}

ETeamAttitude::Type AEnemyBotController::GetTeamAttitudeTowards(const AActor& Other) const
{
	// 与伤害、技能筛选、Spawner 共用同一解析器：同队 Friendly、异队 Hostile、缺少可靠
	// Team/Faction 信息时 Neutral。Neutral 默认不会被测试 AI 主动选择，不能再把 NoTeam 猜成敌人。
	return UShootAbilitySystemLibrary::GetTeamAttitudeForActors(this, &Other);
}

bool AEnemyBotController::CanAttemptMeleeAttack(AActor* Target) const
{
	if (!Target || RememberedHostileTarget.Get() != Target || !GetPawn())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const UShootMeleeEngagementSubsystem* EngagementSubsystem = World
		? World->GetSubsystem<UShootMeleeEngagementSubsystem>()
		: nullptr;
	// 没有世界子系统只可能发生在编辑器构造/拆除阶段；运行时保留攻击能力的降级路径。
	return !EngagementSubsystem || EngagementSubsystem->HasSlot(Target, GetPawn());
}

void AEnemyBotController::BeginPlay()
{
	Super::BeginPlay();

	// 具体队伍由 Controller 蓝图 Class Defaults / Experience 配置；当前默认值 2 是丧尸阵营。
	SetGenericTeamId(FGenericTeamId(DefaultTeamId));
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(
		this, &ThisClass::HandleTargetPerceptionUpdated);
}

void AEnemyBotController::OnPossess(APawn* InPawn)
{
	ResetDecisionState(true);
	if (AEnemyBotCharacter* PreviousEnemy = ObservedEnemyPawn.Get())
	{
		PreviousEnemy->GetOnDeathDelegate().RemoveDynamic(
			this, &ThisClass::HandleControlledPawnDeath);
	}
	ObservedEnemyPawn.Reset();

	Super::OnPossess(InPawn);

	AEnemyBotCharacter* Enemy = Cast<AEnemyBotCharacter>(InPawn);
	if (Enemy)
	{
		ObservedEnemyPawn = Enemy;
		Enemy->GetOnDeathDelegate().AddUniqueDynamic(
			this, &ThisClass::HandleControlledPawnDeath);
	}
	if (AIPerceptionComponent)
	{
		AIPerceptionComponent->RequestStimuliListenerUpdate();
	}

	// 决策资产由 Controller 持有，和 LyraPlayerBotController 的职责边界一致；
	// Character 只提供 Avatar、表现和战斗能力，不再保存第二份 Behavior Tree 配置。
	UBehaviorTree* BehaviorTree = BehaviorTreeAsset;
	if (!BehaviorTree || !BehaviorTree->BlackboardAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Enemy %s has no valid BehaviorTree/Blackboard configuration."),
			*GetNameSafe(InPawn));
		return;
	}

	UBlackboardComponent* InitializedBlackboard = nullptr;
	if (!UseBlackboard(BehaviorTree->BlackboardAsset, InitializedBlackboard) || !InitializedBlackboard)
	{
		UE_LOG(LogTemp, Error, TEXT("Enemy %s failed to initialize blackboard %s."),
			*GetNameSafe(InPawn), *GetNameSafe(BehaviorTree->BlackboardAsset));
		return;
	}

	InitializedBlackboard->SetValueAsVector(HomeLocationBlackboardKey, InPawn->GetActorLocation());
	if (!RunBehaviorTree(BehaviorTree))
	{
		UE_LOG(LogTemp, Error, TEXT("Enemy %s failed to start behavior tree %s."),
			*GetNameSafe(InPawn), *GetNameSafe(BehaviorTree));
	}
	RefreshPerceivedHostileTarget();
}

void AEnemyBotController::HandleControlledPawnDeath(AActor* DeadActor)
{
	if (DeadActor && DeadActor != GetPawn())
	{
		return;
	}

	// DeathStarted 早于 Pawn UnPossess；不能把清理责任推迟到 UnPossess，
	// 否则死亡 Pawn 仍会保留 TargetActor，BT Service 还会继续尝试刷新目标。
	ResetDecisionState(true);
}

void AEnemyBotController::ResetDecisionState(const bool bStopBrain)
{
	ReleaseMeleeEngagementSlot();
	RememberedHostileTarget.Reset();
	RememberedHostileTargetLastSeenTime = 0.0;
	TargetApproachOffset = FVector::ZeroVector;

	if (UBlackboardComponent* ActiveBlackboard = GetBlackboardComponent())
	{
		ActiveBlackboard->ClearValue(TargetActorBlackboardKey);
		ActiveBlackboard->ClearValue(TargetApproachLocationBlackboardKey);
		ActiveBlackboard->ClearValue(PatrolLocationBlackboardKey);
	}

	ClearFocus(EAIFocusPriority::Gameplay);
	StopMovement();
	if (bStopBrain && BrainComponent && BrainComponent->IsRunning())
	{
		BrainComponent->StopLogic(TEXT("Controlled pawn death or possession reset"));
	}
}

void AEnemyBotController::ReleaseMeleeEngagementSlot()
{
	if (UWorld* World = GetWorld())
	{
		if (UShootMeleeEngagementSubsystem* EngagementSubsystem =
			World->GetSubsystem<UShootMeleeEngagementSubsystem>())
		{
			if (APawn* CurrentPawn = GetPawn())
			{
				EngagementSubsystem->ReleaseAllForAttacker(CurrentPawn);
			}
			if (AEnemyBotCharacter* PreviousEnemy = ObservedEnemyPawn.Get())
			{
				if (PreviousEnemy != GetPawn())
				{
					EngagementSubsystem->ReleaseAllForAttacker(PreviousEnemy);
				}
			}
		}
	}
	RememberedMeleeSlotIndex = INDEX_NONE;
}

void AEnemyBotController::TryAcquireMeleeEngagementSlot(
	AActor* Target, AEnemyBotCharacter* Enemy, const FVector& PreferredDirection)
{
	RememberedMeleeSlotIndex = INDEX_NONE;
	if (!Target || !Enemy)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (UShootMeleeEngagementSubsystem* EngagementSubsystem =
			World->GetSubsystem<UShootMeleeEngagementSubsystem>())
		{
			RememberedMeleeSlotIndex = EngagementSubsystem->AcquireSlot(
				Target, Enemy, FMath::Max(MeleeEngagementSlotCount, 1), PreferredDirection);
		}
	}
}

void AEnemyBotController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	RefreshPerceivedHostileTarget();
}

void AEnemyBotController::RefreshPerceivedHostileTarget()
{
	UBlackboardComponent* ActiveBlackboard = GetBlackboardComponent();
	AEnemyBotCharacter* Enemy = Cast<AEnemyBotCharacter>(GetPawn());
	if (!ActiveBlackboard || !Enemy || !AIPerceptionComponent
		|| Enemy->GetDeathState() != EShootDeathState::NotDead)
	{
		return;
	}

	const double CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const float AggroRangeSquared = FMath::Square(Enemy->GetAggroRange());
	const auto IsLiveHostile = [Enemy](AActor* Candidate)
	{
		if (!IsValid(Candidate)
			|| Candidate == Enemy
			|| UShootAbilitySystemLibrary::GetTeamAttitudeForActors(Enemy, Candidate) != ETeamAttitude::Hostile
			|| !UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Candidate))
		{
			return false;
		}

		// 候选类型不绑定玩家或某个 Character 子类；只有实现 Combat Interface 的对象才参与死亡过滤。
		return !Candidate->Implements<UCombatInterface>()
			|| !ICombatInterface::Execute_IsDead(Candidate);
	};

	TArray<AActor*> PerceivedActors;
	// 传入 nullptr 读取所有当前感知来源，和 Lyra 的 Sight + Damage 配置保持一致；
	// 是否可攻击仍由 Team Attitude、存活状态和目标 ASC 共同决定。
	AIPerceptionComponent->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

	AActor* NearestHostile = nullptr;
	float NearestDistanceSquared = AggroRangeSquared;
	AActor* RememberedTarget = RememberedHostileTarget.Get();
	float RememberedDistanceSquared = TNumericLimits<float>::Max();
	bool bRememberedTargetCurrentlyPerceived = false;
	for (AActor* Candidate : PerceivedActors)
	{
		if (!IsLiveHostile(Candidate))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(Enemy->GetActorLocation(), Candidate->GetActorLocation());
		if (Candidate == RememberedTarget && DistanceSquared < AggroRangeSquared)
		{
			RememberedDistanceSquared = DistanceSquared;
			bRememberedTargetCurrentlyPerceived = true;
			RememberedHostileTargetLastSeenTime = CurrentTime;
		}

		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			NearestHostile = Candidate;
		}
	}

	if (NearestHostile && bRememberedTargetCurrentlyPerceived && RememberedTarget
		&& NearestHostile != RememberedTarget)
	{
		// 最近目标不是唯一决策条件：当前目标仍然有效时，只有明显更近的 Hostile 才能抢占，
		// 避免 Perception 刷新顺序导致丧尸在一群同阵营目标之间抖动。
		const float NearestDistance = FMath::Sqrt(NearestDistanceSquared);
		const float RememberedDistance = FMath::Sqrt(RememberedDistanceSquared);
		if (NearestDistance + TargetSwitchDistanceAdvantage >= RememberedDistance)
		{
			NearestHostile = RememberedTarget;
		}
	}

	if (!NearestHostile && IsLiveHostile(RememberedTarget)
		&& CurrentTime - RememberedHostileTargetLastSeenTime <= FMath::Max(0.0f, TargetMemoryDuration))
	{
		// 短暂丢失只进入目标记忆阶段，保持 Engage 分支和 Focus，给 AI 时间绕过遮挡或人群。
		NearestHostile = RememberedTarget;
	}

	if (NearestHostile)
	{
		const bool bNewTarget = RememberedHostileTarget.Get() != NearestHostile;
		if (bNewTarget)
		{
			ReleaseMeleeEngagementSlot();
			RememberedHostileTarget = NearestHostile;
			RememberedHostileTargetLastSeenTime = CurrentTime;
		}

		// 每只 Zombie 申请独立的环形接敌槽位；没有容量时把站位放到外围，
		// 目标仍保留在 Blackboard 中，但攻击 Service 会拒绝没有槽位的攻击尝试。
		FVector ApproachDirection = Enemy->GetActorLocation() - NearestHostile->GetActorLocation();
		ApproachDirection.Z = 0.0f;
		if (!ApproachDirection.Normalize())
		{
			ApproachDirection = Enemy->GetActorForwardVector();
			ApproachDirection.Z = 0.0f;
			if (!ApproachDirection.Normalize())
			{
				ApproachDirection = FVector::ForwardVector;
			}
		}

		// MoveTo 的最终 Pawn 中心距还会受到自身胶囊半径、导航接受半径和其他攻击者碰撞的影响。
		// 旧的 100cm 固定环在 4 个槽位时仍可能把攻击者推到命中包络外；这里把接近环收进
		// 胶囊刚好分离的区域（34cm + 34cm），让 AttackRange 继续表示可见手部接触包络，
		// 而不是为了弥补站位误差再扩大成“远程近战”。高范围 archetype 仍按自身配置比例放大。
		const float PreferredApproachDistance = FMath::Max(80.0f, Enemy->GetAttackRange() * 0.55f);
		if (RememberedMeleeSlotIndex == INDEX_NONE)
		{
			TryAcquireMeleeEngagementSlot(NearestHostile, Enemy, ApproachDirection);
		}

		if (UWorld* World = GetWorld())
		{
			if (UShootMeleeEngagementSubsystem* EngagementSubsystem =
				World->GetSubsystem<UShootMeleeEngagementSubsystem>())
			{
				if (RememberedMeleeSlotIndex != INDEX_NONE)
				{
					TargetApproachOffset = EngagementSubsystem->GetSlotOffset(
						NearestHostile,
						RememberedMeleeSlotIndex,
						FMath::Max(MeleeEngagementSlotCount, 1),
						PreferredApproachDistance);
				}
				else
				{
					// 槽位满时保持在攻击范围之外，避免“站着看目标但每帧都请求失败”。
					const float StagingDistance = FMath::Max(
						PreferredApproachDistance + 100.0f,
						Enemy->GetAttackRange() + 75.0f);
					TargetApproachOffset = ApproachDirection * StagingDistance;
				}
			}
			else
			{
				TargetApproachOffset = ApproachDirection * PreferredApproachDistance;
			}
		}
		else
		{
			TargetApproachOffset = ApproachDirection * PreferredApproachDistance;
		}

		if (ActiveBlackboard->GetValueAsObject(TargetActorBlackboardKey) != NearestHostile)
		{
			ActiveBlackboard->SetValueAsObject(TargetActorBlackboardKey, NearestHostile);
		}
		ActiveBlackboard->SetValueAsVector(
			TargetApproachLocationBlackboardKey,
			NearestHostile->GetActorLocation() + TargetApproachOffset);
		SetFocus(NearestHostile);
	}
	else
	{
		ReleaseMeleeEngagementSlot();
		RememberedHostileTarget.Reset();
		RememberedHostileTargetLastSeenTime = 0.0;
		TargetApproachOffset = FVector::ZeroVector;
		ActiveBlackboard->ClearValue(TargetActorBlackboardKey);
		ActiveBlackboard->ClearValue(TargetApproachLocationBlackboardKey);
		ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void AEnemyBotController::OnUnPossess()
{
	if (AEnemyBotCharacter* PreviousEnemy = ObservedEnemyPawn.Get())
	{
		PreviousEnemy->GetOnDeathDelegate().RemoveDynamic(
			this, &ThisClass::HandleControlledPawnDeath);
	}
	ObservedEnemyPawn.Reset();
	ResetDecisionState(true);
	Super::OnUnPossess();
}
