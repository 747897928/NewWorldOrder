// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"

#include "ShootRobotBehaviorTreeNodes.generated.h"

/** 让 Controller 用感知、TeamId 和当前模式刷新完整黑板上下文。 */
UCLASS()
class NEWWORLDORDER_API UShootBTService_RobotRefreshContext : public UBTService
{
	GENERATED_BODY()

public:
	UShootBTService_RobotRefreshContext();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

/** 对 Blackboard TargetActor 选择近战或射击 GA；命中仍由各自 Montage Notify 结算。 */
UCLASS()
class NEWWORLDORDER_API UShootBTTask_RobotFire : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UShootBTTask_RobotFire();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetActorKey;
};
