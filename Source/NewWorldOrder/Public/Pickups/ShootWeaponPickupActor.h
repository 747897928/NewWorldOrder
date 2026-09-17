// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/ShootInteractionTypes.h"
#include "Inventory/SavedInventoryTypes.h"
#include "Inventory/ShootGameplayTagStack.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "ShootWeaponPickupActor.generated.h"

class UGameplayAbility;
class USphereComponent;
class UStaticMeshComponent;
class UShootInventoryItemDefinition;
class UShootInventoryItemInstance;

/**
 * 武器拾取 Actor（CF/CSOL 风格临时武器）
 *
 * - 支持自动拾取与按键交互
 * - 服务器记录 ItemDefinition / 生命周期 / StatTag 快照
 * - 拾取后调用 InventoryManager -> CombatComponent 填充 QuickBar
 */
UCLASS()
class AShootWeaponPickupActor : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AShootWeaponPickupActor();

	// IInteractableTarget
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag, FGameplayEventData& InOutEventData) override;

	/** 初始化掉落武器的物品身份、生命周期和 StatTag 快照。 */
	void InitializeFromDrop(TSubclassOf<UShootInventoryItemDefinition> InDefinition, EShootItemLifetime InLifetime,
		const FShootGameplayTagStackContainer& InStatTags);

	/** 交互 GA 与自动化验收共用的服务器权威拾取入口。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Pickup")
	bool HandlePickup(APawn* PickingPawn);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, Category="Pickup")
	TObjectPtr<USphereComponent> CollisionComponent;

	/**
	 * 方案 B 的唯一拾取视觉入口：所有 BP_WeaponPickup_* 只在此父类组件配置 StaticMesh。
	 * AActor 本身没有可渲染的 Mesh；CollisionComponent 是 USphereComponent，只负责交互范围，不能承载枪模型。
	 * 不要在拾取蓝图中再添加 SkeletalMesh；武器装备后的 SkeletalMesh 属于独立的 Equipment Actor。
	 */
	UPROPERTY(VisibleAnywhere, Category="Pickup|Visual")
	TObjectPtr<UStaticMeshComponent> VisualComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Pickup")
	TSubclassOf<UShootInventoryItemDefinition> WeaponItemDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Pickup")
	EShootItemLifetime ItemLifetime = EShootItemLifetime::RuntimeOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup")
	bool bDestroyOnPickup = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction")
	EShootInteractionTriggerMode TriggerMode = EShootInteractionTriggerMode::AutoOverlap;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction")
	EShootInteractionUserFilter UserFilter = EShootInteractionUserFilter::PlayerOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction")
	FText InteractionSubText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction")
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	/** 存储 StatTag 快照（掉落时还原弹药/耐久） */
	UPROPERTY(Replicated)
	FShootGameplayTagStackContainer StoredStatTags;

	// Cache overlap delegate
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                    const FHitResult& SweepResult);

	bool AutoEquipIfPossible(APawn* PickingPawn, UShootInventoryItemInstance* NewInstance);
	bool CanBeTriggeredBy(const APawn* Pawn) const;
	bool IsPlayerActor(const APawn* Pawn) const;
};
