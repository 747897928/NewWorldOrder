// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "ShootProjectileBase.generated.h"

class UAbilitySystemComponent;
class UShootRangedWeaponInstance;
class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UParticleSystem;
class UParticleSystemComponent;
class UGameplayEffect;
class UPrimitiveComponent;
class UStaticMesh;

/**
 * 爆炸武器通用配置
 * - 由 ItemDefinition 的 ProjectileConfig Fragment 数据驱动，描述初速度、重力、引信、爆炸半径等行为
 */
USTRUCT(BlueprintType)
struct FProjectileWeaponConfig
{
	GENERATED_BODY()

public:
	/** 初始速度（cm/s），服务器在生成投射物时套用 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Movement")
	float InitialSpeed = 2000.f;

	/** 重力比例，=1 表示受重力影响，=0 表示直线飞行 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Movement")
	float GravityScale = 1.0f;

	/** 是否允许弹跳（榴弹需要弹跳） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Movement")
	bool bShouldBounce = false;

	/** 弹跳速度保留比例；仅在 bShouldBounce=true 时使用。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Movement", meta=(EditCondition="bShouldBounce", ClampMin="0.0", ClampMax="1.0"))
	float Bounciness = 0.4f;

	/** 弹跳时的切向摩擦；仅在 bShouldBounce=true 时使用。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Movement", meta=(EditCondition="bShouldBounce", ClampMin="0.0", ClampMax="1.0"))
	float BounceFriction = 0.2f;

	/** 低于该速度后停止弹跳模拟（cm/s）。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Movement", meta=(EditCondition="bShouldBounce", ClampMin="0.0"))
	float BounceStopSpeed = 50.0f;

	/** Actor 是否让 +X 轴持续跟随当前速度；关闭时保持生成旋转，适合自行滚转的视觉方案。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Movement")
	bool bRotationFollowsVelocity = true;

	/**
	 * 对真实准星阻挡点使用 InitialSpeed 与 GravityScale 求固定速度弹道解。
	 * 未命中阻挡物、无重力或目标超出物理解时仍沿准星初始方向发射，不伪造落点。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Movement")
	bool bUseBallisticAim = false;

	/** 弹道瞄准存在两条物理解时选择高弧；关闭为更适合榴弹发射器的低弧。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Movement", meta=(EditCondition="bUseBallisticAim"))
	bool bFavorHighArc = false;

	/**
	 * 服务器接受所属客户端准星目标点时，允许它与服务器控制旋转的最大夹角。
	 * 仅校验瞄准输入；投射物生成、碰撞、爆炸和伤害始终由服务器权威执行。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Networking", meta=(ClampMin="0.0", ClampMax="180.0"))
	float MaxServerAimErrorDegrees = 15.0f;

	/** 引信时间（秒），<=0 表示靠碰撞立即爆炸 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Damage")
	float FuseTime = 0.0f;

	/** 爆炸伤害内半径，处于该半径内的目标吃满伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Damage")
	float DamageInnerRadius = 200.f;

	/** 爆炸伤害外半径，超出该半径不再造成伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Damage")
	float DamageOuterRadius = 400.f;

	/** 爆炸伤害衰减（0-无衰减），为 1 表示线性衰减到最小伤害 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Damage")
	float DamageFalloff = 1.0f;

	/** 是否对爆炸命中应用物理材质倍率（默认关，火箭/榴弹不吃弱点） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|Damage")
	bool bApplyMaterialMultipliers = false;

	/** 爆炸时触发的 GameplayCue，用于通知所有客户端播放特效 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|FX")
	FGameplayTag ExplosionCueTag;

	/** 可选：飞行中的拖尾特效（Niagara） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|FX")
	UNiagaraSystem* TrailSystem = nullptr;

	/**
	 * 可选：尚未完成 Cascade 到 Niagara 等价验证时使用的原始 Cascade 尾迹。
	 * 这是表现资产类型的明确配置，不参与伤害计算，也不作为 Niagara 的数值回退。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|FX")
	TObjectPtr<UParticleSystem> TrailParticleSystem = nullptr;

	/** 投射物视觉网格（可选；不设置则投射物不可见）。数据驱动，C++ 不引用 /Game/ 资产路径。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile|FX")
	TObjectPtr<UStaticMesh> ProjectileMesh = nullptr;

};

/**
 * 投射物基类：
 * - 由服务器生成并复制到客户端
 * - 只在服务器计算爆炸与伤害，再通过 GameplayCue 广播表现
 * - 需要由 GA 在 Spawn 后调用 InitializeProjectile 传入武器、ASC、伤害规格
 */
UCLASS()
class AShootProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AShootProjectileBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 服务器初始化投射物的入口，GA 生成后调用 */
	void InitializeProjectile(const FProjectileWeaponConfig& InConfig,
		UShootRangedWeaponInstance* InWeaponInstance,
		UAbilitySystemComponent* InSourceASC,
		TSubclassOf<UGameplayEffect> InDamageEffectClass,
		float InDamageEffectLevel,
		const FGameplayEffectContextHandle& InEffectContext,
		const FVector& InFireDirection);

	/**
	 * 将投射物准备为装备在火箭筒上的弹头。此时 Actor 保留可见网格，
	 * 但关闭碰撞、移动和飞行拖尾；之后可直接再次调用 InitializeProjectile 发射。
	 */
	void PrepareAsHeldProjectile(const FProjectileWeaponConfig& InConfig);

	bool IsHeldProjectile() const { return bIsHeldProjectile; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 服务器调用，统一处理爆炸伤害与特效 */
	virtual void Explode(const FHitResult& ImpactHit);

	/** 子类可覆盖，用于在 Explode 之前进行自定义行为（如榴弹二次反弹音效） */
	virtual void HandlePreExplode(const FHitResult& ImpactHit) {}

	/** 子类可覆盖，配置 MovementComponent（火箭/榴弹差异化） */
	virtual void ConfigureMovement();

	/** 播放 GameplayCue（本地 + 远端），在服务器 Explode 时调用 */
	void BroadcastExplosionCue(const FHitResult& ImpactHit);

	/** 服务器注册的碰撞回调 */
	UFUNCTION()
	virtual void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** 子类决定何时因碰撞触发爆炸（默认任何阻挡都会爆炸） */
	virtual bool ShouldExplodeOnHit(AActor* OtherActor, const FHitResult& Hit) const;

	/** 引信超时回调（FuseTime > 0 时使用） */
	void HandleFuseExpired();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	USphereComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	UProjectileMovementComponent* MovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	UNiagaraComponent* TrailComponent;

	/** 原始 Cascade 尾迹组件；与 Niagara 尾迹互斥，避免同一投射物双播。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	UParticleSystemComponent* TrailParticleComponent;

	/** 视觉网格组件：数据驱动（ProjectileMesh），默认空实现不显示任何网格。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	TObjectPtr<UStaticMeshComponent> VisualMeshComponent;

	/** 服务器缓存的武器实例（只在服务端有效），用于查询伤害/材质/曲线等数据 */
	UPROPERTY()
	TWeakObjectPtr<UShootRangedWeaponInstance> CachedWeaponInstance;

	/** 服务器缓存的 Source ASC（执行 GameplayCue 与伤害用） */
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> CachedSourceASC;

	/** 服务器缓存的正式 Damage GE 与上下文；最终伤害由 GE Execution 计算。 */
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;
	float DamageGameplayEffectLevel = 1.0f;
	FGameplayEffectContextHandle DamageEffectContextHandle;

	/** 配置数据复制到客户端，主要用于表现同步（重力/Trail/Cue） */
	UPROPERTY(ReplicatedUsing=OnRep_Config)
	FProjectileWeaponConfig ReplicatedConfig;

	/** 是否已经爆炸，避免重复执行 */
	bool bHasExploded = false;

protected:
	/** 解析此次命中的物理材质（供弱点倍率使用） */
	const UPhysicalMaterial* ResolvePhysicalMaterial(const FOverlapResult& OverlapInfo,
		const FHitResult& ImpactHit) const;

private:
	void ApplyRadialDamage(const FVector& Epicenter, const FHitResult& ImpactHit);
	void SpawnOrUpdateTrail();
	void ClearPendingFuseTimer();
	void SendReticleMessageToOwner(const TArray<FVector>& HitLocations); // 服务器爆炸后通知本地 HUD

	UFUNCTION()
	void OnRep_Config();

	UFUNCTION()
	void OnRep_HeldProjectileState();

	void ApplyHeldProjectileState();

	/** 服务器生成的同一个 Actor 在换弹期间是否仍挂在武器上。 */
	UPROPERTY(ReplicatedUsing=OnRep_HeldProjectileState)
	bool bIsHeldProjectile = false;

	FTimerHandle FuseTimerHandle;
};
