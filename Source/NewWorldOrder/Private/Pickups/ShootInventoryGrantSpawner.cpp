// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Pickups/ShootInventoryGrantSpawner.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Pickups/ShootInventoryGrantActor.h"
#include "TimerManager.h"
#include "UI/Wardrobe/ShootWardrobeCatalogDataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootInventoryGrantSpawner)

AShootInventoryGrantSpawner::AShootInventoryGrantSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	GrantActorClass = AShootInventoryGrantActor::StaticClass();
	InteractionText = NSLOCTEXT("InventoryGrantSpawner", "DefaultInteractionText", "Claim Random Outfit");
	InteractionSubText = NSLOCTEXT("InventoryGrantSpawner", "DefaultInteractionSubText", "Random outfit reward");

#if UE_BUILD_SHIPPING
	bDebugSpawnerEnabled = false;
#endif
}

void AShootInventoryGrantSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !bDebugSpawnerEnabled)
	{
		return;
	}

	if (bSpawnOnBeginPlay)
	{
		ForceRespawn();
	}

	if (SpawnIntervalSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&ThisClass::SpawnGrantActor,
			SpawnIntervalSeconds,
			true);
	}
}

void AShootInventoryGrantSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	DestroyExistingGrantActor();

	Super::EndPlay(EndPlayReason);
}

void AShootInventoryGrantSpawner::ForceRespawn()
{
	SpawnGrantActor();
}

void AShootInventoryGrantSpawner::SpawnGrantActor()
{
	if (!HasAuthority() || !bDebugSpawnerEnabled || !GrantActorClass)
	{
		return;
	}

	if (bDestroyOldSpawnOnRespawn)
	{
		DestroyExistingGrantActor();
	}
	else if (SpawnedGrantActor)
	{
		return;
	}

	TSubclassOf<UShootInventoryItemDefinition> ItemDefinition = PickNextItemDefinition();
	if (!ItemDefinition)
	{
		return;
	}

	TArray<FShootInventoryGrantItemEntry> ItemGrants;
	FShootInventoryGrantItemEntry Entry;
	Entry.ItemDefinition = ItemDefinition;
	Entry.StackCount = 1;
	Entry.Lifetime = EShootItemLifetime::Persistent;
	Entry.bAutoAssignToQuickbarIfEquippable = false;
	ItemGrants.Add(Entry);

	const FTransform SpawnTransform(GetActorRotation(), GetActorLocation() + SpawnOffset, FVector::OneVector);
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AShootInventoryGrantActor* NewGrantActor = World->SpawnActorDeferred<AShootInventoryGrantActor>(
		GrantActorClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!NewGrantActor)
	{
		return;
	}

	// 子 Actor 只拿到本轮要发的单件服装，拾取后写 Persistent InventoryManager。
	NewGrantActor->ConfigureForSpawner(
		ItemGrants,
		SpawnedTriggerMode,
		SpawnedUserFilter,
		/*bInDestroyOnGrant*/ true,
		InteractionText,
		InteractionSubText);

	NewGrantActor->FinishSpawning(SpawnTransform);
	NewGrantActor->Tags.AddUnique(FName(TEXT("WardrobeSpawnerGrant")));
	NewGrantActor->OnDestroyed.AddDynamic(this, &ThisClass::HandleSpawnedActorDestroyed);
	SpawnedGrantActor = NewGrantActor;
}

void AShootInventoryGrantSpawner::HandleSpawnedActorDestroyed(AActor* DestroyedActor)
{
	if (DestroyedActor == SpawnedGrantActor)
	{
		SpawnedGrantActor = nullptr;
	}
}

void AShootInventoryGrantSpawner::DestroyExistingGrantActor()
{
	if (!SpawnedGrantActor)
	{
		return;
	}

	AShootInventoryGrantActor* ActorToDestroy = SpawnedGrantActor;
	SpawnedGrantActor = nullptr;
	ActorToDestroy->OnDestroyed.RemoveDynamic(this, &ThisClass::HandleSpawnedActorDestroyed);
	ActorToDestroy->Destroy();
}

void AShootInventoryGrantSpawner::BuildEffectiveItemPool(
	TArray<TSubclassOf<UShootInventoryItemDefinition>>& OutPool) const
{
	OutPool = ItemDefinitionPool;
	if (OutPool.Num() > 0)
	{
		return;
	}

	if (WardrobeCatalogAsset)
	{
		// 生成器跟衣柜 UI 共用同一份目录资产，避免测试领取池和衣柜图鉴分叉。
		WardrobeCatalogAsset->AppendWardrobeItemsTo(OutPool);
	}

}

TSubclassOf<UShootInventoryItemDefinition> AShootInventoryGrantSpawner::PickNextItemDefinition()
{
	TArray<TSubclassOf<UShootInventoryItemDefinition>> Pool;
	BuildEffectiveItemPool(Pool);
	Pool.RemoveAll([](const TSubclassOf<UShootInventoryItemDefinition>& ItemDefinition)
	{
		return !ItemDefinition;
	});

	if (Pool.Num() == 0)
	{
		return nullptr;
	}

	if (bRandomizeItem)
	{
		return Pool[FMath::RandRange(0, Pool.Num() - 1)];
	}

	const int32 PickedIndex = SequentialItemIndex % Pool.Num();
	SequentialItemIndex = (SequentialItemIndex + 1) % Pool.Num();
	return Pool[PickedIndex];
}
