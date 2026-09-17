// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AI/Robot/ShootRobotCompanionController.h"

#include "AI/Robot/ShootRobotCompanionCharacter.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "AbilitySystemInterface.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Interface/CombatInterface.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootRobotCompanionController)

const FName AShootRobotCompanionController::OwnerActorKey(TEXT("OwnerActor"));
const FName AShootRobotCompanionController::TargetActorKey(TEXT("TargetActor"));
const FName AShootRobotCompanionController::DesiredMoveLocationKey(TEXT("DesiredMoveLocation"));
const FName AShootRobotCompanionController::CommandLocationKey(TEXT("CommandLocation"));
const FName AShootRobotCompanionController::CommandModeKey(TEXT("CommandMode"));
const FName AShootRobotCompanionController::HasTargetKey(TEXT("HasTarget"));
const FName AShootRobotCompanionController::TargetInAttackRangeKey(TEXT("TargetInAttackRange"));
const FName AShootRobotCompanionController::HasLineOfSightKey(TEXT("HasLineOfSight"));
const FName AShootRobotCompanionController::IsDeadKey(TEXT("IsDead"));

AShootRobotCompanionController::AShootRobotCompanionController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsPlayerState = false;
	RobotPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("RobotPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	// 伙伴必须能察觉主人背后的近身威胁。模式自己的活动边界仍在 RefreshRobotContext 中裁剪，
	// 感知半径只负责建立候选，不等于所有模式都会跨 5200cm 追击。
	SightConfig->SightRadius = 5200.f;
	SightConfig->LoseSightRadius = 5600.f;
	SightConfig->PeripheralVisionAngleDegrees = 175.f;
	SightConfig->SetMaxAge(2.5f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	RobotPerceptionComponent->ConfigureSense(*SightConfig);
	RobotPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
	SetPerceptionComponent(*RobotPerceptionComponent);
}

void AShootRobotCompanionController::BeginPlay()
{
	Super::BeginPlay();
	RobotPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(
		this, &ThisClass::HandleTargetPerceptionUpdated);
}

void AShootRobotCompanionController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	AShootRobotCompanionCharacter* Robot = Cast<AShootRobotCompanionCharacter>(InPawn);
	UBehaviorTree* BehaviorTree = Robot ? Robot->GetBehaviorTree() : nullptr;
	if (!Robot || !BehaviorTree || !BehaviorTree->BlackboardAsset)
	{
		UE_LOG(LogTemp, Error, TEXT("Robot %s has no valid BehaviorTree/Blackboard configuration."),
			*GetNameSafe(InPawn));
		return;
	}

	BindOwnerTeam(true);
	if (const ILyraTeamAgentInterface* OwnerTeam = Cast<ILyraTeamAgentInterface>(
		Robot->GetCompanionOwnerPlayerState()))
	{
		SetGenericTeamId(OwnerTeam->GetGenericTeamId());
	}

	UBlackboardComponent* InitializedBlackboard = nullptr;
	if (!UseBlackboard(BehaviorTree->BlackboardAsset, InitializedBlackboard) || !InitializedBlackboard)
	{
		UE_LOG(LogTemp, Error, TEXT("Robot %s failed to initialize blackboard %s."),
			*GetNameSafe(InPawn), *GetNameSafe(BehaviorTree->BlackboardAsset));
		return;
	}
	if (!RunBehaviorTree(BehaviorTree))
	{
		UE_LOG(LogTemp, Error, TEXT("Robot %s failed to start behavior tree %s."),
			*GetNameSafe(InPawn), *GetNameSafe(BehaviorTree));
		return;
	}
	RefreshRobotContext();
}

void AShootRobotCompanionController::OnUnPossess()
{
	BindOwnerTeam(false);
	ClearFocus(EAIFocusPriority::Gameplay);
	Super::OnUnPossess();
}

void AShootRobotCompanionController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	const FGenericTeamId OldTeamId = CachedTeamId;
	CachedTeamId = NewTeamID;
	ILyraTeamAgentInterface::ConditionalBroadcastTeamChanged(this, OldTeamId, CachedTeamId);
	// 与 Lyra PlayerBotController 一致：AI Perception 会缓存 Listener 的 TeamId；队伍变化后必须刷新，
	// 否则机器人虽然已经继承玩家 TeamId，感知过滤仍可能继续按旧的 NoTeam 工作。
	if (RobotPerceptionComponent)
	{
		RobotPerceptionComponent->RequestStimuliListenerUpdate();
	}
}

ETeamAttitude::Type AShootRobotCompanionController::GetTeamAttitudeTowards(const AActor& Other) const
{
	return UShootAbilitySystemLibrary::GetTeamAttitudeForActors(this, &Other);
}

void AShootRobotCompanionController::RefreshRobotContext()
{
	AShootRobotCompanionCharacter* Robot = Cast<AShootRobotCompanionCharacter>(GetPawn());
	UBlackboardComponent* BlackboardComp = GetBlackboardComponent();
	if (!Robot || !BlackboardComp || !RobotPerceptionComponent)
	{
		return;
	}

	const bool bDead = Robot->GetDeathState() != EShootDeathState::NotDead;
	BlackboardComp->SetValueAsBool(IsDeadKey, bDead);
	if (bDead)
	{
		BlackboardComp->ClearValue(TargetActorKey);
		BlackboardComp->SetValueAsBool(HasTargetKey, false);
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	APawn* OwnerPawn = Robot->GetCompanionOwnerPawn();
	const FGameplayTag Mode = Robot->GetCommandMode();
	const bool bBalancedMode = Mode == FShootGameplayTags::Get().Ability_Mode_Robot_Balanced;
	const FVector CommandLocation = Robot->GetCommandLocation();
	BlackboardComp->SetValueAsObject(OwnerActorKey, OwnerPawn);
	BlackboardComp->SetValueAsVector(CommandLocationKey, CommandLocation);
	BlackboardComp->SetValueAsName(CommandModeKey, Mode.GetTagName());

	TArray<AActor*> PerceivedActors;
	RobotPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	AActor* BestTarget = nullptr;
	float BestTargetScore = TNumericLimits<float>::Max();
	float BestRobotDistanceSquared = TNumericLimits<float>::Max();
	for (AActor* Candidate : PerceivedActors)
	{
		if (!IsValidHostileCandidate(Candidate))
		{
			continue;
		}

		const FVector CandidateLocation = Candidate->GetActorLocation();
		const float RobotDistanceSquared = FVector::DistSquared(Robot->GetActorLocation(), CandidateLocation);
		// 均衡护卫从主人身边选威胁，远程与近战模式从机器人自身选目标。
		// 这不是“是否玩家/丧尸”的硬编码，候选敌我仍由 Team Attitude 唯一裁决。
		const FVector PriorityOrigin = bBalancedMode && OwnerPawn
			? OwnerPawn->GetActorLocation()
			: Robot->GetActorLocation();
		const float ModeBoundary = Robot->GetAggroRangeForCurrentMode();
		const float TargetScore = FVector::DistSquared(PriorityOrigin, CandidateLocation);
		if (TargetScore <= FMath::Square(ModeBoundary) && TargetScore < BestTargetScore)
		{
			BestTargetScore = TargetScore;
			BestRobotDistanceSquared = RobotDistanceSquared;
			BestTarget = Candidate;
		}
	}

	const bool bHasTarget = IsValid(BestTarget);
	// BestTarget 来自 Sight 的 CurrentlyPerceivedActors，本身就代表当前视线成立；不要再用 LineOfSightTo
	// 建立第二套可能与 Perception 不一致的判定。真正射击仍由服务器 GA 的 Visibility Trace 最终裁决。
	const bool bHasLineOfSight = bHasTarget;
	const bool bInAttackRange = bHasTarget && bHasLineOfSight
		&& BestRobotDistanceSquared <= FMath::Square(Robot->GetAttackRange());
	BlackboardComp->SetValueAsObject(TargetActorKey, BestTarget);
	BlackboardComp->SetValueAsBool(HasTargetKey, bHasTarget);
	BlackboardComp->SetValueAsBool(HasLineOfSightKey, bHasLineOfSight);
	BlackboardComp->SetValueAsBool(TargetInAttackRangeKey, bInAttackRange);

	FVector DesiredMoveLocation = Robot->GetActorLocation();
	const bool bBalancedLeashExceeded = bBalancedMode && OwnerPawn
		&& FVector::DistSquared(Robot->GetActorLocation(), OwnerPawn->GetActorLocation())
			> FMath::Square(Robot->GetBalancedLeashDistance());
	if (bBalancedLeashExceeded && OwnerPawn)
	{
		DesiredMoveLocation = OwnerPawn->GetActorLocation()
			- OwnerPawn->GetActorForwardVector() * Robot->GetFollowDistance();
	}
	else if (bHasTarget && BestRobotDistanceSquared > FMath::Square(Robot->GetDesiredCombatRange()))
	{
		const FVector AwayFromTarget = (Robot->GetActorLocation() - BestTarget->GetActorLocation()).GetSafeNormal2D();
		DesiredMoveLocation = BestTarget->GetActorLocation() + AwayFromTarget * Robot->GetDesiredCombatRange();
	}
	else if (OwnerPawn)
	{
		DesiredMoveLocation = OwnerPawn->GetActorLocation()
			- OwnerPawn->GetActorForwardVector() * Robot->GetFollowDistance();
	}
	BlackboardComp->SetValueAsVector(DesiredMoveLocationKey, DesiredMoveLocation);

	if (BestTarget)
	{
		// Gameplay focus 的优先级高于 PathFollowing；CharacterMovement 使用 ControllerDesiredRotation，
		// 因而机器人必须先转正再由 GA 的 facing gate 允许伤害。
		SetFocus(BestTarget);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void AShootRobotCompanionController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	RefreshRobotContext();
}

void AShootRobotCompanionController::HandleOwnerTeamChanged(UObject* ObjectChangingTeam,
	const int32 OldTeamId, const int32 NewTeamId)
{
	SetGenericTeamId(IntegerToGenericTeamId(NewTeamId));
	RefreshRobotContext();
}

void AShootRobotCompanionController::BindOwnerTeam(const bool bBind)
{
	AShootRobotCompanionCharacter* Robot = Cast<AShootRobotCompanionCharacter>(GetPawn());
	ILyraTeamAgentInterface* OwnerTeam = Robot
		? Cast<ILyraTeamAgentInterface>(Robot->GetCompanionOwnerPlayerState())
		: nullptr;
	if (!OwnerTeam || !OwnerTeam->GetOnTeamIndexChangedDelegate())
	{
		return;
	}
	if (bBind)
	{
		OwnerTeam->GetOnTeamIndexChangedDelegate()->AddUniqueDynamic(this, &ThisClass::HandleOwnerTeamChanged);
	}
	else
	{
		OwnerTeam->GetOnTeamIndexChangedDelegate()->RemoveDynamic(this, &ThisClass::HandleOwnerTeamChanged);
	}
}

bool AShootRobotCompanionController::IsValidHostileCandidate(const AActor* Candidate) const
{
	if (!IsValid(Candidate) || Candidate == GetPawn()
		|| !Candidate->Implements<UAbilitySystemInterface>()
		|| UShootAbilitySystemLibrary::GetTeamAttitudeForActors(this, Candidate) != ETeamAttitude::Hostile)
	{
		return false;
	}
	if (Candidate->Implements<UCombatInterface>() && ICombatInterface::Execute_IsDead(Candidate))
	{
		return false;
	}
	return true;
}
