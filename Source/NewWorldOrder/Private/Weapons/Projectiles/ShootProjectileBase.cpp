// Copyright ZhaoYiJie

#include "Weapons/Projectiles/ShootProjectileBase.h"

#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"
#include "AbilitySystem/ShootGameplayEffectContext.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "GameModes/ShootGameModeBase.h"
#include "Weapons/ShootRangedWeaponInstance.h"
#include "AbilitySystemGlobals.h"
#include "Physics/LyraCollisionChannels.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Engine/OverlapResult.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/ActorInstanceHandle.h"
#include "Player/ShootPlayerController.h"
#include "UI/Weapons/ShootHitMarkerTypes.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogShootProjectileMovement, Log, All);

AShootProjectileBase::AShootProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(5.f);
	// Lyra_TraceChannel_Weapon 是武器射线查询通道，不是实体投射物的对象通道。
	// 实体投射物沿用 Blaster 的 WorldDynamic：BlockAllDynamic 地板默认会阻挡该对象类型；
	// 若误用 Lyra 武器查询通道，即使本组件单方面阻挡 WorldDynamic，忽略该查询通道的地板仍会让投射物穿过。
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(Lyra_TraceChannel_Weapon, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(Lyra_TraceChannel_Weapon_Capsule, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(Lyra_TraceChannel_Weapon_Multi, ECR_Block);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	SetRootComponent(CollisionComponent);

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	MovementComponent->bRotationFollowsVelocity = true;
	// GA 已把 Actor +X 旋转到“枪口 -> 准星目标点”，并传入同方向的世界空间 Velocity。
	// UProjectileMovementComponent 默认会把初速度当作局部空间再乘一次 Actor 旋转，导致非 +X 朝向
	// 被旋转两次，表现成侧飞、反飞或似乎拐弯。这里明确关闭局部转换，与 Blaster 的
	// ToTarget.Rotation() + ActorForwardVector 约定等价：Actor +X、Velocity 与实际位移始终同向。
	MovementComponent->bInitialVelocityInLocalSpace = false;
	MovementComponent->bAutoActivate = false;
	MovementComponent->ProjectileGravityScale = 1.0f;
	MovementComponent->InitialSpeed = 2000.f;
	MovementComponent->MaxSpeed = 2000.f;

	TrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailComponent"));
	TrailComponent->SetupAttachment(RootComponent);
	TrailComponent->bAutoActivate = false;

	TrailParticleComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("TrailParticleComponent"));
	TrailParticleComponent->SetupAttachment(RootComponent);
	TrailParticleComponent->bAutoActivate = false;

	VisualMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMeshComponent"));
	VisualMeshComponent->SetupAttachment(RootComponent);
	VisualMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMeshComponent->SetVisibility(false);
}

void AShootProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShootProjectileBase, ReplicatedConfig);
	DOREPLIFETIME(AShootProjectileBase, bIsHeldProjectile);
}

void AShootProjectileBase::InitializeProjectile(const FProjectileWeaponConfig& InConfig,
	UShootRangedWeaponInstance* InWeaponInstance,
	UAbilitySystemComponent* InSourceASC,
	TSubclassOf<UGameplayEffect> InDamageEffectClass,
	float InDamageEffectLevel,
	const FGameplayEffectContextHandle& InEffectContext,
	const FVector& InFireDirection)
{
	// 仅服务器调用该方法
	check(HasAuthority());
	bIsHeldProjectile = false;
	bHasExploded = false;

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionComponent->SetGenerateOverlapEvents(true);
	}
	SetActorEnableCollision(true);

	ReplicatedConfig = InConfig;
	CachedWeaponInstance = InWeaponInstance;
	CachedSourceASC = InSourceASC;
	DamageGameplayEffectClass = InDamageEffectClass;
	DamageGameplayEffectLevel = InDamageEffectLevel;
	DamageEffectContextHandle = InEffectContext.Duplicate();

	if (CollisionComponent)
	{
		// 权威投射物从持枪者和武器枪口附近生成，必须显式忽略自己的 Pawn 与装备表现 Actor。
		// 否则第三人称动画让枪口短暂贴近胶囊或武器网格时，会在第一帧撞到自己并爆炸/反弹。
		CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);
		CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
		if (CachedWeaponInstance.IsValid())
		{
			for (AActor* SpawnedActor : CachedWeaponInstance.Get()->GetSpawnedActors())
			{
				CollisionComponent->IgnoreActorWhenMoving(SpawnedActor, true);
			}
		}

		// 友方是否阻挡实体投射物由当前 GameMode 的友伤倍率决定：无友伤时忽略同队 Pawn，
		// 启用友伤时保留 Pawn 阻挡，让 Rocket/Grenade 能命中并伤害队友。敌人和世界始终阻挡。
		const AShootGameModeBase* ShootGameMode = GetWorld()->GetAuthGameMode<AShootGameModeBase>();
		if (ShootGameMode && GetInstigator())
		{
			for (TActorIterator<APawn> PawnIt(GetWorld()); PawnIt; ++PawnIt)
			{
				APawn* CandidatePawn = *PawnIt;
				if (!CandidatePawn || CandidatePawn == GetInstigator())
				{
					continue;
				}

				const bool bIsFriendly = UShootAbilitySystemLibrary::GetTeamAttitudeForActors(
					GetInstigator(), CandidatePawn) == ETeamAttitude::Friendly;
				const bool bFriendlyFireDisabled = ShootGameMode->GetFriendlyFireScalarForActors(
					GetInstigator(), CandidatePawn) <= 0.0f;
				if (bIsFriendly && bFriendlyFireDisabled)
				{
					CollisionComponent->IgnoreActorWhenMoving(CandidatePawn, true);
				}
			}
		}
	}

	if (FShootGameplayEffectContext* ShootContext = FShootGameplayEffectContext::ExtractEffectContext(DamageEffectContextHandle))
	{
		ShootContext->SetRadialDamageParameters(
			ReplicatedConfig.DamageInnerRadius,
			ReplicatedConfig.DamageOuterRadius,
			ReplicatedConfig.DamageFalloff,
			ReplicatedConfig.bApplyMaterialMultipliers);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Projectile %s requires FShootGameplayEffectContext; damage is disabled."), *GetNameSafe(this));
		DamageEffectContextHandle = FGameplayEffectContextHandle();
	}

	// 先应用数据配置，再让 Rocket/Grenade 子类覆盖各自不可违背的运动规则。
	// 旧顺序是在子类覆写后再次写回 bShouldBounce，导致 Rocket 的“禁止弹跳”实际上可能被配置覆盖。
	if (MovementComponent)
	{
		MovementComponent->InitialSpeed = ReplicatedConfig.InitialSpeed;
		MovementComponent->MaxSpeed = ReplicatedConfig.InitialSpeed;
		MovementComponent->ProjectileGravityScale = ReplicatedConfig.GravityScale;
		MovementComponent->bShouldBounce = ReplicatedConfig.bShouldBounce;
		MovementComponent->bRotationFollowsVelocity = ReplicatedConfig.bRotationFollowsVelocity;
		MovementComponent->Bounciness = ReplicatedConfig.Bounciness;
		MovementComponent->Friction = ReplicatedConfig.BounceFriction;
		MovementComponent->BounceVelocityStopSimulatingThreshold = ReplicatedConfig.BounceStopSpeed;
	}

	// 让不同子类根据配置调整运动行为（榴弹需要弹跳、火箭不能弹跳等）。
	ConfigureMovement();

	// 在子类规则生效后设置初速度并启动移动。
	if (MovementComponent)
	{
		MovementComponent->Velocity = InFireDirection * ReplicatedConfig.InitialSpeed;
		MovementComponent->Activate(true);
		UE_LOG(LogShootProjectileMovement, Verbose,
			TEXT("Projectile initialized: Actor=%s Location=%s Velocity=%s Rotation=%s"),
			*GetNameSafe(this),
			*GetActorLocation().ToCompactString(),
			*MovementComponent->Velocity.ToCompactString(),
			*GetActorRotation().ToCompactString());
	}

	// 注册碰撞回调（服务器侧处理爆炸）
	CollisionComponent->OnComponentHit.AddDynamic(this, &ThisClass::OnProjectileHit);

	SpawnOrUpdateTrail();

	// 启动引信计时
	if (ReplicatedConfig.FuseTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &ThisClass::HandleFuseExpired, ReplicatedConfig.FuseTime);
	}
}

void AShootProjectileBase::PrepareAsHeldProjectile(const FProjectileWeaponConfig& InConfig)
{
	check(HasAuthority());

	ReplicatedConfig = InConfig;
	bIsHeldProjectile = true;
	bHasExploded = false;

	if (MovementComponent)
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Deactivate();
		MovementComponent->Velocity = FVector::ZeroVector;
	}

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComponent->SetGenerateOverlapEvents(false);
	}

	SetActorEnableCollision(false);
	SpawnOrUpdateTrail();
}

void AShootProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && MovementComponent)
	{
		// 投射物使用 Deferred Spawn：InitializeProjectile 在组件完成注册前写入速度并尝试激活，
		// 随后的 MovementComponent 注册会因 bAutoActivate=false 再次关闭 Tick。
		// 必须在 BeginPlay 中重新激活，确保服务器权威投射物真正更新位置并执行碰撞扫掠。
		if (!bIsHeldProjectile)
		{
			MovementComponent->Activate(true);
		}
	}

	if (bIsHeldProjectile)
	{
		ApplyHeldProjectileState();
	}

	if (!HasAuthority())
	{
		// 客户端需要在收到 Config 后配置拖尾
		SpawnOrUpdateTrail();
	}
}

void AShootProjectileBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		ClearPendingFuseTimer();
	}
	Super::EndPlay(EndPlayReason);
}

void AShootProjectileBase::ConfigureMovement()
{
	// 默认实现已经在 InitializeProjectile 中设置了必要参数，子类可覆盖进一步配置
}

void AShootProjectileBase::Explode(const FHitResult& ImpactHit)
{
	if (bHasExploded)
	{
		return;
	}
	bHasExploded = true;

	ClearPendingFuseTimer();
	HandlePreExplode(ImpactHit); // 子类可在这里做榴弹二段表现等纯表现
	ApplyRadialDamage(GetActorLocation(), ImpactHit);
	BroadcastExplosionCue(ImpactHit);

	// 爆炸表现统一交给 GameplayCueNotify：Cue 会在服务器广播后由各客户端本地播放
	// Niagara 与音频，避免这里的服务器 Spawn 与 GCN 同时存在而导致 Listen Server 双播。
	Destroy();
}

void AShootProjectileBase::BroadcastExplosionCue(const FHitResult& ImpactHit)
{
	if (!ReplicatedConfig.ExplosionCueTag.IsValid())
	{
		return;
	}

	if (CachedSourceASC.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = GetActorLocation();
		CueParams.Normal = ImpactHit.ImpactNormal;
		CueParams.SourceObject = CachedWeaponInstance.Get();
		CueParams.Instigator = GetInstigator();
		CueParams.EffectCauser = this;
		CueParams.PhysicalMaterial = ImpactHit.PhysMaterial;

		CachedSourceASC->ExecuteGameplayCue(ReplicatedConfig.ExplosionCueTag, CueParams);
	}
}

void AShootProjectileBase::ApplyRadialDamage(const FVector& Epicenter, const FHitResult& ImpactHit)
{
	if (!HasAuthority() || !CachedSourceASC.IsValid() || !DamageGameplayEffectClass || !DamageEffectContextHandle.IsValid())
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;

	FCollisionShape SphereShape = FCollisionShape::MakeSphere(ReplicatedConfig.DamageOuterRadius);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ProjectileDamageOverlap), false, this);

	if (!GetWorld()->OverlapMultiByChannel(Overlaps, Epicenter, FQuat::Identity, ECC_Pawn, SphereShape, QueryParams))
	{
		return;
	}

	TArray<FVector> SuccessfulHitLocations;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* OtherActor = Overlap.GetActor();
		if (!OtherActor)
		{
			// 匿名对象不能承载 ASC 或阵营信息。
			continue;
		}

		if (OtherActor == GetInstigator())
		{
			// 投射物始终忽略自己的胶囊以安全出膛，但爆炸是否能伤到自己仍服从 GameMode 友伤倍率。
			const AShootGameModeBase* ShootGameMode = GetWorld()->GetAuthGameMode<AShootGameModeBase>();
			if (!ShootGameMode
				|| ShootGameMode->GetFriendlyFireScalarForActors(GetInstigator(), OtherActor) <= 0.0f)
			{
				continue;
			}
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
		if (!TargetASC || !DamageGameplayEffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle LocalContext = DamageEffectContextHandle.Duplicate();
		FGameplayEffectSpecHandle LocalSpecHandle = CachedSourceASC->MakeOutgoingSpec(
			DamageGameplayEffectClass, DamageGameplayEffectLevel, LocalContext);

		if (!LocalSpecHandle.IsValid() || !LocalSpecHandle.Data.IsValid())
		{
			continue;
		}

		FHitResult TargetHit = ImpactHit;
		TargetHit.HitObjectHandle = FActorInstanceHandle(OtherActor);
		TargetHit.Component = Overlap.Component;
		TargetHit.TraceStart = Epicenter;
		TargetHit.TraceEnd = Epicenter;
		TargetHit.ImpactPoint = OtherActor->GetActorLocation();
		if (const UPhysicalMaterial* TargetPhysMat = ResolvePhysicalMaterial(Overlap, ImpactHit))
		{
			TargetHit.PhysMaterial = const_cast<UPhysicalMaterial*>(TargetPhysMat);
		}
		LocalSpecHandle.Data->GetContext().AddHitResult(TargetHit);

		SuccessfulHitLocations.Add(OtherActor->GetActorLocation());
		CachedSourceASC->ApplyGameplayEffectSpecToTarget(*LocalSpecHandle.Data.Get(), TargetASC);
	}

	SendReticleMessageToOwner(SuccessfulHitLocations);
}

const UPhysicalMaterial* AShootProjectileBase::ResolvePhysicalMaterial(const FOverlapResult& OverlapInfo,
	const FHitResult& ImpactHit) const
{
	if (OverlapInfo.Component.IsValid())
	{
		if (const FBodyInstance* BodyInstance = OverlapInfo.Component->GetBodyInstance())
		{
			if (const UPhysicalMaterial* PhysMat = BodyInstance->GetSimplePhysicalMaterial())
			{
				return PhysMat;
			}
		}
	}

	if (ImpactHit.PhysMaterial.IsValid())
	{
		return ImpactHit.PhysMaterial.Get();
	}

	return nullptr;
}

void AShootProjectileBase::SendReticleMessageToOwner(const TArray<FVector>& HitLocations)
{
	if (HitLocations.Num() == 0)
	{
		// 未命中任何目标则无需推送 HUD
		return;
	}

	AShootPlayerController* ShootPC = Cast<AShootPlayerController>(GetInstigatorController());
	if (!ShootPC)
	{
		// AI 或非玩家控制器可以跳过
		return;
	}

	FShootReticleHitNotifyMessage Message;
	Message.SourceActor = GetInstigator();
	Message.bHasSuccessfulHit = true;

	for (const FVector& Location : HitLocations)
	{
		FShootReticleHitLocation Hit;
		// 对于投射物爆炸，只能提供世界坐标，HUD 再行投影
		Hit.WorldPosition = Location;
		Hit.bHasWorldPosition = true;
		Hit.ScreenPosition = FVector2D::ZeroVector;
		Message.HitMarkers.Add(Hit);
	}

	ShootPC->ClientReceiveReticleHitNotify(Message);
}
void AShootProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (HasAuthority())
	{
		UE_LOG(LogShootProjectileMovement, Verbose,
			TEXT("Projectile hit: Actor=%s Other=%s Location=%s Velocity=%s ImpactPoint=%s ImpactNormal=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OtherActor),
			*GetActorLocation().ToCompactString(),
			MovementComponent ? *MovementComponent->Velocity.ToCompactString() : TEXT("None"),
			*Hit.ImpactPoint.ToCompactString(),
			*Hit.ImpactNormal.ToCompactString());

		// IgnoreActorWhenMoving 处理正常扫掠；这里再保护初始重叠/物理回调，避免自伤 Actor 进入爆炸分支。
		if (!OtherActor || OtherActor == GetOwner() || OtherActor == GetInstigator()
			|| (GetOwner() && OtherActor->IsOwnedBy(GetOwner())))
		{
			return;
		}
		if (ShouldExplodeOnHit(OtherActor, Hit))
		{
			Explode(Hit);
		}
	}
}

bool AShootProjectileBase::ShouldExplodeOnHit(AActor* OtherActor, const FHitResult& Hit) const
{
	// 默认行为：任何阻挡命中都会触发爆炸
	return true;
}

void AShootProjectileBase::HandleFuseExpired()
{
	if (HasAuthority())
	{
		FHitResult Dummy;
		Dummy.ImpactPoint = GetActorLocation();
		Dummy.ImpactNormal = FVector::UpVector;
		Explode(Dummy);
	}
}

void AShootProjectileBase::OnRep_Config()
{
	// 客户端收到配置后刷新拖尾效果
	SpawnOrUpdateTrail();
}

void AShootProjectileBase::SpawnOrUpdateTrail()
{
	if (bIsHeldProjectile)
	{
		if (TrailComponent)
		{
			TrailComponent->Deactivate();
		}
		if (TrailParticleComponent)
		{
			TrailParticleComponent->Deactivate();
		}
		if (VisualMeshComponent)
		{
			VisualMeshComponent->SetStaticMesh(ReplicatedConfig.ProjectileMesh);
			VisualMeshComponent->SetVisibility(ReplicatedConfig.ProjectileMesh != nullptr);
		}
		return;
	}

	if (ReplicatedConfig.TrailSystem)
	{
		if (TrailParticleComponent)
		{
			TrailParticleComponent->Deactivate();
		}
		if (!TrailComponent->IsActive())
		{
			TrailComponent->SetAsset(ReplicatedConfig.TrailSystem);
			TrailComponent->Activate(true);
		}
	}
	else if (ReplicatedConfig.TrailParticleSystem)
	{
		if (TrailComponent)
		{
			TrailComponent->Deactivate();
		}
		if (TrailParticleComponent)
		{
			TrailParticleComponent->SetTemplate(ReplicatedConfig.TrailParticleSystem);
			if (!TrailParticleComponent->IsActive())
			{
				TrailParticleComponent->Activate(true);
			}
		}
	}
	else
	{
		if (TrailComponent)
		{
			TrailComponent->Deactivate();
		}
		if (TrailParticleComponent)
		{
			TrailParticleComponent->Deactivate();
		}
	}

	// 视觉网格由 ReplicatedConfig 数据驱动；服务器在 InitializeProjectile 应用，
	// 客户端在收到配置后的 BeginPlay 里再次同步（与拖尾同一时机）。
	if (VisualMeshComponent)
	{
		VisualMeshComponent->SetStaticMesh(ReplicatedConfig.ProjectileMesh);
		VisualMeshComponent->SetVisibility(ReplicatedConfig.ProjectileMesh != nullptr);
	}
}

void AShootProjectileBase::OnRep_HeldProjectileState()
{
	ApplyHeldProjectileState();
}

void AShootProjectileBase::ApplyHeldProjectileState()
{
	if (!bIsHeldProjectile)
	{
		if (CollisionComponent)
		{
			CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			CollisionComponent->SetGenerateOverlapEvents(true);
		}
		SetActorEnableCollision(true);
		SpawnOrUpdateTrail();
		return;
	}

	if (MovementComponent)
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->Deactivate();
	}
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComponent->SetGenerateOverlapEvents(false);
	}
	SetActorEnableCollision(false);
	SpawnOrUpdateTrail();
}

void AShootProjectileBase::ClearPendingFuseTimer()
{
	if (GetWorldTimerManager().IsTimerActive(FuseTimerHandle))
	{
		GetWorldTimerManager().ClearTimer(FuseTimerHandle);
	}
}
