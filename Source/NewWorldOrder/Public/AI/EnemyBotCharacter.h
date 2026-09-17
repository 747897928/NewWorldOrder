// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Character/ShootCharacterBase.h"
#include "AbilitySystem/ShootAbilitySet.h"
#include "EnemyBotCharacter.generated.h"

class AEnemyBotController;
class UAIPerceptionStimuliSourceComponent;
class UAnimInstance;
class UAnimMontage;
class UAnimSequence;
class UGameplayEffect;
class USkeletalMesh;

DECLARE_MULTICAST_DELEGATE(FShootZombieMeleeNotify);

/**
 * Zombie archetype 的最小表现状态。
 * 正式 AnimBP 已负责 Locomotion、死亡和 Montage Slot 表现；这里保留复制状态作为迁移期的攻击阶段契约，
 * 供 AnimBP 和服务器近战 GA 共享。寻敌、移动和攻击决策始终由 Controller + Blackboard + BehaviorTree 承担。
 */
UENUM(BlueprintType)
enum class EShootEnemyTestAnimationState : uint8
{
	Idle,
	Moving,
	Attacking
};

UCLASS()
class NEWWORLDORDER_API AEnemyBotCharacter : public AShootCharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyBotCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	/** Combat Interface */
	virtual int32 GetPlayerLevel_Implementation() override;
	virtual void Die(const FVector& DeathImpulse) override;
	/** 返回关卡放置或运行时生成时的初始 Transform，供 GameMode 在敌人死亡后按原出生点重建。 */
	const FTransform& GetInitialSpawnTransform() const { return InitialSpawnTransform; }
	//virtual void SetCombatTarget_Implementation(AActor* InCombatTarget) override;
	//virtual AActor* GetCombatTarget_Implementation() const override;
	/** end Combat Interface */
	
	void SetLevel(int32 InLevel) { Level = InLevel; }

	/** 更新迁移期表现状态；正式 AnimBP 只读取 Attacking，移动状态由 GroundSpeed 派生。 */
	void SetArchetypeAnimationState(EShootEnemyTestAnimationState NewState);

	/** 返回服务器复制的攻击阶段，供正式 Zombie AnimBP 与近战 GA 清理路径共享。 */
	bool IsArchetypeAttackAnimationActive() const
	{
		return TestAnimationState == EShootEnemyTestAnimationState::Attacking;
	}

	/** Behavior Tree 攻击 Service 调用；只按 Ability Tag 请求服务器 GA，不在 Character 直接结算伤害。 */
	bool TryActivateMeleeAttack(AActor* TargetActor);
	AActor* GetPendingMeleeTarget() const { return PendingMeleeTarget.Get(); }
	void ClearPendingMeleeTarget() { PendingMeleeTarget.Reset(); }

	/** Montage 命中帧的服务器入口；具体 Zombie GA 通过该委托结算一次权威伤害。 */
	void NotifyMeleeAnimationEvent();
	FShootZombieMeleeNotify& OnMeleeAnimationNotify() { return MeleeAnimationNotifyDelegate; }
	void PlayReplicatedMeleeMontage();

	/** 统一的服务器目标复核，初始激活和命中 Notify 都必须通过。 */
	bool IsMeleeAttackTargetValid(const AActor& Target) const;
	bool IsAttackTargetInRange(const AActor& Target) const;

	/** 不同 Zombie 蓝图子类在 Class Defaults 中组成独立 archetype，Behavior Tree 通过这些只读接口消费配置。 */
	float GetAggroRange() const { return TestAggroRange; }
	float GetAttackRange() const { return TestAttackRange; }
	float GetAttackInterval() const { return TestAttackInterval; }
	float GetAttackDamage() const { return TestAttackDamage; }
	float GetRoamRadius() const { return TestRoamRadius; }
	UAnimMontage* GetAttackMontage() const { return TestAttackMontage; }
	float GetAttackFacingDotThreshold() const { return TestAttackFacingDotThreshold; }
	TSubclassOf<UGameplayEffect> GetAttackEffectClass() const { return TestAttackEffectClass; }
	TSubclassOf<UGameplayEffect> GetAttackStatusEffectClass() const { return TestAttackStatusEffectClass; }
	float GetAttackStatusMagnitude() const { return TestAttackStatusMagnitude; }

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Class Defaults")
	int32 Level = 1;
	
	UPROPERTY()
	TObjectPtr<AEnemyBotController> EnemyBotController;

	/** 显式注册为 Sight 刺激源，不能依赖引擎“自动注册全部 Pawn”的项目级隐式开关。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AI|Perception")
	TObjectPtr<UAIPerceptionStimuliSourceComponent> SightStimuliSource;

	/**
	 * 每个 Zombie 蓝图子类明确选择自己的网格。它不是随机换皮：同一蓝图类的网格、数值、
	 * 攻击 GE 和动作共同构成一种敌人类型；运行时不会在基类里随机替换外观。
	 *
	 * 这里使用普通 Class Default，而不是直接覆盖 CharacterMesh0 的继承组件模板。后者经蓝图
	 * 编译可能回退到父类默认网格，导致编辑器里看似写入成功、重启后却丢失。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Presentation", meta=(DisplayName="Skeletal Mesh"))
	TObjectPtr<USkeletalMesh> TestSkeletalMesh;

	/**
	 * 正式 Zombie AnimBP。继承的 CharacterMesh0 组件模板不作为持久配置入口，
	 * 由基类在应用 archetype 网格后设置该 Class Default，避免蓝图编译或重启时组件覆盖回退。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Presentation", meta=(DisplayName="Animation Blueprint"))
	TSubclassOf<UAnimInstance> TestAnimClass;

	/** Zombie 近战 GA 播放的 Montage；具体接触帧必须配置 UShootAnimNotify_ZombieMeleeHit。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Animation", meta=(DisplayName="Attack Montage"))
	TObjectPtr<UAnimMontage> TestAttackMontage;

	/** 五套 Zombie Skeleton 已配置 Compatible Skeletons，因此各敌人蓝图子类可以自由搭配动作。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Presentation", meta=(DisplayName="Idle Animation"))
	TObjectPtr<UAnimSequence> TestIdleAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Presentation", meta=(DisplayName="Move Animation"))
	TObjectPtr<UAnimSequence> TestMoveAnimation;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Presentation", meta=(DisplayName="Attack Animation"))
	TObjectPtr<UAnimSequence> TestAttackAnimation;

	/**
	 * 以下参数属于敌人 archetype，不属于 Spawner 全局规则。历史 C++ 标识保留 Test 前缀，避免已有
	 * 蓝图 Class Defaults 在重命名时静默丢值；编辑器 DisplayName 与公开只读接口使用正式命名。
	 * 如果多个 Experience 需要共享同一类型目录，再整体迁入 DataAsset，不增加第二套行为逻辑。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat", meta=(DisplayName="Aggro Range", ClampMin="100.0", Units="cm"))
	float TestAggroRange = 2400.0f;

	/**
	 * 近战命中判定使用 Pawn Actor Location 的二维中心距，不是 Aggro Range，也不是导航 MoveTo 的接受半径。
	 * 该值必须按胶囊半径、手臂可见伸展距离和共用攻击动画一起调节；配置过大时会出现手还没有碰到目标就
	 * 在 Notify 帧造成伤害的“远程近战”观感。实际激活和 Notify 命中仍会在服务器再次复核该距离。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat", meta=(DisplayName="Attack Range", ClampMin="0.0", Units="cm"))
	float TestAttackRange = 280.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat", meta=(DisplayName="Attack Interval", ClampMin="0.1", Units="s"))
	float TestAttackInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat", meta=(DisplayName="Attack Damage", ClampMin="0.0"))
	float TestAttackDamage = 5.0f;

	/** 命中时允许的最小朝向点积；最终命中仍由服务器在 Notify 帧复核。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat", meta=(DisplayName="Attack Facing Dot Threshold", ClampMin="-1.0", ClampMax="1.0"))
	float TestAttackFacingDotThreshold = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat", meta=(DisplayName="Roam Radius", ClampMin="100.0", Units="cm"))
	float TestRoamRadius = 800.0f;

	/** 默认使用统一 IncomingDamage GE；蓝图子类可替换，但禁止直接修改目标 Health。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat", meta=(DisplayName="Attack Effect Class"))
	TSubclassOf<UGameplayEffect> TestAttackEffectClass;

	/**
	 * 可选的命中附加 GE。首期 Bleeder 使用周期 IncomingDamage 表达短暂流血；
	 * Magnitude 通过 SetByCaller.Damage 写入，其他状态类型以后应升级为明确的数据结构。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat", meta=(DisplayName="Attack Status Effect Class"))
	TSubclassOf<UGameplayEffect> TestAttackStatusEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Combat", meta=(DisplayName="Attack Status Magnitude", ClampMin="0.0"))
	float TestAttackStatusMagnitude = 0.0f;

	/**
	 * Lyra 风格的敌人战斗能力套件。Zombie 的 GA/GE 组成由 DataAsset 统一管理，
	 * archetype 只选择套件，不再在每个蓝图里复制 StartupAbilities 数组。
	 * ResourceInventory/InventoryManager 只负责物品，不参与 Zombie 的战斗能力授予。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Abilities")
	TObjectPtr<const UShootAbilitySet> ZombieAbilitySet;

	/**
	 * AI 的 ASC 归属于 Character，PossessedBy 可能因为控制器重接管而重复进入。
	 * 默认属性只在当前 Pawn 生命周期内初始化一次；能力套件另有句柄，供 UnPossessed 时完整回收。
	 * 蓝图子类仍需在 Character Class Defaults 中配置基类的三组 Default Attributes。
	 */
	bool bAbilityDefaultsInitialized = false;

	/** AbilitySet 是可回收的运行时授予物；旧 StartupAbilities 仅作为未迁移类的兼容路径。 */
	bool bZombieAbilitySetGranted = false;
	bool bCharacterAbilitiesGranted = false;
	FShootAbilitySet_GrantedHandles ZombieAbilitySetGrantedHandles;

	/** 解除本 Pawn 通过 ZombieAbilitySet 获得的 GA/GE，允许重生或重新 Possess 时重新授予。 */
	void RemoveZombieAbilitySet();

	/** BeginPlay 首次记录；死亡时不使用当前位置，避免敌人越打越偏离出生区域。 */
	FTransform InitialSpawnTransform;

	UPROPERTY(ReplicatedUsing=OnRep_TestAnimationState)
	EShootEnemyTestAnimationState TestAnimationState = EShootEnemyTestAnimationState::Idle;

	UFUNCTION()
	void OnRep_TestAnimationState();

	UFUNCTION()
	void HandleMovementUpdated(float DeltaSeconds, FVector OldLocation, FVector OldVelocity);

	void ApplyTestSkeletalMesh();
	void ApplyTestAnimationState();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayMeleeMontage();

	TWeakObjectPtr<AActor> PendingMeleeTarget;
	FShootZombieMeleeNotify MeleeAnimationNotifyDelegate;
	
public:
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
