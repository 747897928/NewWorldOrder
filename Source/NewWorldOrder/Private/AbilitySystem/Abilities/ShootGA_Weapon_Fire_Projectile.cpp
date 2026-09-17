// Copyright ZhaoYiJie

#include "AbilitySystem/Abilities/ShootGA_Weapon_Fire_Projectile.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystem/Abilities/ShootAbilityCost_AmmoTagStack.h"
#include "AbilitySystem/Effects/ShootEffect_DamageBase.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Weapons/Projectiles/ShootProjectileBase.h"
#include "Weapons/ShootRangedWeaponInstance.h"

UShootGA_Weapon_Fire_Projectile::UShootGA_Weapon_Fire_Projectile(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetAssetTags(FGameplayTagContainer(
		FGameplayTag::RequestGameplayTag(FName("Ability.Type.Action.WeaponFire"), false)));
	ActivationOwnedTags.AddTag(
		FGameplayTag::RequestGameplayTag(FName("Event.Movement.WeaponFire"), false));
	// 特殊武器也必须遵守换弹/眩晕等系统授予的 NoFiring 来源标签，不能因为生成实体投射物就绕过阻挡。
	SourceBlockedTags.AddTag(
		FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.NoFiring"), false));
	StartupInputTag = FGameplayTag::RequestGameplayTag(FName("InputTag.LMB"), false);

	if (!AdditionalCosts.ContainsByPredicate(
		[](const UShootAbilityCost* Cost)
		{
			return Cost && Cost->IsA<UShootAbilityCost_AmmoTagStack>();
		}))
	{
		UShootAbilityCost_AmmoTagStack* AmmoCost =
			ObjectInitializer.CreateDefaultSubobject<UShootAbilityCost_AmmoTagStack>(
				this, TEXT("ProjectileAmmoCost"));
		AdditionalCosts.Add(AmmoCost);
	}
}

FVector UShootGA_Weapon_Fire_Projectile::GetMuzzleLocation() const
{
	if (UShootRangedWeaponInstance* WeaponInstance = GetWeaponInstance())
	{
		// 装备 Actor 复制稍晚时不能把投射物生成到世界原点；此时退回 Avatar 位置，
		// 等装备表现链完成后下一次开火再使用 RangedWeaponConfig.MuzzleSocketName 指向的真实 Socket。
		const FVector MuzzleLocation = WeaponInstance->GetMuzzleLocation();
		if (!MuzzleLocation.IsNearlyZero())
		{
			return MuzzleLocation;
		}
	}

	if (const AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		return Avatar->GetActorLocation();
	}

	return FVector::ZeroVector;
}

bool UShootGA_Weapon_Fire_Projectile::HasValidProjectileConfig() const
{
	const UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	const TSubclassOf<UGameplayEffect> DamageEffectClass = Weapon
		? Weapon->GetDamageGameplayEffect()
		: nullptr;
	return Weapon
		&& Weapon->IsProjectileWeapon()
		&& Weapon->GetProjectileClass()
		// MaxRange 定义在 ID 蓝图的 UShootInventoryFragment_RangedWeaponConfig；
		// 代码不提供隐藏下限，配置无效就拒绝开火，避免蓝图数值与实际弹道不一致。
		&& Weapon->GetMaxDamageRange() > KINDA_SMALL_NUMBER
		&& DamageEffectClass
		&& DamageEffectClass->IsChildOf(UShootEffect_DamageBase::StaticClass());
}

void UShootGA_Weapon_Fire_Projectile::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 父类 UShootGameplayAbility_Weapon_Fire 绑定与 Rifle/Pistol/Shotgun 相同的 TargetData 委托，
	// 并统一负责 LocalPredicted、Commit、散布/后坐力以及结束时清理。Hitscan 子类仍由服务器重 Trace；
	// 本实体投射物子类保留所属客户端的准星目标点，并在 ResolveFireDirectionFromTargetData 中校验。
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!HasValidProjectileConfig())
	{
		UE_LOG(LogTemp, Error,
			TEXT("Projectile Ability %s requires ProjectileClass, Damage GE and a positive RangedWeaponConfig.MaxRange."),
			*GetNameSafe(this));
		K2_EndAbility();
		return;
	}

	// 与 UShootGA_Weapon_Fire_Rifle 共用 TargetData 复制链：拥有客户端立即预测 Trace；远程服务器实例等待
	// 父类 TargetData Delegate。差异仅在服务器结算：实体弹使用校验后的客户端准星点，避免第三人称
	// 近距离视差把服务器枪口导向另一条线；Rifle/Pistol/Shotgun 的父类默认仍使用服务器重 Trace。
	// Simulated Proxy 不运行 GA，只接收服务器复制的 Projectile Actor、Montage 与 GameplayCue。
	if (IsLocallyControlled())
	{
		StartRangedWeaponTargeting();
	}

	FTimerHandle TargetDataTimeoutHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TargetDataTimeoutHandle,
		[this]() { if (IsActive()) K2_EndAbility(); },
		FMath::Max(EndAbilityDelaySecs, 1.0f),
		false);
}

void UShootGA_Weapon_Fire_Projectile::OnRangedWeaponTargetDataReady_Implementation(
	const FGameplayAbilityTargetDataHandle& TargetData)
{
	FVector FireDirection;
	if (!ResolveFireDirectionFromTargetData(TargetData, FireDirection))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Projectile Ability %s received no usable targeting result."), *GetNameSafe(this));
		K2_EndAbility();
		return;
	}

	// 进入本回调前，父类 OnTargetDataReadyCallback 已完成 Commit；客户端用于即时表现，
	// 服务器保留所属客户端的准星目标点，并在 ResolveFireDirectionFromTargetData 中校验后生成唯一权威投射物。
	ExecuteCommittedFire(FireDirection);
}

bool UShootGA_Weapon_Fire_Projectile::ResolveFireDirectionFromTargetData(
	const FGameplayAbilityTargetDataHandle& TargetData,
	FVector& OutFireDirection) const
{
	if (TargetData.Num() == 0)
	{
		return false;
	}

	const FHitResult* HitResult = TargetData.Get(0)->GetHitResult();
	if (!HitResult)
	{
		return false;
	}

	const bool bBlockingAim = HitResult->bBlockingHit;
	const FVector AimPoint = bBlockingAim && !HitResult->ImpactPoint.IsNearlyZero()
		? HitResult->ImpactPoint
		: HitResult->TraceEnd;

	const FVector MuzzleLocation = GetMuzzleLocation();
	const UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	if (!Weapon || AimPoint.ContainsNaN() || (AimPoint - MuzzleLocation).IsNearlyZero())
	{
		return false;
	}
	const FProjectileWeaponConfig& ProjectileConfig = Weapon->GetProjectileConfig();

	if (K2_HasAuthority())
	{
		APawn* AvatarPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
		if (!AvatarPawn)
		{
			return false;
		}

		// 服务器只采纳“准星目标点”，不采纳客户端声称命中的 Actor 或伤害结果。
		// 目标必须落在 ID 蓝图 MaxRange 内，并与服务器控制旋转保持在可配置夹角内；
		// 后续实体弹的扫掠、碰撞、爆炸和 GE 伤害仍全部由服务器权威决定。
		const FTransform ServerTargetingTransform = GetTargetingTransform(AvatarPawn);
		const FVector ServerViewToAim = AimPoint - ServerTargetingTransform.GetLocation();
		const float SubmittedDistance = ServerViewToAim.Size();
		const FVector ServerAimDirection = ServerTargetingTransform.GetUnitAxis(EAxis::X).GetSafeNormal();
		const FVector SubmittedDirection = ServerViewToAim.GetSafeNormal();
		const float MaxAimErrorDegrees = FMath::Clamp(
			ProjectileConfig.MaxServerAimErrorDegrees, 0.0f, 180.0f);
		const float AimDot = FVector::DotProduct(ServerAimDirection, SubmittedDirection);
		const float MinimumAimDot = FMath::Cos(FMath::DegreesToRadians(MaxAimErrorDegrees));

		if (SubmittedDistance <= KINDA_SMALL_NUMBER
			|| SubmittedDistance > Weapon->GetMaxDamageRange()
			|| AimDot < MinimumAimDot)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Projectile target rejected: Ability=%s Distance=%.1f MaxRange=%.1f AimDot=%.3f MinimumAimDot=%.3f"),
				*GetNameSafe(this), SubmittedDistance, Weapon->GetMaxDamageRange(),
				AimDot, MinimumAimDot);
			return false;
		}
	}

	OutFireDirection = (AimPoint - MuzzleLocation).GetSafeNormal();
	if (bBlockingAim && ProjectileConfig.bUseBallisticAim
		&& ProjectileConfig.InitialSpeed > KINDA_SMALL_NUMBER
		&& ProjectileConfig.GravityScale > KINDA_SMALL_NUMBER)
	{
		// 由 UE 使用同一 InitialSpeed 和缩放后的世界重力求解低/高弧，替代项目内重复的抛体公式。
		// 只对真实阻挡点求解；目标不可达时保持枪口到准星的直射初速度，再由 ProjectileMovement 自然下坠。
		UGameplayStatics::FSuggestProjectileVelocityParameters Parameters(
			this, MuzzleLocation, AimPoint, ProjectileConfig.InitialSpeed);
		Parameters.bFavorHighArc = ProjectileConfig.bFavorHighArc;
		Parameters.OverrideGravityZ = GetWorld()->GetGravityZ() * ProjectileConfig.GravityScale;
		Parameters.TraceOption = ESuggestProjVelocityTraceOption::DoNotTrace;

		FVector SuggestedVelocity;
		if (UGameplayStatics::SuggestProjectileVelocity(Parameters, SuggestedVelocity))
		{
			OutFireDirection = SuggestedVelocity.GetSafeNormal();
		}
	}

	// 该日志只服务投射物准星方向排障；正常开火会高频经过此处，禁止污染默认输出日志。
	UE_LOG(LogTemp, Verbose,
		TEXT("ProjectileTargetResolved Local=%d Authority=%d Avatar=%s Blocking=%d Muzzle=%s AimPoint=%s ViewDir=%s FireDir=%s Dot=%.3f"),
		IsLocallyControlled() ? 1 : 0,
		K2_HasAuthority() ? 1 : 0,
		*GetNameSafe(GetAvatarActorFromActorInfo()),
		bBlockingAim ? 1 : 0,
		*MuzzleLocation.ToCompactString(),
		*AimPoint.ToCompactString(),
		*GetAimDirection().ToCompactString(),
		*OutFireDirection.ToCompactString(),
		FVector::DotProduct(GetAimDirection().GetSafeNormal(), OutFireDirection));

	return !OutFireDirection.IsNearlyZero();
}

void UShootGA_Weapon_Fire_Projectile::ExecuteCommittedFire(const FVector& FireDirection)
{
	if (FireGameplayCueTag.IsValid())
	{
		FGameplayCueParameters CueParameters;
		if (UShootRangedWeaponInstance* RangedWeaponInstance = GetWeaponInstance())
		{
			CueParameters.SourceObject = RangedWeaponInstance;
			CueParameters.Location = RangedWeaponInstance->GetMuzzleLocation();
			CueParameters.Normal = FireDirection;
		}
		K2_ExecuteGameplayCueWithParams(FireGameplayCueTag, CueParameters);
	}

	if (K2_HasAuthority())
	{
		SpawnProjectile(FireDirection);
	}

	UAnimMontage* FireMontage = nullptr;
	if (UShootRangedWeaponInstance* WeaponInstance = GetWeaponInstance())
	{
		FireMontage = WeaponInstance->GetCharacterFireMontage();
	}
	if (FireMontage)
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this, NAME_None, FireMontage, 1.0f, NAME_None,
			false, 1.0f, 0.0f, false);
		MontageTask->OnInterrupted.AddDynamic(this, &UShootGA_Weapon_Fire_Projectile::K2_EndAbility);
		MontageTask->OnCancelled.AddDynamic(this, &UShootGA_Weapon_Fire_Projectile::K2_EndAbility);
		MontageTask->OnCompleted.AddDynamic(this, &UShootGA_Weapon_Fire_Projectile::K2_EndAbilityLocally);
		MontageTask->ReadyForActivation();
	}

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		[this]() { if (IsActive()) K2_EndAbility(); },
		EndAbilityDelaySecs,
		false);
}

void UShootGA_Weapon_Fire_Projectile::SpawnProjectile(const FVector& FireDirection)
{
	UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	if (!Weapon || !Weapon->IsProjectileWeapon())
	{
		return;
	}

	TSubclassOf<AShootProjectileBase> ClassToSpawn = Weapon->GetProjectileClass();
	TSubclassOf<UGameplayEffect> DamageEffectClass = Weapon->GetDamageGameplayEffect();
	if (!ClassToSpawn || !DamageEffectClass
		|| !DamageEffectClass->IsChildOf(UShootEffect_DamageBase::StaticClass()))
	{
		UE_LOG(LogTemp, Error,
			TEXT("Projectile Ability %s has an invalid ProjectileClass or Damage GE."), *GetNameSafe(this));
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	const FVector SpawnLocation = GetMuzzleLocation();
	const FVector NormalizedFireDirection = FireDirection.GetSafeNormal();
	if (NormalizedFireDirection.IsNearlyZero())
	{
		// ResolveFireDirectionFromTargetData 已验证准星目标和枪口不能重合；这里不再偷偷回退到
		// 另一条相机方向，否则上游 TargetData 错误会表现成玩家看到的“投射物拐弯”。
		return;
	}

	const FRotator SpawnRotation = NormalizedFireDirection.Rotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	APawn* InstigatorPawn = Cast<APawn>(GetAvatarActorFromActorInfo());

	bool bNeedsFinishSpawning = false;
	AShootProjectileBase* Projectile = Weapon->TakeProjectileForLaunch(SpawnTransform);
	if (!Projectile)
	{
		Projectile = GetWorld()->SpawnActorDeferred<AShootProjectileBase>(
			ClassToSpawn,
			SpawnTransform,
			Weapon->GetPawn(),
			InstigatorPawn,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		bNeedsFinishSpawning = Projectile != nullptr;
	}
	if (!Projectile)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
	EffectContext.AddSourceObject(Weapon);
	EffectContext.AddInstigator(InstigatorPawn, InstigatorPawn);

	Projectile->InitializeProjectile(
		Weapon->GetProjectileConfig(),
		Weapon,
		SourceASC,
		DamageEffectClass,
		GetAbilityLevel(),
		EffectContext,
		NormalizedFireDirection);
	if (bNeedsFinishSpawning)
	{
		Projectile->FinishSpawning(SpawnTransform);
	}
}
