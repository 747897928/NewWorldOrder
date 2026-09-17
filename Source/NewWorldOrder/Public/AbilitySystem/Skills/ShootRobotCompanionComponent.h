// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"

#include "ShootRobotCompanionComponent.generated.h"

class APawn;
class APlayerState;
class AShootCharacterBase;
class AShootRobotCompanionCharacter;
class UShootSkillDefinition;
class UShootSkillLoadoutComponent;
class UGameplayEffect;

/**
 * PlayerState 上的机器人伙伴权威生命周期组件。
 * 技能槽只描述 Match 构筑；本组件保证每名玩家最多一个机器人，并区分战斗击毁、Owner 死亡、
 * 技能替换和 Experience 卸载。只有战斗击毁会进入重新召唤冷却。
 */
UCLASS(ClassGroup=(Abilities), meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootRobotCompanionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShootRobotCompanionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/**
	 * Robot GA 的唯一入口。没有机器人时生成；已有机器人时按 Definition.ModeTags 循环模式。
	 * 具体蓝图类和冷却 GE 来自 GA 蓝图壳，组件不硬编码 /Game 资源路径。
	 */
	bool ActivateOrCycleRobot(APawn* RequestingPawn, const UShootSkillDefinition* Definition,
		TSubclassOf<AShootRobotCompanionCharacter> RobotClass,
		TSubclassOf<UGameplayEffect> DestroyedCooldownEffect,
		const FVector& SpawnOffset, float SummonDropHeight, float CompanionLifetime,
		bool bOwnerDeathStartsCooldown);

	/** Robot Character 的死亡链在服务器调用；避免 OnDestroyed 猜测销毁原因。 */
	void NotifyRobotCombatDestroyed(AShootRobotCompanionCharacter* DestroyedRobot);

	UFUNCTION(BlueprintPure, Category="Robot Companion")
	AShootRobotCompanionCharacter* GetActiveRobot() const { return ActiveRobot; }

private:
	enum class ECleanupReason : uint8
	{
		OwnerDeath,
		LifetimeExpired,
		SkillRemoved,
		ExperienceUnload
	};

	UFUNCTION()
	void HandleOwnerPawnChanged(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn);

	UFUNCTION()
	void HandleOwnerPawnDeath(AActor* DeadActor);

	UFUNCTION()
	void HandleRobotDestroyed(AActor* DestroyedActor);
	void HandleRobotLifetimeExpired();

	void HandleSkillSlotsChanged(UShootSkillLoadoutComponent* ChangedComponent, int32 ChangedSlotIndex);
	void BindOwnerPawn(APawn* NewPawn, APawn* OldPawn);
	void DestroyActiveRobot(ECleanupReason Reason);
	void ApplyDestroyedCooldown();
	bool ResolveActiveSlot(int32& OutLevel, FGameplayTag& OutMode) const;

	UPROPERTY(ReplicatedUsing=OnRep_ActiveRobot, Transient)
	TObjectPtr<AShootRobotCompanionCharacter> ActiveRobot;

	UFUNCTION()
	void OnRep_ActiveRobot();

	UPROPERTY(Transient)
	TObjectPtr<const UShootSkillDefinition> ActiveDefinition;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> ActiveDestroyedCooldownEffect;

	UPROPERTY(Transient)
	TObjectPtr<AShootCharacterBase> BoundOwnerPawn;

	bool bActiveOwnerDeathStartsCooldown = false;
	bool bCombatDeathAlreadyHandled = false;
	FTimerHandle LifetimeTimerHandle;
};
