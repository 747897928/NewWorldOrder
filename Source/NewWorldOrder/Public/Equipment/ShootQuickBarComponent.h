// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "Character/QuickbarMessageTypes.h"
#include "Inventory/SavedInventoryTypes.h"

#include "ShootQuickBarComponent.generated.h"

class UCombatComponent;
class UShootInventoryItemInstance;
class UShootWeaponInstance;
class APawn;

/**
 * QuickBar 的 Controller 侧迁移入口。
 *
 * 项目最终会把槽位和激活索引从 Pawn CombatComponent 迁到这里，使其与 PlayerController
 * 一起跨 Pawn 重生和角色切换存活。第一阶段只提供无副作用的表现层查询，避免在迁移期间
 * 同时维护两份槽位真相。
 */
UCLASS(BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UShootQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	/** 返回当前 Controller 所控制 Pawn 的兼容 CombatComponent。 */
	UFUNCTION(BlueprintPure, Category="Inventory|Quickbar")
	UCombatComponent* GetPawnCombatComponent() const;

	/**
	 * 给 HUD、动画桥接和后续 Notify 使用的无副作用当前武器查询。
	 * 当前阶段通过 Pawn 上的兼容 CombatComponent 查询；后续槽位迁移完成后，调用方无需改名。
	 */
	UFUNCTION(BlueprintPure, Category="Inventory|Quickbar")
	UShootWeaponInstance* FindActiveWeaponInstance() const;

	/** 当前激活槽位的只读索引。UI 只能从拥有自己的 PlayerController 查询此值。 */
	UFUNCTION(BlueprintPure, Category="Inventory|Quickbar")
	int32 GetActiveSlotIndex() const;

	/**
	 * 读取当前玩家的 QuickBar 展示快照，不创建 WeaponInstance。
	 * 迁移期仍由 Pawn CombatComponent 维护槽位真相；UI 不再直接依赖 Pawn 组件，
	 * 后续转移槽位数据时不需要改动 Widget 调用点。
	 */
	UFUNCTION(BlueprintCallable, Category="Inventory|Quickbar")
	void GetQuickbarSlotsData(TArray<FQuickbarSlotData>& OutSlots) const;

	/** 由 AShootPlayerController::OnUnPossess 在旧 Pawn 仍有效时调用。 */
	void CaptureRuntimeSessionFromPawn(APawn* OldPawn);

	/** 由 AShootPlayerController::OnPossess 在新 Pawn 已完成 ASC/外观初始化后调用。 */
	void RestoreRuntimeSessionToPawn(APawn* NewPawn);

	/**
	 * 副本拾取专用：将已加入 PlayerState InventoryManager 的 RuntimeOnly 物品放入第一个空会话槽位；
	 * 若三槽已满，则让 Pawn 原子地丢弃并替换当前 RuntimeOnly 武器。
	 * Hub Persistent 配置必须继续走 PlayerState 快照与既有配置接口，不能调用此函数。
	 */
	bool AssignRuntimeItemToAvailableSlot(UShootInventoryItemInstance* ItemInstance, int32& OutSlotIndex);

	/** 副本内切枪入口。只接受当前会话缓存中的 RuntimeOnly 槽位，不允许选中账号 Persistent 配置。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Quickbar")
	bool SetActiveRuntimeSlot(int32 SlotIndex);

	/** 副本内向前或向后循环 RuntimeOnly 武器槽。输入映射由蓝图或 GAS 调用此语义入口。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Quickbar")
	bool CycleRuntimeSlot(bool bForward);

	/** 丢弃当前 RuntimeOnly 武器并刷新 Controller 会话缓存；Persistent 账号装备不能由此入口丢弃。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Quickbar")
	bool DropActiveRuntimeWeapon();

	/** 副本内交换两个 RuntimeOnly 槽位。账号 Persistent 出战配置必须继续走 Hub 配置接口。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Quickbar")
	bool SwapRuntimeSlots(int32 SlotIndexA, int32 SlotIndexB);

	/**
	 * 副本结束或返回 Hub 时清空当前 Controller 的 RuntimeOnly 会话。
	 * 该入口同时卸载 Pawn Equipment、删除 PlayerState InventoryManager 中的临时实例并清空 Controller 缓存。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|Quickbar")
	void ClearRuntimeSession();

	/**
	 * 本地玩法输入或 UI 的 RuntimeOnly 选槽请求。客户端不直接修改 Pawn 槽位，
	 * 而是通过拥有自己的 PlayerController 组件发送 Server RPC，由服务器装备并复制结果。
	 */
	UFUNCTION(BlueprintCallable, Category="Inventory|Quickbar")
	void RequestSetActiveRuntimeSlot(int32 SlotIndex);

	/** 副本内循环切枪的网络请求；bForward 是动作语义，不承载具体键盘或手柄绑定。 */
	UFUNCTION(BlueprintCallable, Category="Inventory|Quickbar")
	void RequestCycleRuntimeSlot(bool bForward);

	/** 副本内交换两个 RuntimeOnly 槽位的网络请求。 */
	UFUNCTION(BlueprintCallable, Category="Inventory|Quickbar")
	void RequestSwapRuntimeSlots(int32 SlotIndexA, int32 SlotIndexB);

private:
	/**
	 * 只缓存副本 RuntimeOnly 槽位，跨 Pawn 重生和副本内角色切换时保持当前会话武器。
	 * Persistent 出战配置、SaveGame 与家园装备绝不能写入此数组。
	 */
	TArray<FSavedQuickbarSlot> RuntimeSessionSlots;
	int32 RuntimeSessionActiveSlotIndex = INDEX_NONE;

	bool IsRuntimeSessionSlot(int32 SlotIndex) const;
	void RefreshRuntimeSessionFromCurrentPawn();

	UFUNCTION(Server, Reliable)
	void ServerRequestSetActiveRuntimeSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerRequestCycleRuntimeSlot(bool bForward);

	UFUNCTION(Server, Reliable)
	void ServerRequestSwapRuntimeSlots(int32 SlotIndexA, int32 SlotIndexB);
};
