// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "ModularAIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "EnemyBotController.generated.h"

namespace ETeamAttitude { enum Type : int; }
struct FGenericTeamId;

class APlayerState;
class AEnemyBotCharacter;
class UBehaviorTree;
class UAIPerceptionComponent;
class UAISenseConfig_Damage;
class UAISenseConfig_Sight;
class UShootMeleeEngagementSubsystem;
class UObject;
struct FFrame;
/**
 * 
 */
// EnemyBotController.h

UCLASS()
class NEWWORLDORDER_API AEnemyBotController : public AModularAIController, public ILyraTeamAgentInterface
{
	GENERATED_BODY()

public:
	AEnemyBotController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ILyraTeamAgentInterface
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return CachedTeamID; }
	virtual FOnLyraTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override { return &OnTeamChangedDelegate; }
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;

	/** Behavior Tree Service 与感知事件共用的目标刷新入口，只会把 Hostile 写入黑板。 */
	void RefreshPerceivedHostileTarget();

	/** 只有持有近战接敌槽位的 Zombie 才能发起近战 GA；没有槽位时继续在外围追击/等待。 */
	bool CanAttemptMeleeAttack(AActor* Target) const;

	/** Lyra 风格由 Controller 持有决策资产；Character 只提供 Avatar 和战斗能力。 */
	UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }

	static const FName TargetActorBlackboardKey;
	static const FName TargetApproachLocationBlackboardKey;
	static const FName HomeLocationBlackboardKey;
	static const FName PatrolLocationBlackboardKey;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** Lyra ShooterCore 同时配置 Sight/Damage；Damage 感知可供后续仇恨系统接入，当前仍由 Team 过滤。 */
	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	/**
	 * 当前 Controller 使用的决策资产。行为树属于 AI 身份而不是某个 Pawn 的表现配置，
	 * 因此由 Controller 蓝图 Class Defaults 选择；BehaviorTree 内部仍持有 Blackboard 资产。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="AI|Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

	/**
	 * 目标暂时丢失后继续追踪的时间。感知系统会因为遮挡、拥挤和边缘视角短暂丢失 Stimulus；
	 * 立即清空黑板会让行为树在追击和巡逻之间来回切换，因此这里保留短暂的目标记忆。
	 */
	UPROPERTY(EditDefaultsOnly, Category="AI|Targeting", meta=(ClampMin="0.0", Units="s"))
	float TargetMemoryDuration = 2.0f;

	/**
	 * 当前目标仍在视野内时，新目标必须至少近这么多厘米才会抢占目标，避免多个目标距离接近时
	 * 每次感知刷新都切换目标。该值由控制器蓝图调优，不按“是否玩家”区分目标。
	 */
	UPROPERTY(EditDefaultsOnly, Category="AI|Targeting", meta=(ClampMin="0.0", Units="cm"))
	float TargetSwitchDistanceAdvantage = 250.0f;

	/**
	 * 同一目标最多同时占用的近战站位数；超出容量的 Zombie 进入外围等待，不抢攻击窗口。
	 * 当前角色胶囊半径为 34cm，接近环半径约 100cm 时 8 个槽位会让相邻胶囊几乎相切，
	 * 导航接受半径和碰撞解算会把所有攻击者向外推，最后出现“目标已锁定但够不到”的罚站。
	 * 4 个槽位为首期尸群提供可读的攻击面，其余敌人保留目标记忆并在外围排队。
	 */
	UPROPERTY(EditDefaultsOnly, Category="AI|Targeting", meta=(ClampMin="1", UIMin="1"))
	int32 MeleeEngagementSlotCount = 4;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	/**
	 * Character 的 DeathStarted 生命周期回调。Lyra 在 Pawn 生命周期切换时停止 Brain；
	 * 本项目也必须在死亡而非仅在 UnPossess 时清理 Blackboard、Focus 和目标记忆。
	 */
	UFUNCTION()
	void HandleControlledPawnDeath(AActor* DeadActor);

	void ResetDecisionState(bool bStopBrain);

private:
	UPROPERTY()
	FOnLyraTeamIndexChangedDelegate OnTeamChangedDelegate;

	/** AI 没有 PlayerState，因此由控制器蓝图配置并缓存队伍；默认 2 与当前丧尸阵营配置一致。 */
	UPROPERTY(EditDefaultsOnly, Category="Team")
	uint8 DefaultTeamId = 2;

	UPROPERTY() // 关键：在控制器本地缓存 TeamID（AI 没有 PS）
	FGenericTeamId CachedTeamID = FGenericTeamId::NoTeam;

	/** 最近一次确认仍在攻击范围内的 Hostile 目标；它只来自 Team Attitude 过滤结果。 */
	TWeakObjectPtr<AActor> RememberedHostileTarget;
	double RememberedHostileTargetLastSeenTime = 0.0;

	/** 当前目标第一次被锁定时生成的环形接近偏移；目标移动时只平移该点，不让群体重新挤到中心。 */
	FVector TargetApproachOffset = FVector::ZeroVector;

	/** 当前目标上的槽位索引；INDEX_NONE 表示仍在外围等待容量。 */
	int32 RememberedMeleeSlotIndex = INDEX_NONE;

	void ReleaseMeleeEngagementSlot();
	void TryAcquireMeleeEngagementSlot(AActor* Target, AEnemyBotCharacter* Enemy,
		const FVector& PreferredDirection);

	/** 用于在重新 Possess/UnPossess 时解除旧 Pawn 的死亡委托。 */
	TWeakObjectPtr<AEnemyBotCharacter> ObservedEnemyPawn;
};
