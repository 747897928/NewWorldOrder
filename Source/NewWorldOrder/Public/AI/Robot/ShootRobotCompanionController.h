// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "ModularAIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Teams/LyraTeamAgentInterface.h"

#include "ShootRobotCompanionController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/**
 * 机器人专用 AIController。感知只收集候选，所有敌我结论统一走 ILyraTeamAgentInterface；
 * Controller 写黑板，BehaviorTree 编排移动与开火，Character 不维护第二条决策链。
 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API AShootRobotCompanionController : public AModularAIController,
	public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	AShootRobotCompanionController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return CachedTeamId; }
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override { return &OnTeamChangedDelegate; }
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	/** BehaviorTree Service 与感知事件共用的唯一上下文刷新入口。 */
	void RefreshRobotContext();

	static const FName OwnerActorKey;
	static const FName TargetActorKey;
	static const FName DesiredMoveLocationKey;
	static const FName CommandLocationKey;
	static const FName CommandModeKey;
	static const FName HasTargetKey;
	static const FName TargetInAttackRangeKey;
	static const FName HasLineOfSightKey;
	static const FName IsDeadKey;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Perception")
	TObjectPtr<UAIPerceptionComponent> RobotPerceptionComponent;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

private:
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void HandleOwnerTeamChanged(UObject* ObjectChangingTeam, int32 OldTeamId, int32 NewTeamId);

	void BindOwnerTeam(bool bBind);
	bool IsValidHostileCandidate(const AActor* Candidate) const;

	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	FGenericTeamId CachedTeamId = FGenericTeamId::NoTeam;
};
