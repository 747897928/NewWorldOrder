// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Character/ShootCharacterBase.h"
#include "AbilitySystem/ShootAbilitySet.h"

#include "ShootRobotCompanionCharacter.generated.h"

class APlayerState;
class UBehaviorTree;
class UGameplayEffect;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_MULTICAST_DELEGATE(FShootRobotFireNotify);
DECLARE_MULTICAST_DELEGATE(FShootRobotMeleeNotify);

/**
 * 正式机器人伙伴 Character。
 * 自有 ASC/AttributeSet，动画始终由 AnimBP + Montage 驱动；寻敌和移动决策只由 AIController、
 * Blackboard 与 BehaviorTree 负责，本类不在 Tick 中扫描目标。
 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API AShootRobotCompanionCharacter : public AShootCharacterBase
{
	GENERATED_BODY()

public:
	AShootRobotCompanionCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual int32 GetPlayerLevel_Implementation() override { return CompanionLevel; }
	virtual void Die(const FVector& DeathImpulse) override;

	/** SpawnActorDeferred 后、FinishSpawning 前由 PlayerState 组件注入 Match 所有权。 */
	void InitializeCompanion(APlayerState* InOwnerPlayerState, int32 InLevel, FGameplayTag InMode);
	void MarkSummonDropPending(bool bPending) { bSummonDropPending = bPending; }
	void SetCompanionOwnerPawn(APawn* InOwnerPawn);
	void SetCompanionLevel(int32 InLevel);
	void SetCommandMode(FGameplayTag InMode, const APawn* CommandingPawn);
	void SelfDestructFromOwnerLoss();
	void SelfDestructFromLifetimeExpiry();

	/** BT Task 的统一攻击入口；近距离优先近战，其余距离使用射击。 */
	bool TryActivateAttack(AActor* TargetActor);

	/** BT Task 写入目标后尝试激活机器人自己的 Fire GA。 */
	bool TryActivateFire(AActor* TargetActor);
	AActor* GetPendingFireTarget() const { return PendingFireTarget.Get(); }
	void ClearPendingFireTarget() { PendingFireTarget.Reset(); }
	bool TryActivateMelee(AActor* TargetActor);
	AActor* GetPendingMeleeTarget() const { return PendingMeleeTarget.Get(); }
	void ClearPendingMeleeTarget() { PendingMeleeTarget.Reset(); }

	/** AnimNotify 的服务器入口；GA 监听此事件进行权威命中。 */
	void NotifyFireAnimationEvent();
	FShootRobotFireNotify& OnFireAnimationNotify() { return FireAnimationNotifyDelegate; }
	void NotifyMeleeAnimationEvent();
	FShootRobotMeleeNotify& OnMeleeAnimationNotify() { return MeleeAnimationNotifyDelegate; }

	UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }
	APlayerState* GetCompanionOwnerPlayerState() const { return CompanionOwnerPlayerState; }
	APawn* GetCompanionOwnerPawn() const { return CompanionOwnerPawn; }
	FGameplayTag GetCommandMode() const { return CommandModeTag; }
	FVector GetCommandLocation() const { return CommandLocation; }
	float GetAggroRangeForCurrentMode() const;
	float GetAttackRange() const { return AttackRange; }
	float GetDesiredCombatRange() const;
	float GetFollowDistance() const { return FollowDistance; }
	float GetBalancedLeashDistance() const { return BalancedLeashDistance; }
	float GetFireInterval() const;
	float GetShotDamage() const { return BaseShotDamage + (CompanionLevel - 1) * DamagePerLevel; }
	float GetMeleeRange() const { return MeleeRange; }
	float GetMeleeInterval(bool bChomp) const;
	float GetMeleeDamage(bool bChomp) const;
	float GetMeleeRadius(bool bChomp) const { return bChomp ? ChompRadius : ClawRadius; }
	TSubclassOf<UGameplayEffect> GetDamageEffectClass() const { return DamageEffectClass; }
	USceneComponent* GetNextMuzzle(bool& bOutLeftMuzzle);
	void PlayReplicatedFireMontage(bool bLeftMuzzle);
	bool ChooseChompAttack();
	void PlayReplicatedMeleeMontage(bool bChomp);
	void ApplyMeleeImpact(AActor* PreferredTarget, bool bChomp);
	bool IsFacingAttackTarget(const AActor* TargetActor) const;

	UFUNCTION(BlueprintImplementableEvent, Category="Robot|Presentation")
	void BP_OnCommandModeChanged(FGameplayTag NewModeTag);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Robot|Presentation")
	TObjectPtr<UStaticMeshComponent> RightWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Robot|Presentation")
	TObjectPtr<UStaticMeshComponent> LeftWeapon;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Robot|Presentation")
	TObjectPtr<UStaticMeshComponent> PowerPod;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Robot|Combat")
	TObjectPtr<USceneComponent> RightMuzzle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Robot|Combat")
	TObjectPtr<USceneComponent> LeftMuzzle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	/** Robot 自己的 Fire GA 等能力，由 Character ASC 持有，不能授予玩家 ASC。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Abilities")
	TObjectPtr<const UShootAbilitySet> RobotAbilitySet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Attributes")
	TSubclassOf<UGameplayEffect> RobotVitalsEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Animation")
	TObjectPtr<UAnimMontage> FireRightMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Animation")
	TObjectPtr<UAnimMontage> FireLeftMontage;

	/** 右手持枪时，近战默认使用空闲的左爪。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Animation")
	TObjectPtr<UAnimMontage> ClawLeftMontage;

	/** 近战强袭模式每第三次攻击使用咬击，形成比重复挥爪更有生命力的节奏。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Animation")
	TObjectPtr<UAnimMontage> ChompMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Attachment")
	FName RightWeaponSocket = TEXT("Buster_RSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Attachment")
	FName LeftWeaponSocket = TEXT("Buster_LSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Attachment")
	FName PowerPodSocket = TEXT("PowerPod");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="100.0", Units="cm"))
	float BalancedAggroRange = 2400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="100.0", Units="cm"))
	float RangedAggroRange = 3400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="100.0", Units="cm"))
	float MeleeAggroRange = 3600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="100.0", Units="cm"))
	float AttackRange = 2200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="100.0", Units="cm"))
	float BalancedDesiredCombatRange = 600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="100.0", Units="cm"))
	float RangedDesiredCombatRange = 950.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="100.0", Units="cm"))
	float MeleeDesiredCombatRange = 165.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.0"))
	float BaseShotDamage = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.0"))
	float DamagePerLevel = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.05", Units="s"))
	float BaseFireInterval = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.0", Units="s"))
	float FireIntervalPerLevel = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.1"))
	float RangedFireIntervalMultiplier = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.1"))
	float BalancedFireIntervalMultiplier = 0.9f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="50.0", Units="cm"))
	float MeleeRange = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.0"))
	float BaseMeleeDamage = 32.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.0"))
	float MeleeDamagePerLevel = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="1.0"))
	float ChompDamageMultiplier = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.05", Units="s"))
	float ClawInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="0.05", Units="s"))
	float ChompInterval = 1.35f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="1.0", Units="cm"))
	float ClawRadius = 190.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="1.0", Units="cm"))
	float ChompRadius = 130.f;

	/** 伤害前必须面向目标；防止骨骼还朝前时命中背后的敌人。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat", meta=(ClampMin="-1.0", ClampMax="1.0"))
	float AttackFacingDotThreshold = 0.94f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Movement", meta=(ClampMin="100.0", Units="cm"))
	float FollowDistance = 300.f;

	/** 均衡护卫模式超过该距离会放弃追击，优先回到主人身边。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Movement", meta=(ClampMin="100.0", Units="cm"))
	float BalancedLeashDistance = 1400.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Attributes", meta=(ClampMin="1.0"))
	float BaseMaxHealth = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Attributes", meta=(ClampMin="0.0"))
	float MaxHealthPerLevel = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Death", meta=(ClampMin="0.0", Units="s"))
	float DeathLifeSpan = 2.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Presentation", meta=(Categories="GameplayCue"))
	FGameplayTag SelfDestructCueTag;

	/** 从高处召唤落地时播放的一次性冲击表现，由机器人蓝图配置具体 GameplayCue。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Presentation", meta=(Categories="GameplayCue"))
	FGameplayTag SummonImpactCueTag;

	/** 从高处落地时对周围 Hostile 造成一次服务器权威伤害；数值与半径由机器人蓝图调优。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat|AreaDamage", meta=(ClampMin="0.0"))
	float SummonImpactDamage = 45.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat|AreaDamage", meta=(ClampMin="0.0", Units="cm"))
	float SummonImpactRadius = 350.f;

	/** Owner 死亡触发自爆时的伤害；技能替换和 Experience 卸载不会走这条战斗结算。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat|AreaDamage", meta=(ClampMin="0.0"))
	float SelfDestructDamage = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Robot|Combat|AreaDamage", meta=(ClampMin="0.0", Units="cm"))
	float SelfDestructRadius = 450.f;

private:
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayFireMontage(bool bLeftMuzzle);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayMeleeMontage(bool bChomp);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayDeathMontage(bool bSelfDestruct);

	UFUNCTION()
	void OnRep_CommandMode(FGameplayTag OldModeTag);

	UFUNCTION()
	void OnRep_CompanionLevel();

	void InitializeRobotAbilitySystem();
	void ApplyRobotVitals();
	void ApplyAttachmentSockets();
	void ApplyAreaDamageToHostiles(const FVector& Origin, float Radius, float Damage);
	void ExecuteSelfDestruct(const TCHAR* StopReason);

	UPROPERTY(Replicated, Transient)
	TObjectPtr<APlayerState> CompanionOwnerPlayerState;

	UPROPERTY(Replicated, Transient)
	TObjectPtr<APawn> CompanionOwnerPawn;

	UPROPERTY(ReplicatedUsing=OnRep_CompanionLevel)
	int32 CompanionLevel = 1;

	UPROPERTY(ReplicatedUsing=OnRep_CommandMode)
	FGameplayTag CommandModeTag;

	UPROPERTY(Replicated)
	FVector_NetQuantize CommandLocation = FVector::ZeroVector;

	TWeakObjectPtr<AActor> PendingFireTarget;
	TWeakObjectPtr<AActor> PendingMeleeTarget;
	FShootRobotFireNotify FireAnimationNotifyDelegate;
	FShootRobotMeleeNotify MeleeAnimationNotifyDelegate;
	FShootAbilitySet_GrantedHandles RobotAbilityHandles;
	bool bRobotAbilitySystemInitialized = false;
	bool bUseLeftMuzzleNext = false;
	bool bSummonDropPending = false;
	int32 MeleeAttackSequence = 0;
};
