// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AI/Robot/ShootRobotBehaviorTreeNodes.h"

#include "AI/Robot/ShootRobotCompanionCharacter.h"
#include "AI/Robot/ShootRobotCompanionController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UShootBTService_RobotRefreshContext::UShootBTService_RobotRefreshContext()
{
	NodeName = TEXT("Refresh Robot Team Context");
	Interval = 0.2f;
	RandomDeviation = 0.03f;
	bCallTickOnSearchStart = true;
}

void UShootBTService_RobotRefreshContext::TickNode(UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	if (AShootRobotCompanionController* Controller = Cast<AShootRobotCompanionController>(OwnerComp.GetAIOwner()))
	{
		Controller->RefreshRobotContext();
	}
}

UShootBTTask_RobotFire::UShootBTTask_RobotFire()
{
	NodeName = TEXT("Robot Attack Through GAS");
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
}

EBTNodeResult::Type UShootBTTask_RobotFire::ExecuteTask(UBehaviorTreeComponent& OwnerComp,
	uint8* NodeMemory)
{
	AShootRobotCompanionCharacter* Robot = OwnerComp.GetAIOwner()
		? Cast<AShootRobotCompanionCharacter>(OwnerComp.GetAIOwner()->GetPawn())
		: nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AActor* Target = Blackboard
		? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey.SelectedKeyName))
		: nullptr;
	return Robot && Target && Robot->TryActivateAttack(Target)
		? EBTNodeResult::Succeeded
		: EBTNodeResult::Failed;
}
