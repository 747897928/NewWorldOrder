// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShootEnemyTestSpawner.generated.h"

class AEnemyBotCharacter;

/** 测试副本的敌人类型与生成权重；类型本身的网格、动画、属性 GE 和战斗参数都放在蓝图子类。 */
USTRUCT(BlueprintType)
struct FShootEnemyTestSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AEnemyBotCharacter> EnemyClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.0"))
	float Weight = 1.0f;
};

/**
 * 副本敌人种群生成器。
 *
 * 本 Actor 只负责服务器权威生成、维持数量和暂停/恢复整组 AI。寻敌、巡逻、追击、攻击与
 * 冷却全部由 AEnemyBotController + Blackboard + BehaviorTree 编排，战斗结算由 Character 的
 * GAS/GE 入口完成；禁止再把行为逻辑塞回本类，避免形成第二套 AI 框架。
 */
UCLASS(BlueprintType)
class NEWWORLDORDER_API AShootEnemyTestSpawner : public AActor
{
	GENERATED_BODY()

public:
	AShootEnemyTestSpawner();

	/**
	 * 测试期间暂停/恢复敌人追击和攻击；人口维持仍继续，方便验收相机、HUD 和武器表现。
	 * 蓝图按钮应在服务器权威路径调用，本函数不绑定具体键盘或手柄按键。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Test Enemy Behavior")
	void SetEnemyBehaviorEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="Test Enemy Behavior")
	bool IsEnemyBehaviorEnabled() const { return bEnemyBehaviorEnabled; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/**
	 * 旧地图兼容回退。新配置使用 EnemyArchetypes；数组为空时才生成该类，避免旧关卡实例立即失效。
	 * 完成 TestMap 资产迁移并确认无实例覆盖后可删除。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test Enemy Population")
	TSubclassOf<AEnemyBotCharacter> EnemyClass;

	/**
	 * 可生成的敌人蓝图子类。Spawner 只按权重选类，不随机替换基类 Mesh；
	 * 每个子类独立配置移动速度、生命属性 GE、攻击节奏、伤害、附加状态和 Compatible Skeleton 动画。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test Enemy Population")
	TArray<FShootEnemyTestSpawnEntry> EnemyArchetypes;

	/** 地图中的存活敌人固定目标数；服务器会周期性补足缺口。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test Enemy Population", meta=(ClampMin="0", UIMin="0"))
	int32 TargetPopulation = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test Enemy Population", meta=(ClampMin="100.0", Units="cm"))
	float SpawnRadius = 3500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test Enemy Population", meta=(ClampMin="0.0", Units="cm"))
	float MinimumSpawnSeparation = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test Enemy Population", meta=(ClampMin="0.1", Units="s"))
	float PopulationRefreshInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test Enemy Population", meta=(ClampMin="1", UIMin="1"))
	int32 MaxSpawnAttemptsPerEnemy = 30;

	/** 默认开启；关闭后敌人留在场内，但暂停各自正式 Behavior Tree 并立即停止移动。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Test Enemy Behavior")
	bool bEnemyBehaviorEnabled = true;

private:
	void RefreshPopulation();
	void FillPopulation(const TArray<AEnemyBotCharacter*>& LivingEnemies);
	bool FindNonOverlappingSpawnTransform(TSubclassOf<AEnemyBotCharacter> SpawnClass, FTransform& OutTransform) const;
	TSubclassOf<AEnemyBotCharacter> SelectEnemyClass() const;
	bool IsManagedEnemy(const AEnemyBotCharacter& Enemy) const;
	void ApplyBehaviorEnabledState(AEnemyBotCharacter& Enemy) const;

	FTimerHandle RefreshTimerHandle;
	double NextSpawnWarningTime = 0.0;
};
