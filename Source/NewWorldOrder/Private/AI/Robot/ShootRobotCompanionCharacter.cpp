// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AI/Robot/ShootRobotCompanionCharacter.h"

#include "AI/Robot/ShootRobotCompanionController.h"
#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/Abilities/Robot/ShootGA_RobotFire.h"
#include "AbilitySystem/Abilities/Robot/ShootGA_RobotMelee.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/Effects/ShootEffect_DamageSetByCaller.h"
#include "AbilitySystem/Effects/ShootEffect_RobotCompanion.h"
#include "AbilitySystem/Skills/ShootRobotCompanionComponent.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "AIController.h"
#include "Net/UnrealNetwork.h"
#include "Player/ShootPlayerState.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootRobotCompanionCharacter)

AShootRobotCompanionCharacter::AShootRobotCompanionCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	AbilitySystemComponent = CreateDefaultSubobject<UShootAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<UShootAttributeSet>(TEXT("AttributeSet"));

	AIControllerClass = AShootRobotCompanionController::StaticClass();
	AutoPossessAI = EAutoPossessAI::Spawned;
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	// Gameplay Focus 必须支配朝向。两项同时为 true 时，移动朝向会覆盖 AIController 的目标朝向，
	// 造成机器人背对敌人播放射击 Montage；无目标时 PathFollowing 自己仍会提供移动焦点。
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 480.f, 0.f);
	GetCharacterMovement()->bUseRVOAvoidance = true;
	GetCharacterMovement()->AvoidanceConsiderationRadius = 120.f;

	RightWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWeapon"));
	RightWeapon->SetupAttachment(GetMesh());
	RightWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightMuzzle = CreateDefaultSubobject<USceneComponent>(TEXT("RightMuzzle"));
	RightMuzzle->SetupAttachment(RightWeapon);

	LeftWeapon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWeapon"));
	LeftWeapon->SetupAttachment(GetMesh());
	LeftWeapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftMuzzle = CreateDefaultSubobject<USceneComponent>(TEXT("LeftMuzzle"));
	LeftMuzzle->SetupAttachment(LeftWeapon);

	PowerPod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PowerPod"));
	PowerPod->SetupAttachment(GetMesh());
	PowerPod->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RobotVitalsEffectClass = UShootEffect_RobotCompanionVitals::StaticClass();
	DamageEffectClass = UShootEffect_DamageSetByCaller::StaticClass();
}

void AShootRobotCompanionCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyAttachmentSockets();
	InitializeRobotAbilitySystem();
}

void AShootRobotCompanionCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() && AbilitySystemComponent)
	{
		RobotAbilityHandles.TakeFromAbilitySystem(CastChecked<UShootAbilitySystemComponent>(AbilitySystemComponent));
	}
	FireAnimationNotifyDelegate.Clear();
	MeleeAnimationNotifyDelegate.Clear();
	Super::EndPlay(EndPlayReason);
}

void AShootRobotCompanionCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeRobotAbilitySystem();
}

void AShootRobotCompanionCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyAttachmentSockets();
	OnRep_CompanionLevel();
}

void AShootRobotCompanionCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	if (!HasAuthority() || !bSummonDropPending)
	{
		return;
	}

	bSummonDropPending = false;
	ApplyAreaDamageToHostiles(Hit.ImpactPoint, SummonImpactRadius, SummonImpactDamage);
	if (SummonImpactCueTag.IsValid() && AbilitySystemComponent)
	{
		FGameplayCueParameters Parameters;
		Parameters.Location = Hit.ImpactPoint;
		Parameters.Normal = Hit.ImpactNormal;
		Parameters.EffectCauser = this;
		AbilitySystemComponent->ExecuteGameplayCue(SummonImpactCueTag, Parameters);
	}
}

void AShootRobotCompanionCharacter::InitializeCompanion(APlayerState* InOwnerPlayerState,
	const int32 InLevel, const FGameplayTag InMode)
{
	check(HasAuthority());
	CompanionOwnerPlayerState = InOwnerPlayerState;
	CompanionOwnerPawn = InOwnerPlayerState ? InOwnerPlayerState->GetPawn() : nullptr;
	CompanionLevel = FMath::Clamp(InLevel, 1, 3);
	CommandModeTag = InMode;
	CommandLocation = CompanionOwnerPawn ? CompanionOwnerPawn->GetActorLocation() : GetActorLocation();
	SetOwner(CompanionOwnerPawn);
}

void AShootRobotCompanionCharacter::SetCompanionOwnerPawn(APawn* InOwnerPawn)
{
	if (!HasAuthority())
	{
		return;
	}
	CompanionOwnerPawn = InOwnerPawn;
	SetOwner(InOwnerPawn);
	ForceNetUpdate();
}

void AShootRobotCompanionCharacter::SetCompanionLevel(const int32 InLevel)
{
	if (!HasAuthority())
	{
		return;
	}
	const int32 NewLevel = FMath::Clamp(InLevel, 1, 3);
	if (CompanionLevel == NewLevel)
	{
		return;
	}
	CompanionLevel = NewLevel;
	ApplyRobotVitals();
	OnRep_CompanionLevel();
	ForceNetUpdate();
}

void AShootRobotCompanionCharacter::SetCommandMode(const FGameplayTag InMode, const APawn* CommandingPawn)
{
	if (!HasAuthority() || !InMode.IsValid())
	{
		return;
	}
	const FGameplayTag OldMode = CommandModeTag;
	CommandModeTag = InMode;
	// CommandLocation 继续留在黑板契约中，供以后真正的定点指令使用；当前三模式只切换战斗策略。
	const APawn* SourcePawn = CommandingPawn ? CommandingPawn : CompanionOwnerPawn.Get();
	CommandLocation = SourcePawn ? SourcePawn->GetActorLocation() : GetActorLocation();
	OnRep_CommandMode(OldMode);
	if (AShootRobotCompanionController* RobotController = Cast<AShootRobotCompanionController>(GetController()))
	{
		RobotController->RefreshRobotContext();
	}
	ForceNetUpdate();
}

void AShootRobotCompanionCharacter::SelfDestructFromOwnerLoss()
{
	if (!HasAuthority() || GetDeathState() != EShootDeathState::NotDead)
	{
		return;
	}
	// Owner 死亡是技能自爆语义：先按 TeamId 结算周围敌人，再终止 AI 和角色生命周期。
	ExecuteSelfDestruct(TEXT("Robot owner died"));
}

void AShootRobotCompanionCharacter::SelfDestructFromLifetimeExpiry()
{
	if (!HasAuthority() || GetDeathState() != EShootDeathState::NotDead)
	{
		return;
	}
	ExecuteSelfDestruct(TEXT("Robot lifetime expired"));
}

void AShootRobotCompanionCharacter::ExecuteSelfDestruct(const TCHAR* StopReason)
{
	ApplyAreaDamageToHostiles(GetActorLocation(), SelfDestructRadius, SelfDestructDamage);
	StartDeath();
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StopLogic(StopReason);
		}
	}
	MulticastPlayDeathMontage(true);
	FinishDeath();
	SetLifeSpan(FMath::Max(0.1f, DeathLifeSpan));
}

void AShootRobotCompanionCharacter::ApplyAreaDamageToHostiles(const FVector& Origin,
	const float Radius, const float Damage)
{
	if (!HasAuthority() || Radius <= KINDA_SMALL_NUMBER || Damage <= KINDA_SMALL_NUMBER
		|| !AbilitySystemComponent || !DamageEffectClass || !GetWorld())
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RobotCompanionAreaDamage), false, this);
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(CompanionOwnerPawn);
	if (!GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), QueryParams))
	{
		return;
	}

	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!IsValid(TargetActor) || DamagedActors.Contains(TargetActor)
			|| UShootAbilitySystemLibrary::GetTeamAttitudeForActors(this, TargetActor) != ETeamAttitude::Hostile)
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!TargetASC)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		// 机器人是 EffectCauser，召唤者 Pawn 才是 Instigator。这样机器人伤害继续走自身 ASC，
		// 但伤害数字、击杀归属和玩家侧反馈都能回到实际召唤者；Owner 不可用时才退回机器人自身。
		AActor* DamageInstigator = IsValid(CompanionOwnerPawn) ? CompanionOwnerPawn.Get() : this;
		Context.AddInstigator(DamageInstigator, this);
		Context.AddSourceObject(this);
		if (const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
			DamageEffectClass, CompanionLevel, Context); Spec.IsValid())
		{
			FShootGameplayTags::SetSetByCallerMagnitude(
				*Spec.Data.Get(), FShootGameplayTags::Get().SetByCaller_Damage, Damage);
			AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			DamagedActors.Add(TargetActor);
		}
	}
}

bool AShootRobotCompanionCharacter::TryActivateFire(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValid(TargetActor) || GetDeathState() != EShootDeathState::NotDead)
	{
		return false;
	}
	PendingFireTarget = TargetActor;
	bool bActivated = false;
	if (AbilitySystemComponent)
	{
		// RobotAbilitySet 可以授予 GA_RobotFire 的蓝图子类。这里按原生语义基类查找已经授予的 Spec，
		// 再用真实 Handle 激活；不能依赖 CDO 构造期 GameplayTag 缓存，也不硬编码任何 /Game 资产路径。
		for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->IsA<UShootGA_RobotFire>())
			{
				bActivated = AbilitySystemComponent->TryActivateAbility(Spec.Handle);
				break;
			}
		}
	}
	if (!bActivated)
	{
		PendingFireTarget.Reset();
	}
	return bActivated;
}

bool AShootRobotCompanionCharacter::TryActivateAttack(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValid(TargetActor) || GetDeathState() != EShootDeathState::NotDead)
	{
		return false;
	}

	if (AAIController* RobotController = Cast<AAIController>(GetController()))
	{
		RobotController->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	}
	// 不用“伤害射线自动拐向目标”掩盖表现错误。朝向尚未追上时，本轮 BT Task 失败并在下一轮重试。
	if (!IsFacingAttackTarget(TargetActor))
	{
		return false;
	}

	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
	const bool bInsideMeleeRange = FVector::DistSquared(GetActorLocation(), TargetActor->GetActorLocation())
		<= FMath::Square(MeleeRange);
	// 远程压制始终射击，近战强袭只在贴近后攻击，均衡护卫才按距离自动择招。
	if (CommandModeTag == GameplayTags.Ability_Mode_Robot_Ranged)
	{
		return TryActivateFire(TargetActor);
	}
	if (CommandModeTag == GameplayTags.Ability_Mode_Robot_Melee)
	{
		return bInsideMeleeRange && TryActivateMelee(TargetActor);
	}
	if (bInsideMeleeRange && TryActivateMelee(TargetActor))
	{
		return true;
	}
	return TryActivateFire(TargetActor);
}

bool AShootRobotCompanionCharacter::TryActivateMelee(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValid(TargetActor) || GetDeathState() != EShootDeathState::NotDead)
	{
		return false;
	}
	PendingMeleeTarget = TargetActor;
	bool bActivated = false;
	if (AbilitySystemComponent)
	{
		for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			if (Spec.Ability && Spec.Ability->IsA<UShootGA_RobotMelee>())
			{
				bActivated = AbilitySystemComponent->TryActivateAbility(Spec.Handle);
				break;
			}
		}
	}
	if (!bActivated)
	{
		PendingMeleeTarget.Reset();
	}
	return bActivated;
}

void AShootRobotCompanionCharacter::NotifyFireAnimationEvent()
{
	if (HasAuthority())
	{
		FireAnimationNotifyDelegate.Broadcast();
	}
}

void AShootRobotCompanionCharacter::NotifyMeleeAnimationEvent()
{
	if (HasAuthority())
	{
		MeleeAnimationNotifyDelegate.Broadcast();
	}
}

float AShootRobotCompanionCharacter::GetAggroRangeForCurrentMode() const
{
	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
	if (CommandModeTag == GameplayTags.Ability_Mode_Robot_Ranged)
	{
		return RangedAggroRange;
	}
	return CommandModeTag == GameplayTags.Ability_Mode_Robot_Melee ? MeleeAggroRange : BalancedAggroRange;
}

float AShootRobotCompanionCharacter::GetDesiredCombatRange() const
{
	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
	if (CommandModeTag == GameplayTags.Ability_Mode_Robot_Ranged)
	{
		return RangedDesiredCombatRange;
	}
	return CommandModeTag == GameplayTags.Ability_Mode_Robot_Melee
		? MeleeDesiredCombatRange
		: BalancedDesiredCombatRange;
}

float AShootRobotCompanionCharacter::GetFireInterval() const
{
	const float LevelInterval = FMath::Max(0.05f,
		BaseFireInterval - (CompanionLevel - 1) * FireIntervalPerLevel);
	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
	if (CommandModeTag == GameplayTags.Ability_Mode_Robot_Ranged)
	{
		return LevelInterval * RangedFireIntervalMultiplier;
	}
	return CommandModeTag == GameplayTags.Ability_Mode_Robot_Balanced
		? LevelInterval * BalancedFireIntervalMultiplier
		: LevelInterval;
}

float AShootRobotCompanionCharacter::GetMeleeInterval(const bool bChomp) const
{
	return bChomp ? ChompInterval : ClawInterval;
}

float AShootRobotCompanionCharacter::GetMeleeDamage(const bool bChomp) const
{
	const float LevelDamage = BaseMeleeDamage + (CompanionLevel - 1) * MeleeDamagePerLevel;
	return bChomp ? LevelDamage * ChompDamageMultiplier : LevelDamage;
}

bool AShootRobotCompanionCharacter::ChooseChompAttack()
{
	if (CommandModeTag != FShootGameplayTags::Get().Ability_Mode_Robot_Melee || !ChompMontage)
	{
		return false;
	}
	++MeleeAttackSequence;
	return MeleeAttackSequence % 3 == 0;
}

bool AShootRobotCompanionCharacter::IsFacingAttackTarget(const AActor* TargetActor) const
{
	if (!IsValid(TargetActor))
	{
		return false;
	}
	const FVector ToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	return !ToTarget.IsNearlyZero()
		&& FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), ToTarget) >= AttackFacingDotThreshold;
}

void AShootRobotCompanionCharacter::ApplyMeleeImpact(AActor* PreferredTarget, const bool bChomp)
{
	if (!HasAuthority() || !IsValid(PreferredTarget) || !IsFacingAttackTarget(PreferredTarget))
	{
		return;
	}
	const FVector Origin = GetActorLocation() + GetActorForwardVector() * 105.f;
	ApplyAreaDamageToHostiles(Origin, GetMeleeRadius(bChomp), GetMeleeDamage(bChomp));
}

USceneComponent* AShootRobotCompanionCharacter::GetNextMuzzle(bool& bOutLeftMuzzle)
{
	bOutLeftMuzzle = CompanionLevel >= 3 && bUseLeftMuzzleNext;
	bUseLeftMuzzleNext = CompanionLevel >= 3 && !bUseLeftMuzzleNext;
	return bOutLeftMuzzle ? LeftMuzzle : RightMuzzle;
}

void AShootRobotCompanionCharacter::PlayReplicatedFireMontage(const bool bLeftMuzzle)
{
	if (HasAuthority())
	{
		MulticastPlayFireMontage(bLeftMuzzle);
	}
}

void AShootRobotCompanionCharacter::PlayReplicatedMeleeMontage(const bool bChomp)
{
	if (HasAuthority())
	{
		MulticastPlayMeleeMontage(bChomp);
	}
}

void AShootRobotCompanionCharacter::Die(const FVector& DeathImpulse)
{
	if (GetDeathState() != EShootDeathState::NotDead)
	{
		return;
	}
	StartDeath();
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AIController->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Robot destroyed in combat"));
		}
	}
	if (HasAuthority())
	{
		if (AShootPlayerState* OwnerPS = Cast<AShootPlayerState>(CompanionOwnerPlayerState))
		{
			if (UShootRobotCompanionComponent* Component = OwnerPS->GetRobotCompanionComponent())
			{
				Component->NotifyRobotCombatDestroyed(this);
			}
		}
		MulticastPlayDeathMontage(false);
		FinishDeath();
		SetLifeSpan(FMath::Max(0.1f, DeathLifeSpan));
	}
}

void AShootRobotCompanionCharacter::MulticastPlayFireMontage_Implementation(const bool bLeftMuzzle)
{
	if (UAnimMontage* Montage = bLeftMuzzle && FireLeftMontage ? FireLeftMontage : FireRightMontage)
	{
		PlayAnimMontage(Montage);
	}
}

void AShootRobotCompanionCharacter::MulticastPlayMeleeMontage_Implementation(const bool bChomp)
{
	if (UAnimMontage* Montage = bChomp && ChompMontage ? ChompMontage : ClawLeftMontage)
	{
		PlayAnimMontage(Montage);
	}
}

void AShootRobotCompanionCharacter::MulticastPlayDeathMontage_Implementation(const bool bSelfDestruct)
{
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
	if (bSelfDestruct && SelfDestructCueTag.IsValid() && AbilitySystemComponent)
	{
		FGameplayCueParameters Parameters;
		Parameters.Location = GetActorLocation();
		Parameters.EffectCauser = this;
		AbilitySystemComponent->ExecuteGameplayCue(SelfDestructCueTag, Parameters);
	}
}

void AShootRobotCompanionCharacter::OnRep_CommandMode(const FGameplayTag OldModeTag)
{
	BP_OnCommandModeChanged(CommandModeTag);
}

void AShootRobotCompanionCharacter::OnRep_CompanionLevel()
{
	if (LeftWeapon)
	{
		// Lv3 才解锁双枪；网格与相对 Transform 仍由机器人蓝图维护。
		LeftWeapon->SetVisibility(CompanionLevel >= 3, true);
	}
}

void AShootRobotCompanionCharacter::InitializeRobotAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		return;
	}
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	if (!AbilitySystemComponent->HasMatchingGameplayTag(FShootGameplayTags::Get().Faction_Player))
	{
		AbilitySystemComponent->AddLooseGameplayTag(FShootGameplayTags::Get().Faction_Player);
	}
	if (HasAuthority() && !bRobotAbilitySystemInitialized)
	{
		ApplyRobotVitals();
		if (RobotAbilitySet)
		{
			RobotAbilitySet->GiveToAbilitySystem(
				CastChecked<UShootAbilitySystemComponent>(AbilitySystemComponent), &RobotAbilityHandles, this,
				CompanionLevel);
		}
		bRobotAbilitySystemInitialized = true;
	}
}

void AShootRobotCompanionCharacter::ApplyRobotVitals()
{
	if (!HasAuthority() || !AbilitySystemComponent || !RobotVitalsEffectClass)
	{
		return;
	}
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);
	if (const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(
		RobotVitalsEffectClass, CompanionLevel, Context); Spec.IsValid())
	{
		const float MaxHealth = BaseMaxHealth + (CompanionLevel - 1) * MaxHealthPerLevel;
		Spec.Data->SetSetByCallerMagnitude(FName("SetByCaller.Robot.MaxHealth"), MaxHealth);
		Spec.Data->SetSetByCallerMagnitude(FName("SetByCaller.Robot.Health"), MaxHealth);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void AShootRobotCompanionCharacter::ApplyAttachmentSockets()
{
	if (RightWeapon)
	{
		RightWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, RightWeaponSocket);
	}
	if (LeftWeapon)
	{
		LeftWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, LeftWeaponSocket);
	}
	if (PowerPod)
	{
		PowerPod->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, PowerPodSocket);
	}
}

void AShootRobotCompanionCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, CompanionOwnerPlayerState);
	DOREPLIFETIME(ThisClass, CompanionOwnerPawn);
	DOREPLIFETIME(ThisClass, CompanionLevel);
	DOREPLIFETIME(ThisClass, CommandModeTag);
	DOREPLIFETIME(ThisClass, CommandLocation);
}
