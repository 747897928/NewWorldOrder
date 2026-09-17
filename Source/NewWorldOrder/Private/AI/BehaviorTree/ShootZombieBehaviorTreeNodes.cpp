// Copyright ZhaoYiJie

#include "AI/BehaviorTree/ShootZombieBehaviorTreeNodes.h"

#include "AI/EnemyBotCharacter.h"
#include "AI/EnemyBotController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "NavigationSystem.h"

UShootBTService_RefreshHostileTarget::UShootBTService_RefreshHostileTarget()
{
	NodeName = TEXT("Refresh Perceived Hostile Target");
	Interval = 0.35f;
	RandomDeviation = 0.1f;
	bCallTickOnSearchStart = true;
}

void UShootBTService_RefreshHostileTarget::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	if (AEnemyBotController* Controller = Cast<AEnemyBotController>(OwnerComp.GetAIOwner()))
	{
		Controller->RefreshPerceivedHostileTarget();
	}
}

UShootBTTask_FindPatrolLocation::UShootBTTask_FindPatrolLocation()
{
	NodeName = TEXT("Find Patrol Location");
	HomeLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, HomeLocationKey));
	PatrolLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, PatrolLocationKey));
}

UShootBTService_AttemptMeleeAttack::UShootBTService_AttemptMeleeAttack()
{
	NodeName = TEXT("Attempt Melee Attack");
	Interval = 0.15f;
	RandomDeviation = 0.03f;
	bCallTickOnSearchStart = true;
	TargetActorKey.AddObjectFilter(
		this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
}

void UShootBTService_AttemptMeleeAttack::TickNode(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AEnemyBotController* Controller = Cast<AEnemyBotController>(OwnerComp.GetAIOwner());
	AEnemyBotCharacter* Enemy = Controller
		? Cast<AEnemyBotCharacter>(Controller->GetPawn())
		: nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* Target = Blackboard
		? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName))
		: nullptr;
	if (!Controller || !Enemy || !Target
		|| Enemy->GetDeathState() != EShootDeathState::NotDead
		|| Enemy->IsArchetypeAttackAnimationActive()
		|| !Controller->CanAttemptMeleeAttack(Target)
		|| !Enemy->IsAttackTargetInRange(*Target))
	{
		return;
	}

	// 仅在目标进入近战距离时尝试一次；失败不能让 Engage Sequence 失败，下一次服务 Tick 会重试。
	// 这样可以覆盖冷却、动画仍在收尾、短暂碰撞阻挡或 GAS 尚未完成初始化等可恢复状态。
	if (!Enemy->TryActivateMeleeAttack(Target))
	{
		UE_LOG(LogTemp, VeryVerbose,
			TEXT("Zombie %s could not activate melee against %s; service will retry."),
			*GetNameSafe(Enemy), *GetNameSafe(Target));
	}
}

EBTNodeResult::Type UShootBTTask_FindPatrolLocation::ExecuteTask(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AEnemyBotCharacter* Enemy = OwnerComp.GetAIOwner()
		? Cast<AEnemyBotCharacter>(OwnerComp.GetAIOwner()->GetPawn())
		: nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	UNavigationSystemV1* NavigationSystem = Enemy
		? UNavigationSystemV1::GetCurrent(Enemy->GetWorld())
		: nullptr;
	if (!Enemy || !Blackboard || !NavigationSystem)
	{
		return EBTNodeResult::Failed;
	}

	const FVector HomeLocation = Blackboard->GetValueAsVector(HomeLocationKey.SelectedKeyName);
	FNavLocation PatrolLocation;
	if (!NavigationSystem->GetRandomReachablePointInRadius(
		HomeLocation, Enemy->GetRoamRadius(), PatrolLocation))
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsVector(PatrolLocationKey.SelectedKeyName, PatrolLocation.Location);
	return EBTNodeResult::Succeeded;
}
