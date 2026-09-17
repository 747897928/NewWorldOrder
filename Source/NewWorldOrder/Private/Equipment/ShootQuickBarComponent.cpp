// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Equipment/ShootQuickBarComponent.h"

#include "Character/CombatComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Weapons/ShootRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootQuickBarComponent)

UShootQuickBarComponent::UShootQuickBarComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UShootQuickBarComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Lyra 由 Controller 上的 WeaponStateComponent 驱动当前武器 Tick；本项目明确不引入该组件。
	// QuickBar 本来就是每个 Controller 的战斗会话边界，因此只驱动当前武器的散布/玩家姿态倍率，
	// 不做命中确认或额外网络校验，伤害权威仍由现有 GAS CommitAbility / ApplyCost 链保证。
	if (UShootRangedWeaponInstance* RangedWeapon = Cast<UShootRangedWeaponInstance>(FindActiveWeaponInstance()))
	{
		RangedWeapon->Tick(DeltaTime);
	}
}

UCombatComponent* UShootQuickBarComponent::GetPawnCombatComponent() const
{
	const AController* OwnerController = GetController<AController>();
	if (!OwnerController)
	{
		return nullptr;
	}

	APawn* ControlledPawn = OwnerController->GetPawn();
	return ControlledPawn ? ControlledPawn->FindComponentByClass<UCombatComponent>() : nullptr;
}

UShootWeaponInstance* UShootQuickBarComponent::FindActiveWeaponInstance() const
{
	if (const UCombatComponent* CombatComponent = GetPawnCombatComponent())
	{
		// 当前阶段只通过无副作用接口读取，不能在 HUD/动画查询期间创建装备。
		return CombatComponent->FindActiveWeaponInstance();
	}

	return nullptr;
}

int32 UShootQuickBarComponent::GetActiveSlotIndex() const
{
	if (const UCombatComponent* CombatComponent = GetPawnCombatComponent())
	{
		return CombatComponent->GetActiveSlotIndex();
	}

	return INDEX_NONE;
}

void UShootQuickBarComponent::GetQuickbarSlotsData(TArray<FQuickbarSlotData>& OutSlots) const
{
	OutSlots.Reset();

	if (UCombatComponent* CombatComponent = GetPawnCombatComponent())
	{
		CombatComponent->GetQuickbarSlotsData(OutSlots);
	}
}

void UShootQuickBarComponent::CaptureRuntimeSessionFromPawn(APawn* OldPawn)
{
	AController* OwnerController = GetController<AController>();
	if (!OwnerController || !OwnerController->HasAuthority() || !OldPawn)
	{
		return;
	}

	UCombatComponent* CombatComponent = OldPawn->FindComponentByClass<UCombatComponent>();
	if (!CombatComponent)
	{
		return;
	}

	// Controller 保存本局临时武器 Guid，旧 Pawn 仅卸载表现；不能调用 ClearRuntimeSlots，否则重生会丢失局内拾取。
	CombatComponent->BuildRuntimeQuickbarSessionData(RuntimeSessionSlots, RuntimeSessionActiveSlotIndex);
	CombatComponent->DetachRuntimeQuickbarSessionForPawnTransition();
}

void UShootQuickBarComponent::RestoreRuntimeSessionToPawn(APawn* NewPawn)
{
	AController* OwnerController = GetController<AController>();
	if (!OwnerController || !OwnerController->HasAuthority() || !NewPawn || RuntimeSessionSlots.IsEmpty())
	{
		return;
	}

	if (UCombatComponent* CombatComponent = NewPawn->FindComponentByClass<UCombatComponent>())
	{
		// 新 Pawn 的 EquipmentManager 属于表现层；会话槽位仍由 Controller 保存的 RuntimeOnly Guid 恢复。
		CombatComponent->RestoreRuntimeQuickbarSessionData(RuntimeSessionSlots, RuntimeSessionActiveSlotIndex);
	}
}

bool UShootQuickBarComponent::AssignRuntimeItemToAvailableSlot(UShootInventoryItemInstance* ItemInstance, int32& OutSlotIndex)
{
	OutSlotIndex = INDEX_NONE;

	AController* OwnerController = GetController<AController>();
	if (!OwnerController || !OwnerController->HasAuthority() || !ItemInstance ||
		ItemInstance->GetItemLifetime() != EShootItemLifetime::RuntimeOnly)
	{
		return false;
	}

	UCombatComponent* CombatComponent = GetPawnCombatComponent();
	if (!CombatComponent)
	{
		return false;
	}

	const int32 FreeSlotIndex = CombatComponent->GetNextFreeItemSlot();
	if (FreeSlotIndex == INDEX_NONE)
	{
		// Lyra QuickBar 本身只提供空槽 API；本项目按副本需求在 Controller 会话边界适配满槽拾枪行为。
		// 由 Pawn CombatComponent 原子替换当前 RuntimeOnly 枪，Persistent 账号配置不会进入此路径。
		const bool bReplaced = CombatComponent->ReplaceActiveRuntimeWeapon(ItemInstance, OutSlotIndex);
		if (bReplaced)
		{
			RefreshRuntimeSessionFromCurrentPawn();
		}
		return bReplaced;
	}

	// RuntimeOnly 的身份仍由 PlayerState InventoryManager 管理；Controller 只决定本局会话槽位。
	CombatComponent->AssignSlotFromInventory(FreeSlotIndex, ItemInstance->GetItemInstanceId());
	RefreshRuntimeSessionFromCurrentPawn();
	OutSlotIndex = FreeSlotIndex;
	return true;
}

bool UShootQuickBarComponent::SetActiveRuntimeSlot(int32 SlotIndex)
{
	AController* OwnerController = GetController<AController>();
	UCombatComponent* CombatComponent = GetPawnCombatComponent();
	if (!OwnerController || !OwnerController->HasAuthority() || !CombatComponent || !IsRuntimeSessionSlot(SlotIndex))
	{
		return false;
	}

	CombatComponent->SetActiveSlotIndex(SlotIndex);
	RefreshRuntimeSessionFromCurrentPawn();
	return true;
}

bool UShootQuickBarComponent::CycleRuntimeSlot(bool bForward)
{
	AController* OwnerController = GetController<AController>();
	UCombatComponent* CombatComponent = GetPawnCombatComponent();
	if (!OwnerController || !OwnerController->HasAuthority() || !CombatComponent || RuntimeSessionSlots.IsEmpty())
	{
		return false;
	}

	TArray<int32> RuntimeSlotIndices;
	RuntimeSlotIndices.Reserve(RuntimeSessionSlots.Num());
	for (const FSavedQuickbarSlot& Slot : RuntimeSessionSlots)
	{
		if (Slot.ItemInstanceId.IsValid())
		{
			RuntimeSlotIndices.Add(Slot.SlotIndex);
		}
	}
	RuntimeSlotIndices.Sort();
	if (RuntimeSlotIndices.IsEmpty())
	{
		return false;
	}

	int32 CurrentIndex = RuntimeSlotIndices.IndexOfByKey(CombatComponent->GetActiveSlotIndex());
	if (CurrentIndex == INDEX_NONE)
	{
		CurrentIndex = bForward ? RuntimeSlotIndices.Num() - 1 : 0;
	}

	const int32 NextIndex = (CurrentIndex + (bForward ? 1 : RuntimeSlotIndices.Num() - 1)) % RuntimeSlotIndices.Num();
	return SetActiveRuntimeSlot(RuntimeSlotIndices[NextIndex]);
}

bool UShootQuickBarComponent::DropActiveRuntimeWeapon()
{
	AController* OwnerController = GetController<AController>();
	UCombatComponent* CombatComponent = GetPawnCombatComponent();
	if (!OwnerController || !OwnerController->HasAuthority() || !CombatComponent)
	{
		return false;
	}

	// Controller 是玩家输入和 RuntimeOnly 会话边界；Pawn CombatComponent 执行实际卸装、库存删除和掉落实体生成。
	const bool bDropped = CombatComponent->DropActiveRuntimeWeapon();
	if (bDropped)
	{
		RefreshRuntimeSessionFromCurrentPawn();
	}
	return bDropped;
}

bool UShootQuickBarComponent::SwapRuntimeSlots(int32 SlotIndexA, int32 SlotIndexB)
{
	AController* OwnerController = GetController<AController>();
	UCombatComponent* CombatComponent = GetPawnCombatComponent();
	if (!OwnerController || !OwnerController->HasAuthority() || !CombatComponent ||
		!IsRuntimeSessionSlot(SlotIndexA) || !IsRuntimeSessionSlot(SlotIndexB))
	{
		return false;
	}

	if (!CombatComponent->SwapRuntimeQuickbarSlots(SlotIndexA, SlotIndexB))
	{
		return false;
	}

	RefreshRuntimeSessionFromCurrentPawn();
	return true;
}

void UShootQuickBarComponent::ClearRuntimeSession()
{
	AController* OwnerController = GetController<AController>();
	if (!OwnerController || !OwnerController->HasAuthority())
	{
		return;
	}

	if (UCombatComponent* CombatComponent = GetPawnCombatComponent())
	{
		// CombatComponent 负责卸载当前 Pawn 的 Equipment，并让 PlayerState InventoryManager 删除 RuntimeOnly 实例。
		CombatComponent->ClearRuntimeSlots();
	}

	// Controller 只保存战斗会话引用；返回 Hub 后不能让旧 Guid 在下一次 Possess 时复活。
	RuntimeSessionSlots.Reset();
	RuntimeSessionActiveSlotIndex = INDEX_NONE;
}

void UShootQuickBarComponent::RequestSetActiveRuntimeSlot(int32 SlotIndex)
{
	if (AController* OwnerController = GetController<AController>())
	{
		if (OwnerController->HasAuthority())
		{
			SetActiveRuntimeSlot(SlotIndex);
		}
		else
		{
			ServerRequestSetActiveRuntimeSlot(SlotIndex);
		}
	}
}

void UShootQuickBarComponent::RequestCycleRuntimeSlot(bool bForward)
{
	if (AController* OwnerController = GetController<AController>())
	{
		if (OwnerController->HasAuthority())
		{
			CycleRuntimeSlot(bForward);
		}
		else
		{
			ServerRequestCycleRuntimeSlot(bForward);
		}
	}
}

void UShootQuickBarComponent::RequestSwapRuntimeSlots(int32 SlotIndexA, int32 SlotIndexB)
{
	if (AController* OwnerController = GetController<AController>())
	{
		if (OwnerController->HasAuthority())
		{
			SwapRuntimeSlots(SlotIndexA, SlotIndexB);
		}
		else
		{
			ServerRequestSwapRuntimeSlots(SlotIndexA, SlotIndexB);
		}
	}
}

void UShootQuickBarComponent::ServerRequestSetActiveRuntimeSlot_Implementation(int32 SlotIndex)
{
	SetActiveRuntimeSlot(SlotIndex);
}

void UShootQuickBarComponent::ServerRequestCycleRuntimeSlot_Implementation(bool bForward)
{
	CycleRuntimeSlot(bForward);
}

void UShootQuickBarComponent::ServerRequestSwapRuntimeSlots_Implementation(int32 SlotIndexA, int32 SlotIndexB)
{
	SwapRuntimeSlots(SlotIndexA, SlotIndexB);
}

bool UShootQuickBarComponent::IsRuntimeSessionSlot(int32 SlotIndex) const
{
	return RuntimeSessionSlots.ContainsByPredicate([SlotIndex](const FSavedQuickbarSlot& Slot)
	{
		return Slot.SlotIndex == SlotIndex && Slot.ItemInstanceId.IsValid();
	});
}

void UShootQuickBarComponent::RefreshRuntimeSessionFromCurrentPawn()
{
	if (UCombatComponent* CombatComponent = GetPawnCombatComponent())
	{
		CombatComponent->BuildRuntimeQuickbarSessionData(RuntimeSessionSlots, RuntimeSessionActiveSlotIndex);
	}
}
