// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/ShootInteractionTypes.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Inventory/SavedInventoryTypes.h"
#include "ShootResourcePickup.generated.h"

class UGameplayAbility;
class USphereComponent;
class UStaticMeshComponent;
class UShootInventoryItemDefinition;
class UShootInventoryManagerComponent;
struct FGameplayEventData;

/** 
 * 资源拾取 Actor
 *
 * 用途：
 * - 副本/场景里放置的材料、金币、徽章等掉落
 * - 服务器检测 Pawn Overlap 后调用 ResourceInventoryComponent（数量型资源仓库）增加资源
 *
 * 架构说明：
 * - ResourceInventoryComponent = 数量型资源仓库（材料 / 货币 / 徽章 / 设计图）
 * - UShootInventoryManagerComponent = 有身份的物品 / 武器仓库
 *   QuickBar / Equipment 只与 InventoryManager 交互，Hub 配置与副本内换枪走同一条线
 *
 * 新版交互：
 * - TriggerMode = AutoOverlap → 继续沿用旧逻辑（进入碰撞直接拾取，可配置作用对象 Player/AI）
 * - TriggerMode = PressToInteract → 实现 IInteractableTarget，交由 UShootGA_Interact 扫描并在按键时触发
 */
UCLASS()
class AShootResourcePickup : public AActor, public IInteractableTarget
{
	GENERATED_BODY()

public:
	AShootResourcePickup();

protected:
	virtual void BeginPlay() override;
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag, FGameplayEventData& InOutEventData) override;

private:
	/** 触发体，用于检测玩家 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup", meta=(AllowPrivateAccess="true"))
	USphereComponent* CollisionComponent;

	/** 简易可视化组件，可在蓝图里替换为网格/粒子 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup", meta=(AllowPrivateAccess="true"))
	UStaticMeshComponent* VisualComponent;

	/** 拾取后给予的资源类型（例如 ID_Material_MilitaryAlloy） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UShootInventoryItemDefinition> ResourceItemDef;

	/** 默认给予的数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup", meta=(AllowPrivateAccess="true", ClampMin="1"))
	int32 Amount = 1;

	/** 是否拾取成功后销毁 Actor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup", meta=(AllowPrivateAccess="true"))
	bool bDestroyOnPickup = true;

	/** 是否将数量视为弹药补给，直接写入当前武器 StatTags（服务器），优先级高于 AddResource */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Ammo", meta=(AllowPrivateAccess="true"))
	bool bGrantAmmoToActiveWeapon = false;

	/** 补给弹夹弹药数（仅 bGrantAmmoToActiveWeapon 时生效） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Ammo", meta=(AllowPrivateAccess="true", ClampMin="0"))
	int32 MagazineAmmoDelta = 0;

	/** 补给备弹数量（仅 bGrantAmmoToActiveWeapon 时生效） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Ammo", meta=(AllowPrivateAccess="true", ClampMin="0"))
	int32 ReserveAmmoDelta = 0;

	/** 交互触发模式（自动拾取 / 按键 / AI 脚本） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction", meta=(AllowPrivateAccess="true"))
	EShootInteractionTriggerMode TriggerMode = EShootInteractionTriggerMode::AutoOverlap;

	/** 允许哪些角色与此拾取互动 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction", meta=(AllowPrivateAccess="true"))
	EShootInteractionUserFilter UserFilter = EShootInteractionUserFilter::PlayerOnly;

	/** 交互提示文本（留空则读取物品定义 DisplayName） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction", meta=(AllowPrivateAccess="true"))
	FText InteractionText;

	/** 可选的副标题（例如数量） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction", meta=(AllowPrivateAccess="true"))
	FText InteractionSubText;

	/** 按键交互时授予的能力（默认为 UShootGA_Interaction_Collect，可用蓝图子类覆盖） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	/** 根据阵营应用额外 GE（玩家扣血 / AI 回血等） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Interaction", meta=(AllowPrivateAccess="true"))
	TArray<FShootFactionEffectEntry> FactionEffects;

	/** 是否直接通过 InventoryManager 发放物品（例如武器），否则走 ResourceInventory 流程 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Inventory", meta=(AllowPrivateAccess="true"))
	bool bGrantInventoryItem = false;

	/** 发放物品时的生命周期（Persistent=账号资产，RuntimeOnly=副本临时） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pickup|Inventory", meta=(AllowPrivateAccess="true", EditCondition="bGrantInventoryItem"))
	EShootItemLifetime PickupItemLifetime = EShootItemLifetime::RuntimeOnly;

	/** 服务器处理拾取逻辑，返回是否成功 */
	bool HandlePickup(APawn* PickingPawn);
	bool GiveInventoryItem(APawn* PickingPawn);
	UShootInventoryManagerComponent* ResolveInventoryManager(APawn* PickingPawn) const;

	bool CanBeTriggeredBy(const APawn* Pawn) const;
	bool IsPlayerActor(const APawn* Pawn) const;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                    const FHitResult& SweepResult);

public:
	TSubclassOf<UShootInventoryItemDefinition> GetResourceItemDef() const { return ResourceItemDef; }
	int32 GetAmount() const { return Amount; }
	EShootInteractionTriggerMode GetTriggerMode() const { return TriggerMode; }
	const TArray<FShootFactionEffectEntry>& GetFactionEffects() const { return FactionEffects; }
	bool ProcessPickupFromAbility(APawn* PickingPawn);
};
