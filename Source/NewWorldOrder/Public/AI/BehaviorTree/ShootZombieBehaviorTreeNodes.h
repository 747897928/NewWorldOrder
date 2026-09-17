// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "ShootZombieBehaviorTreeNodes.generated.h"

/**
 * 从 AEnemyBotController 的 AI Perception 结果中刷新最近 Hostile。
 * 目标选择只写 Blackboard，不直接移动或攻击；Neutral/Friendly 默认不会产生攻击欲望。
 */
UCLASS()
class NEWWORLDORDER_API UShootBTService_RefreshHostileTarget : public UBTService
{
	GENERATED_BODY()

public:
	UShootBTService_RefreshHostileTarget();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

/** 在出生点附近从 NavMesh 选择巡逻位置并写入 Blackboard。 */
UCLASS()
class NEWWORLDORDER_API UShootBTTask_FindPatrolLocation : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UShootBTTask_FindPatrolLocation();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector HomeLocationKey;

	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector PatrolLocationKey;
};

/**
 * Lyra ShooterCore 风格的持续攻击服务：行为树保持在 Engage 分支，服务按固定节奏尝试近战 GA，
 * 不把一次能力激活失败升级成整个行为树分支失败。攻击规则仍由 Character + GAS 决定。
 */
UCLASS()
class NEWWORLDORDER_API UShootBTService_AttemptMeleeAttack : public UBTService
{
	GENERATED_BODY()

public:
	UShootBTService_AttemptMeleeAttack();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category="Blackboard")
	FBlackboardKeySelector TargetActorKey;
};
