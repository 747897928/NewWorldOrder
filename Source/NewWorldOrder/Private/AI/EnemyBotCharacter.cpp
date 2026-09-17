// Copyright ZhaoYiJie

#include "AI/EnemyBotCharacter.h"

#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "AbilitySystem/Effects/ShootEffect_DamageSetByCaller.h"
#include "AI/EnemyBotController.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbility.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interface/CombatInterface.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"
#include "ShootGameplayTags.h"
#include "TimerManager.h"


// Sets default values
AEnemyBotCharacter::AEnemyBotCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	// AShootCharacterBase 已给 Capsule/Mesh 分别设置 LyraPawnCapsule/LyraPawnMesh。
	// 主 Weapon 射线必须穿过 Capsule 命中 Mesh 的 PhysicsAsset，才能取得 BoneName 和 WeakSpot PhysMat；
	// 粗略胶囊查询应使用独立的 Weapon_Capsule 通道，不能让 Capsule 抢占主 Weapon 命中。

	AbilitySystemComponent = CreateDefaultSubobject<UShootAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	// Zombie 的 Locomotion 动画是前进方向动画，移动时必须由速度方向驱动 Actor 朝向。
	// 如果同时使用 Controller Desired Rotation，AI Focus 会让 Pawn 面向玩家，而 PathFollowing/RVO
	// 仍可能把速度推向侧后方，结果就是“面向玩家倒退/横移”。近战开始时会单独对准目标。
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	// 群体避障属于敌人移动能力，不属于某张测试地图的 Spawner。所有放置/运行时生成敌人保持一致。
	// 当前 Behavior Tree 尚未提供接敌槽位；让所有 Zombie 对同一 Pawn 启用 RVO，会把近战者推离目标，
	// 同时 Controller 仍保持 Focus，形成面向目标平移/后退。接敌槽位或 CrowdFollowing 完成后再恢复 RVO。
	GetCharacterMovement()->bUseRVOAvoidance = false;

	AttributeSet = CreateDefaultSubobject<UShootAttributeSet>("AttributeSet");

	// 机器人等友方 AI 必须稳定感知 Zombie；显式 Sight Source 不依赖全局自动注册 Pawn 配置。
	SightStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("SightStimuliSource"));
	SightStimuliSource->RegisterForSense(UAISense_Sight::StaticClass());

	// 只配置原生逻辑类，不加载 /Game 资产。各 Zombie 蓝图子类可以覆盖为自己的攻击 GE。
	TestAttackEffectClass = UShootEffect_DamageSetByCaller::StaticClass();

	/*HealthBar = CreateDefaultSubobject<UWidgetComponent>("HealthBar");
	HealthBar->SetupAttachment(GetRootComponent());

	GetMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
	GetMesh()->MarkRenderStateDirty();
	Weapon->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
	Weapon->MarkRenderStateDirty();*/
}

void AEnemyBotCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	EnemyBotController = Cast<AEnemyBotController>(NewController);

	// ASC 归属（AI：Owner=Character, Avatar=Character）
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		if (!AbilitySystemComponent->HasMatchingGameplayTag(FShootGameplayTags::Get().Faction_Enemy))
		{
			AbilitySystemComponent->AddLooseGameplayTag(FShootGameplayTags::Get().Faction_Enemy);
		}

		if (HasAuthority())
		{
			if (!bAbilityDefaultsInitialized)
			{
				// 调用链：AIController Possess -> 初始化 Character 自有 ASC -> 应用蓝图配置的默认属性。
				// 这里不硬编码敌人生命值；BP_EnemyBotCharacter 使用的是 AShootCharacterBase 上的三组 Default Attributes。
				InitializeDefaultAttributes();
				bAbilityDefaultsInitialized = true;
			}

			if (ZombieAbilitySet && !bZombieAbilitySetGranted)
			{
				// Lyra 风格：能力、冷却和附加效果的组合由 DataAsset 授予，并记录句柄，
				// 这样 UnPossessed/重生时可以完整取回，不会把旧 Pawn 的能力规格留在 ASC。
				if (UShootAbilitySystemComponent* ShootASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent))
				{
					ZombieAbilitySet->GiveToAbilitySystem(
						ShootASC,
						&ZombieAbilitySetGrantedHandles,
						this,
						GetPlayerLevel_Implementation());
					bZombieAbilitySetGranted = true;
				}
			}
			else if (!ZombieAbilitySet && !bCharacterAbilitiesGranted)
			{
				// 仍未迁移到 AbilitySet 的旧 AI 类继续走基类兼容入口；正式 Zombie archetype
				// 应配置 ZombieAbilitySet 并清空 StartupAbilities，避免形成两套授予来源。
				AddCharacterAbilities();
				bCharacterAbilitiesGranted = true;
			}
		}
	}

	// 黑板与 Behavior Tree 的生命周期由 AEnemyBotController::OnPossess 统一管理。
}

void AEnemyBotCharacter::UnPossessed()
{
	if (HasAuthority())
	{
		if (AbilitySystemComponent)
		{
			// 先终止正在播放的能力，再移除 AbilitySet；否则 Montage/Notify 可能在取回句柄后
			// 继续访问已经不属于当前 Pawn 生命周期的目标状态。
			AbilitySystemComponent->CancelAllAbilities();
		}

		PendingMeleeTarget.Reset();
		RemoveZombieAbilitySet();
	}

	EnemyBotController = nullptr;
	Super::UnPossessed();
}

int32 AEnemyBotCharacter::GetPlayerLevel_Implementation()
{
	return Level;
}

void AEnemyBotCharacter::Die(const FVector& DeathImpulse)
{
	/*SetLifeSpan(LifeSpan);
	if (EnemyBotController) EnemyBotController->GetBlackboardComponent()->SetValueAsBool(FName("Dead"), true);
	SpawnLoot();*/
	Super::Die(DeathImpulse);
}

// Called when the game starts or when spawned
void AEnemyBotCharacter::BeginPlay()
{
	Super::BeginPlay();
	InitialSpawnTransform = GetActorTransform();
	ApplyTestSkeletalMesh();
	ApplyTestAnimationState();
	// ACharacter 在 CharacterMovement 完成本帧更新后广播该事件；无需再为表现状态保留 Actor Tick。
	OnCharacterMovementUpdated.AddDynamic(this, &ThisClass::HandleMovementUpdated);
	if (HasAuthority() && AbilitySystemComponent)
	{
		// 默认属性必须等 PossessedBy 完成 ASC ActorInfo 初始化后再应用。
		// BeginPlay 不再重复施加 GE，避免重接管时永久属性叠加。
	}
}

void AEnemyBotCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->CancelAllAbilities();
		}
		RemoveZombieAbilitySet();
	}

	// GA 使用 AddUObject 监听命中帧；清理委托使 Pawn 被销毁/重生时不会保留迁移期监听关系。
	MeleeAnimationNotifyDelegate.Clear();
	Super::EndPlay(EndPlayReason);
}

void AEnemyBotCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyTestSkeletalMesh();
}

void AEnemyBotCharacter::SetArchetypeAnimationState(EShootEnemyTestAnimationState NewState)
{
	if (!HasAuthority() || TestAnimationState == NewState)
	{
		return;
	}

	TestAnimationState = NewState;
	ApplyTestAnimationState();
	ForceNetUpdate();
}

bool AEnemyBotCharacter::IsAttackTargetInRange(const AActor& Target) const
{
	return FVector::DistSquared2D(GetActorLocation(), Target.GetActorLocation())
		<= FMath::Square(TestAttackRange);
}

bool AEnemyBotCharacter::TryActivateMeleeAttack(AActor* TargetActor)
{
	if (!HasAuthority() || !IsValid(TargetActor) || GetDeathState() != EShootDeathState::NotDead
		|| UShootAbilitySystemLibrary::GetTeamAttitudeForActors(this, TargetActor) != ETeamAttitude::Hostile
		|| (TargetActor->Implements<UCombatInterface>() && ICombatInterface::Execute_IsDead(TargetActor))
		|| !IsAttackTargetInRange(*TargetActor)
		|| !UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
	{
		return false;
	}

	if (AAIController* AIControllerInstance = Cast<AAIController>(GetController()))
	{
		AIControllerInstance->StopMovement();
		AIControllerInstance->SetFocus(TargetActor, EAIFocusPriority::Gameplay);
	}
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
	}

	// 攻击 Service 可能在追击 MoveTo 仍运行时进入攻击距离；此处把攻击起始朝向固定到目标，
	// 避免等待下一帧旋转时被 Ability 的朝向校验拒绝。命中 Notify 仍会再次复核朝向。
	const FVector ToTarget = (TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero())
	{
		return false;
	}
	SetActorRotation(FRotator(0.0f, ToTarget.Rotation().Yaw, 0.0f));
	if (!IsMeleeAttackTargetValid(*TargetActor))
	{
		return false;
	}

	PendingMeleeTarget = TargetActor;
	const FGameplayTag ZombieMeleeTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Ability.Skill.Zombie.Melee")), false);
	FGameplayTagContainer AbilityTags;
	if (ZombieMeleeTag.IsValid())
	{
		AbilityTags.AddTag(ZombieMeleeTag);
	}
	const bool bActivated = AbilitySystemComponent
		&& AbilityTags.Num() > 0
		&& AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags);

	if (!bActivated)
	{
		PendingMeleeTarget.Reset();
	}
	return bActivated;
}

void AEnemyBotCharacter::RemoveZombieAbilitySet()
{
	if (!bZombieAbilitySetGranted)
	{
		return;
	}

	if (HasAuthority())
	{
		if (UShootAbilitySystemComponent* ShootASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent))
		{
			ZombieAbilitySetGrantedHandles.TakeFromAbilitySystem(ShootASC);
		}
	}

	// 即使 ASC 已经在销毁流程中，也要清除本地标记，保证 EndPlay/UnPossessed 重入安全。
	bZombieAbilitySetGranted = false;
}

void AEnemyBotCharacter::NotifyMeleeAnimationEvent()
{
	if (HasAuthority())
	{
		MeleeAnimationNotifyDelegate.Broadcast();
	}
}

void AEnemyBotCharacter::PlayReplicatedMeleeMontage()
{
	if (HasAuthority())
	{
		MulticastPlayMeleeMontage();
	}
}

bool AEnemyBotCharacter::IsMeleeAttackTargetValid(const AActor& Target) const
{
	if (!IsValid(&Target) || &Target == this || GetDeathState() != EShootDeathState::NotDead
		|| (Target.Implements<UCombatInterface>() && ICombatInterface::Execute_IsDead(&Target))
		|| UShootAbilitySystemLibrary::GetTeamAttitudeForActors(this, &Target) != ETeamAttitude::Hostile
		|| !UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(&Target)
		|| !IsAttackTargetInRange(Target))
	{
		return false;
	}

	const FVector ToTarget = (Target.GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	if (ToTarget.IsNearlyZero() || FVector::DotProduct(Forward, ToTarget) < TestAttackFacingDotThreshold)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const float SourceHalfHeight = GetCapsuleComponent()
		? GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f
		: 50.0f;
	const FVector Start = GetActorLocation() + FVector(0.0f, 0.0f, SourceHalfHeight);
	const FVector End = Target.GetActorLocation() + FVector(0.0f, 0.0f, SourceHalfHeight);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ZombieMeleeLineOfSight), true, this);
	QueryParams.AddIgnoredActor(this);

	TArray<FHitResult> HitResults;
	if (!World->LineTraceMultiByChannel(HitResults, Start, End, ECC_Visibility, QueryParams))
	{
		return true;
	}

	// Zombie 群体的 Capsule/Mesh 都会阻挡 Visibility；近战视线不能把排队中的其他角色当成墙，
	// 否则拥挤时所有 Zombie 都会在攻击任务里失败。角色之间的碰撞/避让由移动系统处理，
	// 这里仅拦截真正的世界几何遮挡，同时继续要求命中目标本身，保持服务器命中复核。
	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor == &Target
			|| (Hit.GetComponent() && Hit.GetComponent()->GetOwner() == &Target))
		{
			return true;
		}

		if (HitActor && HitActor->IsA<ACharacter>())
		{
			continue;
		}

		return false;
	}

	return true;
}

void AEnemyBotCharacter::OnRep_TestAnimationState()
{
	ApplyTestAnimationState();
}

void AEnemyBotCharacter::ApplyTestSkeletalMesh()
{
	if (TestSkeletalMesh && GetMesh() && GetMesh()->GetSkeletalMeshAsset() != TestSkeletalMesh)
	{
		// TestSkeletalMesh 由具体 Zombie 蓝图子类配置；基类只应用该类的固定表现，不做随机换皮。
		GetMesh()->SetSkeletalMeshAsset(TestSkeletalMesh);
	}
	if (TestAnimClass && GetMesh() && GetMesh()->GetAnimClass() != TestAnimClass)
	{
		// 由 archetype Class Defaults 提供正式 AnimBP；不要再用 PlayAnimation 抢占 AnimGraph。
		GetMesh()->SetAnimInstanceClass(TestAnimClass);
	}
}

void AEnemyBotCharacter::ApplyTestAnimationState()
{
	if (TestAnimClass)
	{
		// 正式 AnimBP 已由 ApplyTestSkeletalMesh 设置到 CharacterMesh0；此时不能再调用
		// PlayAnimation 切到 Single Node。枚举目前只为 bIsAttacking 提供迁移期复制状态，
		// Idle/Moving 由正式 AnimBP 的 GroundSpeed 自己决定；完成多人验收后再删除该兼容状态。
		return;
	}

	UAnimSequence* Animation = nullptr;
	bool bLooping = true;
	switch (TestAnimationState)
	{
	case EShootEnemyTestAnimationState::Moving:
		Animation = TestMoveAnimation;
		break;
	case EShootEnemyTestAnimationState::Attacking:
		Animation = TestAttackAnimation;
		bLooping = false;
		break;
	case EShootEnemyTestAnimationState::Idle:
	default:
		Animation = TestIdleAnimation;
		break;
	}

	if (Animation && GetMesh())
	{
		// PlayAnimation 会切到 SingleNode。该路径只服务当前没有 AnimClass 的 BP_EnemyBotCharacter；
		// 正式 AnimBP 上线时应清空这三项资产并删除测试表现驱动，避免覆盖生产动画图。
		GetMesh()->PlayAnimation(Animation, bLooping);
	}
}

void AEnemyBotCharacter::MulticastPlayMeleeMontage_Implementation()
{
	if (TestAttackMontage)
	{
		PlayAnimMontage(TestAttackMontage);
	}
}

void AEnemyBotCharacter::HandleMovementUpdated(float DeltaSeconds, FVector OldLocation, FVector OldVelocity)
{
	if (HasAuthority()
		&& TestAnimationState != EShootEnemyTestAnimationState::Attacking)
	{
		const bool bMoving = GetVelocity().SizeSquared2D() > FMath::Square(5.0f);
		SetArchetypeAnimationState(bMoving
			? EShootEnemyTestAnimationState::Moving
			: EShootEnemyTestAnimationState::Idle);
	}
}

// Called to bind functionality to input
void AEnemyBotCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AEnemyBotCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, TestAnimationState);
}
