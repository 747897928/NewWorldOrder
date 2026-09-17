// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/ShootInteractionTypes.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "ShootInventoryGrantSpawner.generated.h"

class AShootInventoryGrantActor;
class USceneComponent;
class UShootInventoryItemDefinition;
class UShootWardrobeCatalogDataAsset;

/**
 * 开发期库存发放生成器。
 *
 * 用途：
 * - 放在 HomeMap 里定时生成服装领取物，方便从玩家视角验证“拾取 -> 库存 -> 衣柜 -> 装备 -> 存档”。
 * - 每次刷新前会销毁上一件未领取物，避免测试地图里堆出大量 Actor。
 * - 具体授予仍交给 AShootInventoryGrantActor，生成器本身不直接写库存。
 */
UCLASS()
class NEWWORLDORDER_API AShootInventoryGrantSpawner : public AActor
{
	GENERATED_BODY()

public:
	AShootInventoryGrantSpawner();

	UFUNCTION(BlueprintCallable, Category="Grant Spawner")
	void ForceRespawn();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void SpawnGrantActor();

	UFUNCTION()
	void HandleSpawnedActorDestroyed(AActor* DestroyedActor);

	void DestroyExistingGrantActor();
	void BuildEffectiveItemPool(TArray<TSubclassOf<UShootInventoryItemDefinition>>& OutPool) const;
	TSubclassOf<UShootInventoryItemDefinition> PickNextItemDefinition();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true"))
	TSubclassOf<AShootInventoryGrantActor> GrantActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true"))
	TArray<TSubclassOf<UShootInventoryItemDefinition>> ItemDefinitionPool;

	/** 正式衣柜目录。ItemDefinitionPool 为空时，生成器优先从这里取候选服装。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootWardrobeCatalogDataAsset> WardrobeCatalogAsset;

	/** 是否启用这个开发生成器。正式地图可关闭，或把生成器迁到专用测试地图。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true"))
	bool bDebugSpawnerEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true", ClampMin="5.0"))
	float SpawnIntervalSeconds = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true"))
	bool bSpawnOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true"))
	bool bRandomizeItem = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true"))
	bool bDestroyOldSpawnOnRespawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner", meta=(AllowPrivateAccess="true"))
	FVector SpawnOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner|Grant", meta=(AllowPrivateAccess="true"))
	EShootInteractionTriggerMode SpawnedTriggerMode = EShootInteractionTriggerMode::PressToInteract;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner|Grant", meta=(AllowPrivateAccess="true"))
	EShootInteractionUserFilter SpawnedUserFilter = EShootInteractionUserFilter::PlayerOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner|Grant", meta=(AllowPrivateAccess="true"))
	FText InteractionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant Spawner|Grant", meta=(AllowPrivateAccess="true"))
	FText InteractionSubText;

	UPROPERTY()
	TObjectPtr<AShootInventoryGrantActor> SpawnedGrantActor;

	FTimerHandle SpawnTimerHandle;
	int32 SequentialItemIndex = 0;
};
