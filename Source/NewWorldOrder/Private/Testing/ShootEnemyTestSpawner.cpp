// Copyright ZhaoYiJie

#include "Testing/ShootEnemyTestSpawner.h"

#include "AI/EnemyBotCharacter.h"
#include "AI/EnemyBotController.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

AShootEnemyTestSpawner::AShootEnemyTestSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
}

void AShootEnemyTestSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	// 延迟到首帧之后再生成，确保 GameMode 已放置玩家、NavMesh 和关卡内现有敌人都已进入世界。
	GetWorldTimerManager().SetTimer(
		RefreshTimerHandle,
		this,
		&ThisClass::RefreshPopulation,
		FMath::Max(PopulationRefreshInterval, 0.1f),
		true,
		0.1f);
}

void AShootEnemyTestSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RefreshTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AShootEnemyTestSpawner::SetEnemyBehaviorEnabled(bool bEnabled)
{
	if (!HasAuthority() || bEnemyBehaviorEnabled == bEnabled)
	{
		return;
	}

	bEnemyBehaviorEnabled = bEnabled;
	for (TActorIterator<AEnemyBotCharacter> It(GetWorld()); It; ++It)
	{
		AEnemyBotCharacter* Enemy = *It;
		if (Enemy && IsManagedEnemy(*Enemy) && Enemy->GetDeathState() == EShootDeathState::NotDead)
		{
			ApplyBehaviorEnabledState(*Enemy);
		}
	}
}

void AShootEnemyTestSpawner::RefreshPopulation()
{
	if (!HasAuthority() || (!EnemyClass && EnemyArchetypes.IsEmpty()))
	{
		return;
	}

	TArray<AEnemyBotCharacter*> LivingEnemies;
	for (TActorIterator<AEnemyBotCharacter> It(GetWorld()); It; ++It)
	{
		AEnemyBotCharacter* Enemy = *It;
		if (Enemy && IsManagedEnemy(*Enemy) && Enemy->GetDeathState() == EShootDeathState::NotDead)
		{
			LivingEnemies.Add(Enemy);
		}
	}

	FillPopulation(LivingEnemies);
	for (AEnemyBotCharacter* Enemy : LivingEnemies)
	{
		if (Enemy)
		{
			ApplyBehaviorEnabledState(*Enemy);
		}
	}
}

void AShootEnemyTestSpawner::FillPopulation(const TArray<AEnemyBotCharacter*>& LivingEnemies)
{
	const int32 MissingCount = FMath::Max(0, TargetPopulation - LivingEnemies.Num());
	for (int32 Index = 0; Index < MissingCount; ++Index)
	{
		const TSubclassOf<AEnemyBotCharacter> SpawnClass = SelectEnemyClass();
		if (!SpawnClass)
		{
			break;
		}

		FTransform SpawnTransform;
		if (!FindNonOverlappingSpawnTransform(SpawnClass, SpawnTransform))
		{
			const double Now = GetWorld()->GetTimeSeconds();
			if (Now >= NextSpawnWarningTime)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("Enemy test spawner %s could not find a non-overlapping point (%d/%d)."),
					*GetNameSafe(this), Index, MissingCount);
				NextSpawnWarningTime = Now + 5.0;
			}
			break;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		AEnemyBotCharacter* SpawnedEnemy = GetWorld()->SpawnActor<AEnemyBotCharacter>(
			SpawnClass, SpawnTransform, SpawnParameters);
		if (!SpawnedEnemy)
		{
			continue;
		}

		if (!SpawnedEnemy->GetController())
		{
			SpawnedEnemy->SpawnDefaultController();
		}
		ApplyBehaviorEnabledState(*SpawnedEnemy);
	}
}

TSubclassOf<AEnemyBotCharacter> AShootEnemyTestSpawner::SelectEnemyClass() const
{
	float TotalWeight = 0.0f;
	for (const FShootEnemyTestSpawnEntry& Entry : EnemyArchetypes)
	{
		if (Entry.EnemyClass && Entry.Weight > 0.0f)
		{
			TotalWeight += Entry.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return EnemyClass;
	}

	float Selection = FMath::FRandRange(0.0f, TotalWeight);
	for (const FShootEnemyTestSpawnEntry& Entry : EnemyArchetypes)
	{
		if (!Entry.EnemyClass || Entry.Weight <= 0.0f)
		{
			continue;
		}

		Selection -= Entry.Weight;
		if (Selection <= 0.0f)
		{
			return Entry.EnemyClass;
		}
	}

	// 仅用于抵抗浮点边界；正常路径会在循环内返回。
	for (int32 Index = EnemyArchetypes.Num() - 1; Index >= 0; --Index)
	{
		if (EnemyArchetypes[Index].EnemyClass && EnemyArchetypes[Index].Weight > 0.0f)
		{
			return EnemyArchetypes[Index].EnemyClass;
		}
	}
	return EnemyClass;
}

bool AShootEnemyTestSpawner::IsManagedEnemy(const AEnemyBotCharacter& Enemy) const
{
	for (const FShootEnemyTestSpawnEntry& Entry : EnemyArchetypes)
	{
		if (Entry.EnemyClass && Enemy.IsA(Entry.EnemyClass))
		{
			return true;
		}
	}
	return EnemyClass && Enemy.IsA(EnemyClass);
}

bool AShootEnemyTestSpawner::FindNonOverlappingSpawnTransform(
	TSubclassOf<AEnemyBotCharacter> SpawnClass,
	FTransform& OutTransform) const
{
	const AEnemyBotCharacter* EnemyCDO = SpawnClass ? SpawnClass->GetDefaultObject<AEnemyBotCharacter>() : nullptr;
	const UCapsuleComponent* Capsule = EnemyCDO ? EnemyCDO->GetCapsuleComponent() : nullptr;
	const float CapsuleRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 42.0f;
	const float CapsuleHalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 96.0f;
	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());

	for (int32 Attempt = 0; Attempt < MaxSpawnAttemptsPerEnemy; ++Attempt)
	{
		FVector GroundLocation = FVector::ZeroVector;
		FNavLocation NavLocation;
		if (NavigationSystem
			&& NavigationSystem->GetRandomReachablePointInRadius(GetActorLocation(), SpawnRadius, NavLocation))
		{
			GroundLocation = NavLocation.Location;
		}
		else
		{
			// TestMap 是平面靶场；Recast 尚未生成或临时失效时，用地面射线保证测试夹具仍可工作。
			// 正式 PVE 地图必须提供 NavMesh，不能把这条回退当成生产刷怪方案。
			const FVector2D RandomOffset = FMath::RandPointInCircle(SpawnRadius);
			const FVector TraceStart = GetActorLocation() + FVector(RandomOffset.X, RandomOffset.Y, 2000.0f);
			const FVector TraceEnd = TraceStart - FVector(0.0f, 0.0f, 4000.0f);
			FHitResult GroundHit;
			FCollisionQueryParams GroundQueryParams(SCENE_QUERY_STAT(EnemyTestGroundTrace), false, this);
			if (!GetWorld()->LineTraceSingleByChannel(
				GroundHit, TraceStart, TraceEnd, ECC_Visibility, GroundQueryParams))
			{
				continue;
			}
			GroundLocation = GroundHit.ImpactPoint;
		}

		const FVector Candidate = GroundLocation + FVector(0.0f, 0.0f, CapsuleHalfHeight);
		bool bNearPlayerStart = false;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (FVector::DistSquared2D(Candidate, It->GetActorLocation()) < FMath::Square(MinimumSpawnSeparation * 3.0f))
			{
				bNearPlayerStart = true;
				break;
			}
		}
		if (bNearPlayerStart)
		{
			continue;
		}

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EnemyTestSpawnOverlap), false, this);
		const FCollisionShape SpawnShape = FCollisionShape::MakeCapsule(
			CapsuleRadius + MinimumSpawnSeparation * 0.5f,
			CapsuleHalfHeight);
		if (GetWorld()->OverlapAnyTestByObjectType(
			Candidate,
			FQuat::Identity,
			FCollisionObjectQueryParams(ECC_Pawn),
			SpawnShape,
			QueryParams))
		{
			continue;
		}

		OutTransform = FTransform(FRotator(0.0f, FMath::FRandRange(-180.0f, 180.0f), 0.0f), Candidate);
		return true;
	}

	return false;
}

void AShootEnemyTestSpawner::ApplyBehaviorEnabledState(AEnemyBotCharacter& Enemy) const
{
	AEnemyBotController* Controller = Cast<AEnemyBotController>(Enemy.GetController());
	if (!Controller)
	{
		return;
	}

	if (UBrainComponent* Brain = Controller->GetBrainComponent())
	{
		if (bEnemyBehaviorEnabled)
		{
			Brain->ResumeLogic(TEXT("Enemy behavior enabled by test map population control"));
		}
		else
		{
			Brain->PauseLogic(TEXT("Enemy behavior disabled by test map population control"));
		}
	}

	if (!bEnemyBehaviorEnabled)
	{
		Controller->StopMovement();
		if (UCharacterMovementComponent* Movement = Enemy.GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
		Enemy.SetArchetypeAnimationState(EShootEnemyTestAnimationState::Idle);
	}
}
