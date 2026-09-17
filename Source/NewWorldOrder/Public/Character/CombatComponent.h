// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/SavedInventoryTypes.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Inventory/ShootQuickbarSlotTypes.h"
#include "Weapons/ShootWeaponInstance.h"
#include "CombatComponent.generated.h"

class UShootInventoryItemInstance;
class UShootEquipmentInstance;
class UShootInventoryManagerComponent;
class UShootEquipmentManagerComponent;
class AShootWeaponPickupActor;
class UShootInventoryItemDefinition;
class UShootWeaponInstance;

USTRUCT(BlueprintType)
struct FShootQuickbarSlot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FGuid ItemInstanceId;

	UPROPERTY(BlueprintReadOnly)
	EShootItemLifetime Lifetime = EShootItemLifetime::Persistent;

	UPROPERTY()
	TObjectPtr<AActor> WeaponActor = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<UShootInventoryItemInstance> CachedItemInstance;

	UPROPERTY(Transient)
	TWeakObjectPtr<UShootEquipmentInstance> EquipmentInstance;

	/** Lyra 架构下的武器 UObject（逻辑层），仅本地缓存，不参与复制。 */
	TWeakObjectPtr<UShootWeaponInstance> WeaponInstance;

	bool IsOccupied() const
	{
		return ItemInstanceId.IsValid() || WeaponActor != nullptr || WeaponInstance.IsValid();
	}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category="Lyra")
	void CycleActiveSlotForward();

	UFUNCTION(BlueprintCallable, Category="Lyra")
	void CycleActiveSlotBackward();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Lyra")
	void SetActiveSlotIndex(int32 NewIndex);

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TArray<AActor*> GetSlots() const
	{
		TArray<AActor*> Result;
		Result.Reserve(Slots.Num());
		for (const FShootQuickbarSlot& Slot : Slots)
		{
			Result.Add(Slot.WeaponActor);
		}
		return Result;
	}

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	AActor* GetActiveSlotItem() const;

	/** Phase2：获取当前槽位绑定的 UShootWeaponInstance（武器逻辑层 UObject） */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="Inventory|Quickbar")
	UShootWeaponInstance* GetActiveWeaponInstance() const;

	/**
	 * 只读查询当前已经装备的武器实例。
	 * 动画、HUD 等表现层必须使用此函数；它绝不创建 EquipmentInstance、不会写入槽位缓存，
	 * 从而避免客户端仅为读取状态而改变装备或网络权威数据。
	 */
	UFUNCTION(BlueprintPure, Category="Inventory|Quickbar")
	UShootWeaponInstance* FindActiveWeaponInstance() const;

	/** 服务器专用：丢弃当前 RuntimeOnly 武器（Persistent 槽位不允许丢弃） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Quickbar")
	bool DropActiveRuntimeWeapon();

	/**
	 * 服务器专用：QuickBar 满槽拾枪时，把当前 RuntimeOnly 武器掉落到世界并在同一槽位装备新物品。
	 * 该入口原子地完成卸装、库存删除、世界掉落和新装备，只广播最终状态，避免同帧先自动切到相邻枪再切回来。
	 */
	bool ReplaceActiveRuntimeWeapon(UShootInventoryItemInstance* ReplacementItem, int32& OutSlotIndex);

	/** AnimNotify 驱动武器显隐时调用 */
	UFUNCTION(BlueprintCallable, Category="Inventory|Quickbar")
	void HandleWeaponAnimNotify(EShootWeaponAnimAction Action);

	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetNextFreeItemSlot() const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void AddItemToSlot(int32 SlotIndex, AActor* Item, UShootInventoryItemInstance* InventoryItem = nullptr);

	/** 获取 Quickbar 当前槽位数据快照（客户端 UI 调用） */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="Inventory|Quickbar")
	void GetQuickbarSlotsData(TArray<FQuickbarSlotData>& OutSlots);

	/** 通过 Inventory Guid 指定槽位（服务器） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	void AssignSlotFromInventory(int32 SlotIndex, FGuid ItemInstanceId);

	/** 释放槽位（服务器），用于 RuntimeOnly 清理 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	void ReleaseSlot(int32 SlotIndex);

	/** 构建 Quickbar 存档数据（仅服务器调用） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|SaveGame")
	void BuildQuickbarSaveData(TArray<FSavedQuickbarSlot>& OutSlots) const;

	/** 根据存档数据恢复 Quickbar（仅服务器调用） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|SaveGame")
	void ApplyQuickbarSaveData(const TArray<FSavedQuickbarSlot>& InSlots);

	/**
	 * PVE 开局/重生专用：从 Persistent 出战配置物化 RuntimeOnly 战斗武器。
	 * InPersistentSlots 只保存账号物品 Guid；函数绝不把 Persistent 实例直接装备到本局 QuickBar。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Quickbar")
	void InitializeRuntimeLoadoutFromPersistentSlots(const TArray<FSavedQuickbarSlot>& InPersistentSlots);

	/**
	 * 只导出当前 Pawn 已绑定的 RuntimeOnly 会话槽位。
	 * 调用链：PlayerController 在 UnPossess 前保存会话状态，随后新 Pawn 会用这些 Guid 重新绑定装备；
	 * Persistent 出战配置仍只属于 PlayerState 快照，绝不能通过本函数进入 Controller。
	 */
	void BuildRuntimeQuickbarSessionData(TArray<FSavedQuickbarSlot>& OutSlots, int32& OutActiveSlotIndex) const;

	/**
	 * Pawn 切换专用：卸载 RuntimeOnly 装备并清空本 Pawn 的槽位缓存，但不删除 InventoryManager 中的临时实例。
	 * 这与 ClearRuntimeSlots 不同；后者用于死亡/回合结束/退出副本，必须销毁 RuntimeOnly 实例。
	 */
	void DetachRuntimeQuickbarSessionForPawnTransition();

	/** Controller QuickBar 在新 Pawn 已 Possess 后调用，用已保存的 RuntimeOnly Guid 重建本 Pawn 的装备引用。 */
	void RestoreRuntimeQuickbarSessionData(const TArray<FSavedQuickbarSlot>& InSlots, int32 InActiveSlotIndex);

	/** Controller QuickBar 的副本内交换入口；只允许交换两个 RuntimeOnly 槽位，Persistent 配置不能进入该路径。 */
	bool SwapRuntimeQuickbarSlots(int32 SlotIndexA, int32 SlotIndexB);

	/** 清理 RuntimeOnly 槽位（副本结束时调用） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory")
	void ClearRuntimeSlots();

	/**
	 * Hub Loadout 使用的 QuickBar 操作接口（服务器）。UI 通过这些接口修改槽位，
	 * 再调用 PlayerState::CommitCurrentGenderLoadoutToSave() 刷新快照并写入存档。
	 */
	UFUNCTION(BlueprintCallable, Category="Combat|QuickBar", BlueprintAuthorityOnly)
	void Server_SetQuickbarSlot(int32 SlotIndex, UShootInventoryItemInstance* ItemInstance);

	UFUNCTION(BlueprintCallable, Category="Combat|QuickBar", BlueprintAuthorityOnly)
	void Server_ClearQuickbarSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category="Combat|QuickBar", BlueprintAuthorityOnly)
	void Server_SwapQuickbarSlots(int32 SlotIndexA, int32 SlotIndexB);

	/** 服务器：为当前武器直接补给弹药（TagStack），返回是否成功 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Ammo")
	bool GrantAmmoToActiveWeapon(int32 MagazineDelta, int32 ReserveDelta);


	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	AActor* RemoveItemFromSlot(int32 SlotIndex);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY(ReplicatedUsing=OnRep_Slots)
	TArray<FShootQuickbarSlot> Slots;

	UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = -1;

	UPROPERTY(EditDefaultsOnly, Category="Inventory|Pickup")
	TSubclassOf<AShootWeaponPickupActor> WeaponPickupActorClass;

	/** Quickbar 槽位类型覆盖（为空时使用默认规则） */
	UPROPERTY(EditDefaultsOnly, Category="Inventory|Quickbar")
	TArray<EShootQuickbarSlotType> SlotTypeOverrides;

	UPROPERTY()
	bool bDisableRuntimeDropSpawn = false;

	void UnequipItemInSlot();
	void EquipItemInSlot();
	AActor* RemoveItemFromSlotInternal(int32 SlotIndex, bool bAutoSelectNext, bool bBroadcastState);

	UPROPERTY()
	int32 NumSlots = 3;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex();

	UShootInventoryItemInstance* ResolveInventoryItem(FShootQuickbarSlot& Slot) const;
	UShootInventoryManagerComponent* GetInventoryManagerForOwner() const;
	UShootEquipmentManagerComponent* GetEquipmentManager() const;
	UShootEquipmentInstance* ResolveEquipmentInstance(FShootQuickbarSlot& Slot) const;
	UShootWeaponInstance* ResolveWeaponInstance(FShootQuickbarSlot& Slot) const;
	UShootWeaponInstance* GetOrCreateWeaponInstanceFromInventory(FShootQuickbarSlot& Slot) const;
	void RefreshSlotActorFromEquipment(FShootQuickbarSlot& Slot) const;
	void HandleRuntimeWeaponDropVisual(TSubclassOf<UShootInventoryItemDefinition> ItemDef,
		const FShootGameplayTagStackContainer& StatTags,
		EShootItemLifetime Lifetime) const;
	void PopulateQuickbarSlotData(FShootQuickbarSlot& Slot, FQuickbarSlotData& OutData) const;
	EShootQuickbarSlotType GetSlotTypeForIndex(int32 SlotIndex) const;
	bool IsItemAllowedInSlot(int32 SlotIndex, UShootInventoryItemInstance* ItemInstance) const;

	/** 槽位只绑定自己当前引用的 ItemInstance，避免不相关仓库物品刷新玩家 HUD。 */
	void RefreshItemStatTagBindings();
	void ClearItemStatTagBindings();
	void HandleItemStatTagsChanged(UShootInventoryItemInstance* ChangedItem);
	void BroadcastAmmoChangedForItem(UShootInventoryItemInstance* ChangedItem);

	TArray<TWeakObjectPtr<UShootInventoryItemInstance>> BoundStatTagItems;

	/**
	 * 客户端可能先收到 Pawn 的 QuickBar 槽位 Guid，稍后才收到 PlayerState InventoryManager
	 * 复制出的 ItemInstance 子对象。仅在这个短暂窗口内由 Tick 重试解析；全部依赖就绪后
	 * 立即重发完整槽位快照并关闭重试，避免 UI 永久保留首次的 0/0 弹药占位值。
	 */
	bool bPendingReplicatedSlotResolution = false;
};
