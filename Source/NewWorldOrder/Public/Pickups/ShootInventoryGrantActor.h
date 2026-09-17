// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/ShootInteractionTypes.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "ShootInventoryGrantActor.generated.h"

class UGameplayAbility;
class UUserWidget;
class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UShootInventoryItemDefinition;
class UShootInventoryManagerComponent;
class UShootWardrobeCatalogDataAsset;
struct FGameplayEventData;

/** HomeMap 调试入口要执行的库存操作；只影响正式衣柜目录中的 Persistent 服装。 */
UENUM(BlueprintType)
enum class EShootInventoryGrantDebugOperation : uint8
{
	GrantConfiguredContents,
	ClearWardrobeOwnership
};

USTRUCT(BlueprintType)
struct FShootInventoryGrantItemEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant")
	TSubclassOf<UShootInventoryItemDefinition> ItemDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant", meta=(ClampMin="1"))
	int32 StackCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant")
	EShootItemLifetime Lifetime = EShootItemLifetime::Persistent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant")
	bool bAutoAssignToQuickbarIfEquippable = true;
};

USTRUCT(BlueprintType)
struct FShootInventoryGrantResourceEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant")
	TSubclassOf<UShootInventoryItemDefinition> ResourceItemDef;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant", meta=(ClampMin="1"))
	int32 Amount = 1;
};

/**
 * 调试授予 Actor。
 *
 * 用途：
 * - 放在 HomeMap 或测试地图里，玩家交互后直接获得服装、武器、材料等库存内容。
 * - 服装走 InventoryManager Persistent 物品链路，衣柜 UI 再从 InventoryManager 读取。
 * - 武器可按 RuntimeOnly/Persistent 配置，并且只有带 EquippableItem Fragment 时才自动进 QuickBar。
 * - ClearWardrobeOwnership 只用于开发期切换“全部已获取 / 全部未获取”，不会清空武器、资源或 RuntimeOnly 物品。
 */
UCLASS()
class NEWWORLDORDER_API AShootInventoryGrantActor : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AShootInventoryGrantActor();

	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag, FGameplayEventData& InOutEventData) override;

	UFUNCTION(BlueprintCallable, Category="Grant")
	bool ProcessGrantFromAbility(APawn* ReceivingPawn);

	UFUNCTION(BlueprintCallable, Category="Grant")
	void ConfigureForSpawner(const TArray<FShootInventoryGrantItemEntry>& InItemGrants,
	                         EShootInteractionTriggerMode InTriggerMode,
	                         EShootInteractionUserFilter InUserFilter,
	                         bool bInDestroyOnGrant,
	                         FText InInteractionText,
	                         FText InInteractionSubText);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                    const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void ProcessCurrentOverlaps();
	void SetLocalPromptVisibleForPawn(APawn* Pawn, bool bVisible);
	bool GrantToPawn(APawn* ReceivingPawn);
	bool GrantInventoryItems(APawn* ReceivingPawn);
	bool GrantResources(APawn* ReceivingPawn);
	bool ClearWardrobeOwnership(APawn* ReceivingPawn);
	bool CanBeTriggeredBy(const APawn* Pawn) const;
	bool IsPlayerActor(const APawn* Pawn) const;
	UShootInventoryManagerComponent* ResolveInventoryManager(APawn* ReceivingPawn) const;
	bool HasPersistentItemGrantConfigured() const;
	void PersistInventoryToSave(APawn* ReceivingPawn) const;
	void BuildEffectiveItemGrants(TArray<FShootInventoryGrantItemEntry>& OutEntries) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grant", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grant", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> VisualComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grant|Interaction", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWidgetComponent> InteractionPromptComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant", meta=(AllowPrivateAccess="true"))
	TArray<FShootInventoryGrantItemEntry> ItemGrants;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant", meta=(AllowPrivateAccess="true"))
	TArray<FShootInventoryGrantResourceEntry> ResourceGrants;

	/** ItemGrants 为空时从正式衣柜目录发放全部服装，常用于测试“一次领齐”。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootWardrobeCatalogDataAsset> WardrobeCatalogAsset;

	/** Grant 读取 ItemGrants/目录；Clear 仅移除目录中列出的服装并立即覆盖存档。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant|Debug", meta=(AllowPrivateAccess="true"))
	EShootInventoryGrantDebugOperation DebugOperation = EShootInventoryGrantDebugOperation::GrantConfiguredContents;

	/** 是否启用这个调试发放入口。正式地图可直接关闭，或移到专用测试地图。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant|Debug", meta=(AllowPrivateAccess="true"))
	bool bDebugGrantEnabled = true;

	/** 已拥有唯一物品时视为成功，避免重复测试样例服装时交互失败。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant|Debug", meta=(AllowPrivateAccess="true"))
	bool bTreatAlreadyOwnedAsSuccess = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant", meta=(AllowPrivateAccess="true"))
	bool bDestroyOnGrant = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant|Interaction", meta=(AllowPrivateAccess="true"))
	EShootInteractionTriggerMode TriggerMode = EShootInteractionTriggerMode::PressToInteract;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant|Interaction", meta=(AllowPrivateAccess="true"))
	EShootInteractionUserFilter UserFilter = EShootInteractionUserFilter::PlayerOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant|Interaction", meta=(AllowPrivateAccess="true"))
	FText InteractionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant|Interaction", meta=(AllowPrivateAccess="true"))
	FText InteractionSubText;

	/** 本地交互提示 Widget；BP_ShootInventoryGrantActor 使用 W_Icon_Interact，内部 CommonActionWidget 绑定 IA_Interact。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant|Interaction", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UUserWidget> InteractionPromptWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Grant|Interaction", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;
};
