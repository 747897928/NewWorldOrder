// Copyright ZhaoYiJie


#include "Character/CombatComponent.h"

#include "ShootGameplayTags.h"
#include "Character/QuickbarMessageTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/ShootRangedWeaponInstance.h"
#include "Inventory/Fragments/ShootInventoryFragment_WeaponBasicConfig.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Inventory/ShootInventoryManagerComponent.h"
#include "Inventory/ShootInventoryFragment_EquippableItem.h"
#include "Inventory/Fragments/ShootInventoryFragment_QuickbarSlotRules.h"
#include "Equipment/ShootEquipmentManagerComponent.h"
#include "Equipment/ShootEquipmentInstance.h"
#include "Pickups/ShootWeaponPickupActor.h"
#include "Player/ShootPlayerState.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogCombatComponent, Log, All);

namespace CombatComponentLog
{
static const TCHAR* GetAuthorityString(const UCombatComponent* Component)
{
	if (!Component)
	{
		return TEXT("Unknown");
	}

	const AActor* OwnerActor = Component->GetOwner();
	return (OwnerActor && OwnerActor->HasAuthority()) ? TEXT("Server") : TEXT("Client");
}

static const TCHAR* GetLifetimeString(EShootItemLifetime Lifetime)
{
	return Lifetime == EShootItemLifetime::RuntimeOnly ? TEXT("RuntimeOnly") : TEXT("Persistent");
}
}

// Sets default values for this component's properties
UCombatComponent::UCombatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	// ...

	WeaponPickupActorClass = AShootWeaponPickupActor::StaticClass();
}


// Called when the game starts
void UCombatComponent::BeginPlay()
{
	if (Slots.Num() < NumSlots)
	{
		Slots.AddDefaulted(NumSlots - Slots.Num());
	}

	Super::BeginPlay();
	RefreshItemStatTagBindings();
}

void UCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearItemStatTagBindings();
	Super::EndPlay(EndPlayReason);
}

void UCombatComponent::UnequipItemInSlot()
{
	if (Slots.IsValidIndex(ActiveSlotIndex))
	{
		FShootQuickbarSlot& ActiveSlot = Slots[ActiveSlotIndex];
		if (UShootEquipmentInstance* EquipInstance = ActiveSlot.EquipmentInstance.Get())
		{
			if (UShootEquipmentManagerComponent* EquipmentManager = GetEquipmentManager())
			{
				EquipmentManager->UnequipItem(EquipInstance);
			}
		}
		ActiveSlot.EquipmentInstance.Reset();
		ActiveSlot.WeaponInstance.Reset();
		ActiveSlot.WeaponActor = nullptr;
	}

}

void UCombatComponent::EquipItemInSlot()
{
	check(Slots.IsValidIndex(ActiveSlotIndex));

	FShootQuickbarSlot& Slot = Slots[ActiveSlotIndex];

	// Phase2：QuickBar 逻辑优先依赖 WeaponInstance（UObject），Legacy Actor 仅用于表现层
	UShootWeaponInstance* WeaponInstance = GetOrCreateWeaponInstanceFromInventory(Slot);

	RefreshSlotActorFromEquipment(Slot);

	// WeaponInstance 存在时视为装备成功，不依赖 Legacy Actor
	if (!WeaponInstance && Slot.WeaponActor == nullptr)
	{
		UE_LOG(LogCombatComponent, Warning,
			TEXT("[%s] EquipItemInSlot failed to resolve WeaponInstance for slot %d (ItemInstanceId=%s)."),
			CombatComponentLog::GetAuthorityString(this),
			ActiveSlotIndex,
			Slot.ItemInstanceId.IsValid() ? *Slot.ItemInstanceId.ToString() : TEXT("Invalid"));
	}

	// WeaponInstance 是逻辑权威；槽位中的 Actor 只保留当前 Pawn 的网格、Socket、动画与开火表现。
}

void UCombatComponent::OnRep_Slots()
{
	const FShootGameplayTags& ShootGameplayTags = FShootGameplayTags::Get();
	bPendingReplicatedSlotResolution = false;

	FQuickbarSlotsChangedMessage Msg;
	Msg.Owner = GetOwner();
	Msg.Slots.Reset();
	Msg.Slots.Reserve(Slots.Num());

	for (FShootQuickbarSlot& Slot : Slots)
	{
		UShootInventoryItemInstance* ItemInstance = ResolveInventoryItem(Slot);
		if (Slot.ItemInstanceId.IsValid() && !ItemInstance)
		{
			// Slots 与 PlayerState InventoryManager 的 UObject 子对象使用独立复制通道，
			// Guid 先到并不代表 ItemInstance 已能解析；Tick 只在这个窗口继续对账。
			bPendingReplicatedSlotResolution = true;
		}
		ResolveWeaponInstance(Slot);
		RefreshSlotActorFromEquipment(Slot);

		FQuickbarSlotData Data;
		PopulateQuickbarSlotData(Slot, Data);
		Msg.Slots.Add(Data);
	}

	RefreshItemStatTagBindings();

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(ShootGameplayTags.Msg_Quickbar_SlotsChanged, Msg);

}

void UCombatComponent::ClearItemStatTagBindings()
{
	for (const TWeakObjectPtr<UShootInventoryItemInstance>& ItemPtr : BoundStatTagItems)
	{
		if (UShootInventoryItemInstance* Item = ItemPtr.Get())
		{
			Item->OnStatTagsChanged().RemoveAll(this);
		}
	}
	BoundStatTagItems.Reset();
}

void UCombatComponent::RefreshItemStatTagBindings()
{
	ClearItemStatTagBindings();

	for (FShootQuickbarSlot& Slot : Slots)
	{
		if (UShootInventoryItemInstance* Item = ResolveInventoryItem(Slot))
		{
			if (!BoundStatTagItems.Contains(Item))
			{
				Item->OnStatTagsChanged().AddUObject(this, &ThisClass::HandleItemStatTagsChanged);
				BoundStatTagItems.Add(Item);
			}
		}
	}
}

void UCombatComponent::HandleItemStatTagsChanged(UShootInventoryItemInstance* ChangedItem)
{
	BroadcastAmmoChangedForItem(ChangedItem);
}

void UCombatComponent::BroadcastAmmoChangedForItem(UShootInventoryItemInstance* ChangedItem)
{
	if (!ChangedItem || !UGameplayMessageSubsystem::HasInstance(this))
	{
		return;
	}

	for (FShootQuickbarSlot& Slot : Slots)
	{
		if (ResolveInventoryItem(Slot) != ChangedItem)
		{
			continue;
		}

		FQuickbarSlotData SlotData;
		PopulateQuickbarSlotData(Slot, SlotData);
		if (SlotData.WeaponId.IsNone())
		{
			return;
		}

		FWeaponAmmoChangedMessage Message;
		Message.Owner = GetOwner();
		Message.WeaponId = SlotData.WeaponId;
		Message.Ammo = SlotData.Ammo;
		Message.Reserve = SlotData.Reserve;

		UGameplayMessageSubsystem::Get(this).BroadcastMessage(
			FShootGameplayTags::Get().Msg_Weapon_AmmoChanged,
			Message);
		return;
	}
}

void UCombatComponent::OnRep_ActiveSlotIndex()
{
	const FShootGameplayTags& ShootGameplayTags = FShootGameplayTags::Get();
	
	FQuickbarActiveIndexChangedMessage Msg;
	Msg.Owner = GetOwner();
	Msg.ActiveIndex = ActiveSlotIndex;

	UGameplayMessageSubsystem::Get(this).BroadcastMessage(ShootGameplayTags.Msg_Quickbar_ActiveIndexChanged, Msg);

}


// Called every frame
void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bPendingReplicatedSlotResolution || !GetOwner() || GetOwner()->HasAuthority())
	{
		return;
	}

	// 不使用固定延迟猜测复制何时结束：只要所有已占用槽的 ItemInstance 都能按 Guid
	// 从本玩家 PlayerState 的 InventoryManager 解析，就重走同一 OnRep 快照/绑定入口。
	for (FShootQuickbarSlot& Slot : Slots)
	{
		if (Slot.ItemInstanceId.IsValid() && !ResolveInventoryItem(Slot))
		{
			return;
		}
	}

	bPendingReplicatedSlotResolution = false;
	OnRep_Slots();
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Slots);
	DOREPLIFETIME(ThisClass, ActiveSlotIndex);
}

void UCombatComponent::CycleActiveSlotForward()
{
	if (Slots.Num() < 2)
	{
		return;
	}

	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num() - 1 : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex + 1) % Slots.Num();
		if (Slots[NewIndex].IsOccupied())
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	}
	while (NewIndex != OldIndex);
}

void UCombatComponent::CycleActiveSlotBackward()
{
	if (Slots.Num() < 2)
	{
		return;
	}

	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num() - 1 : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex - 1 + Slots.Num()) % Slots.Num();
		if (Slots[NewIndex].IsOccupied())
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	}
	while (NewIndex != OldIndex);
}

AActor* UCombatComponent::GetActiveSlotItem() const
{
	if (!Slots.IsValidIndex(ActiveSlotIndex))
	{
		return nullptr;
	}
	return Slots[ActiveSlotIndex].WeaponActor;
}

UShootWeaponInstance* UCombatComponent::GetActiveWeaponInstance() const
{
	if (!Slots.IsValidIndex(ActiveSlotIndex))
	{
		return nullptr;
	}
	FShootQuickbarSlot& ActiveSlot = const_cast<FShootQuickbarSlot&>(Slots[ActiveSlotIndex]);
	return GetOrCreateWeaponInstanceFromInventory(ActiveSlot);
}

UShootWeaponInstance* UCombatComponent::FindActiveWeaponInstance() const
{
	if (!Slots.IsValidIndex(ActiveSlotIndex))
	{
		return nullptr;
	}

	const FShootQuickbarSlot& ActiveSlot = Slots[ActiveSlotIndex];
	if (UShootWeaponInstance* WeaponInstance = ActiveSlot.WeaponInstance.Get())
	{
		return WeaponInstance;
	}

	if (UShootWeaponInstance* WeaponInstance = Cast<UShootWeaponInstance>(ActiveSlot.EquipmentInstance.Get()))
	{
		return WeaponInstance;
	}

	const UShootInventoryManagerComponent* InventoryManager = GetInventoryManagerForOwner();
	UShootInventoryItemInstance* ItemInstance = InventoryManager && ActiveSlot.ItemInstanceId.IsValid()
		? InventoryManager->FindItemByInstanceId(ActiveSlot.ItemInstanceId)
		: nullptr;
	if (!ItemInstance)
	{
		return nullptr;
	}

	if (const UShootEquipmentManagerComponent* EquipmentManager = GetEquipmentManager())
	{
		const TArray<UShootEquipmentInstance*> WeaponInstances = EquipmentManager->GetEquipmentInstancesOfType(UShootWeaponInstance::StaticClass());
		for (UShootEquipmentInstance* EquipmentInstance : WeaponInstances)
		{
			if (EquipmentInstance && EquipmentInstance->GetInstigator() == ItemInstance)
			{
				return Cast<UShootWeaponInstance>(EquipmentInstance);
			}
		}
	}

	return nullptr;
}

bool UCombatComponent::DropActiveRuntimeWeapon()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("DropActiveRuntimeWeapon failed: component is not authoritative."));
		return false;
	}

	if (!Slots.IsValidIndex(ActiveSlotIndex))
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("DropActiveRuntimeWeapon failed: ActiveSlotIndex is invalid."));
		return false;
	}

	FShootQuickbarSlot& ActiveSlot = Slots[ActiveSlotIndex];
	if (ActiveSlot.Lifetime != EShootItemLifetime::RuntimeOnly)
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("DropActiveRuntimeWeapon ignored: slot %d is not RuntimeOnly."), ActiveSlotIndex);
		return false;
	}

	if (!ActiveSlot.ItemInstanceId.IsValid())
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("DropActiveRuntimeWeapon failed: slot %d has no ItemInstanceId."), ActiveSlotIndex);
		return false;
	}

	UE_LOG(LogCombatComponent, Log, TEXT("[%s] Dropping runtime weapon from slot %d (ItemInstanceId=%s)"),
		CombatComponentLog::GetAuthorityString(this),
		ActiveSlotIndex, *ActiveSlot.ItemInstanceId.ToString());

	RemoveItemFromSlot(ActiveSlotIndex);
	return true;
}

bool UCombatComponent::ReplaceActiveRuntimeWeapon(UShootInventoryItemInstance* ReplacementItem, int32& OutSlotIndex)
{
	OutSlotIndex = INDEX_NONE;
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ReplacementItem ||
		ReplacementItem->GetItemLifetime() != EShootItemLifetime::RuntimeOnly ||
		!Slots.IsValidIndex(ActiveSlotIndex))
	{
		return false;
	}

	const int32 ReplacementSlotIndex = ActiveSlotIndex;
	const FShootQuickbarSlot& SlotToReplace = Slots[ReplacementSlotIndex];
	if (SlotToReplace.Lifetime != EShootItemLifetime::RuntimeOnly || !SlotToReplace.ItemInstanceId.IsValid() ||
		!IsItemAllowedInSlot(ReplacementSlotIndex, ReplacementItem))
	{
		return false;
	}

	// 满槽拾枪采用“替换当前枪”策略：玩家明确知道自己将放下哪一把，同时保持原槽位位置不变。
	// 低层移除先生成旧枪掉落并删除旧 RuntimeOnly ItemInstance，但不自动装备邻槽、不广播中间空态。
	RemoveItemFromSlotInternal(ReplacementSlotIndex, false, false);
	AssignSlotFromInventory(ReplacementSlotIndex, ReplacementItem->GetItemInstanceId());

	if (!Slots.IsValidIndex(ReplacementSlotIndex) ||
		Slots[ReplacementSlotIndex].ItemInstanceId != ReplacementItem->GetItemInstanceId())
	{
		UE_LOG(LogCombatComponent, Error,
			TEXT("ReplaceActiveRuntimeWeapon failed to bind replacement item %s to slot %d."),
			*ReplacementItem->GetItemInstanceId().ToString(), ReplacementSlotIndex);
		return false;
	}

	OutSlotIndex = ReplacementSlotIndex;
	return true;
}

void UCombatComponent::HandleWeaponAnimNotify(EShootWeaponAnimAction Action)
{
	if (UShootWeaponInstance* WeaponInstance = GetActiveWeaponInstance())
	{
		WeaponInstance->HandleVisualAnimCue(Action);
	}
}

int32 UCombatComponent::GetNextFreeItemSlot() const
{
	int32 SlotIndex = 0;
	for (const FShootQuickbarSlot& Slot : Slots)
	{
		if (!Slot.IsOccupied())
		{
			return SlotIndex;
		}
		++SlotIndex;
	}

	return INDEX_NONE;
}

void UCombatComponent::AddItemToSlot(int32 SlotIndex, AActor* Item, UShootInventoryItemInstance* InventoryItem)
{
	if (!Slots.IsValidIndex(SlotIndex) || (!Item && !InventoryItem))
	{
		return;
	}

	FShootQuickbarSlot& Slot = Slots[SlotIndex];
	if (Slot.IsOccupied())
	{
		return;
	}

	if (InventoryItem)
	{
		Slot.ItemInstanceId = InventoryItem->GetItemInstanceId();
		Slot.Lifetime = InventoryItem->GetItemLifetime();
		Slot.CachedItemInstance = InventoryItem;
		Slot.WeaponActor = nullptr;
		Slot.EquipmentInstance.Reset();
		Slot.WeaponInstance.Reset();

		// 与 Lyra QuickBar 保持同一不变量：槽位只保存 Inventory ItemInstance，
		// Pawn EquipmentManager 同时只物化当前激活槽。非激活枪若在拾取时也 Equip，
		// 会累计多个 WeaponInstance，并让最后拾取的枪错误覆盖当前 AnimLayer。
	}
	else
	{
		Slot.ItemInstanceId.Invalidate();
		Slot.Lifetime = EShootItemLifetime::Persistent;
		Slot.CachedItemInstance.Reset();
		Slot.EquipmentInstance.Reset();
		Slot.WeaponInstance.Reset();
		Slot.WeaponActor = Item;
	}

	const bool bIsServer = GetOwner() && GetOwner()->HasAuthority();

	// 自动装备第一件武器（服务器）
	if (bIsServer && ActiveSlotIndex < 0)
	{
		ActiveSlotIndex = SlotIndex;
		EquipItemInSlot();
		OnRep_ActiveSlotIndex();
	}

	OnRep_Slots();
}

void UCombatComponent::GetQuickbarSlotsData(TArray<FQuickbarSlotData>& OutSlots)
{
	OutSlots.Reset();

	for (FShootQuickbarSlot& Slot : Slots)
	{
		FQuickbarSlotData Data;
		PopulateQuickbarSlotData(Slot, Data);
		OutSlots.Add(Data);
	}
}

void UCombatComponent::AssignSlotFromInventory(int32 SlotIndex, FGuid ItemInstanceId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Slots.IsValidIndex(SlotIndex) || !ItemInstanceId.IsValid())
	{
		return;
	}

	if (UShootInventoryManagerComponent* InventoryManager = GetInventoryManagerForOwner())
	{
		if (UShootInventoryItemInstance* ItemInstance = InventoryManager->FindItemByInstanceId(ItemInstanceId))
		{
			AddItemToSlot(SlotIndex, /*Item=*/nullptr, ItemInstance);

			UE_LOG(LogCombatComponent, Log,
				TEXT("[%s] Assigned ItemInstance %s (Lifetime=%s) to slot %d."),
				CombatComponentLog::GetAuthorityString(this),
				*ItemInstanceId.ToString(),
				CombatComponentLog::GetLifetimeString(ItemInstance->GetItemLifetime()),
				SlotIndex);
		}
	}
}

void UCombatComponent::ReleaseSlot(int32 SlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	RemoveItemFromSlot(SlotIndex);
}

void UCombatComponent::BuildQuickbarSaveData(TArray<FSavedQuickbarSlot>& OutSlots) const
{
	OutSlots.Reset();

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		const FShootQuickbarSlot& Slot = Slots[Index];
		if (Slot.Lifetime == EShootItemLifetime::Persistent && Slot.ItemInstanceId.IsValid())
		{
			FSavedQuickbarSlot SavedSlot;
			SavedSlot.SlotIndex = Index;
			SavedSlot.ItemInstanceId = Slot.ItemInstanceId;
			OutSlots.Add(SavedSlot);
		}
	}
}

void UCombatComponent::ApplyQuickbarSaveData(const TArray<FSavedQuickbarSlot>& InSlots)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		ReleaseSlot(Index);
	}

	for (const FSavedQuickbarSlot& SavedSlot : InSlots)
	{
		if (SavedSlot.SlotIndex != INDEX_NONE && Slots.IsValidIndex(SavedSlot.SlotIndex))
		{
			AssignSlotFromInventory(SavedSlot.SlotIndex, SavedSlot.ItemInstanceId);
		}
	}
}

void UCombatComponent::InitializeRuntimeLoadoutFromPersistentSlots(const TArray<FSavedQuickbarSlot>& InPersistentSlots)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UShootInventoryManagerComponent* InventoryManager = GetInventoryManagerForOwner();
	if (!InventoryManager)
	{
		return;
	}

	// 先释放上一回合/上一次死亡的会话槽位。Persistent 槽位只解除引用，RuntimeOnly 槽位会被库存清理。
	TArray<FSavedQuickbarSlot> EmptySlots;
	ApplyQuickbarSaveData(EmptySlots);
	InventoryManager->RemoveRuntimeItems();

	for (const FSavedQuickbarSlot& PersistentSlot : InPersistentSlots)
	{
		if (!Slots.IsValidIndex(PersistentSlot.SlotIndex) || !PersistentSlot.ItemInstanceId.IsValid())
		{
			continue;
		}

		UShootInventoryItemInstance* PersistentItem = InventoryManager->FindItemByInstanceId(PersistentSlot.ItemInstanceId);
		if (!PersistentItem || PersistentItem->GetItemLifetime() != EShootItemLifetime::Persistent)
		{
			continue;
		}

		UShootInventoryItemInstance* RuntimeItem = InventoryManager->CreateRuntimeWeaponFromPersistent(PersistentItem);
		if (RuntimeItem)
		{
			AssignSlotFromInventory(PersistentSlot.SlotIndex, RuntimeItem->GetItemInstanceId());
		}
	}
}

void UCombatComponent::BuildRuntimeQuickbarSessionData(TArray<FSavedQuickbarSlot>& OutSlots, int32& OutActiveSlotIndex) const
{
	OutSlots.Reset();
	OutActiveSlotIndex = INDEX_NONE;

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		const FShootQuickbarSlot& Slot = Slots[SlotIndex];
		if (Slot.Lifetime != EShootItemLifetime::RuntimeOnly || !Slot.ItemInstanceId.IsValid())
		{
			continue;
		}

		FSavedQuickbarSlot& SavedSlot = OutSlots.AddDefaulted_GetRef();
		SavedSlot.SlotIndex = SlotIndex;
		SavedSlot.ItemInstanceId = Slot.ItemInstanceId;
	}

	if (Slots.IsValidIndex(ActiveSlotIndex) && Slots[ActiveSlotIndex].Lifetime == EShootItemLifetime::RuntimeOnly)
	{
		OutActiveSlotIndex = ActiveSlotIndex;
	}
}

void UCombatComponent::DetachRuntimeQuickbarSessionForPawnTransition()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		FShootQuickbarSlot& Slot = Slots[SlotIndex];
		if (Slot.Lifetime != EShootItemLifetime::RuntimeOnly)
		{
			continue;
		}

		if (UShootEquipmentInstance* EquipInstance = Slot.EquipmentInstance.Get())
		{
			if (UShootEquipmentManagerComponent* EquipmentManager = GetEquipmentManager())
			{
				EquipmentManager->UnequipItem(EquipInstance);
			}
		}

		Slot.ItemInstanceId.Invalidate();
		Slot.Lifetime = EShootItemLifetime::Persistent;
		Slot.WeaponActor = nullptr;
		Slot.CachedItemInstance.Reset();
		Slot.EquipmentInstance.Reset();
		Slot.WeaponInstance.Reset();
	}

	ActiveSlotIndex = INDEX_NONE;
	// Pawn 即将失去 Controller；只广播旧 Pawn 的卸载结果，InventoryManager 中的 RuntimeOnly 实例继续由 Controller 会话持有。
	OnRep_Slots();
	OnRep_ActiveSlotIndex();
}

void UCombatComponent::RestoreRuntimeQuickbarSessionData(const TArray<FSavedQuickbarSlot>& InSlots, int32 InActiveSlotIndex)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	for (const FSavedQuickbarSlot& SavedSlot : InSlots)
	{
		if (!Slots.IsValidIndex(SavedSlot.SlotIndex) || !SavedSlot.ItemInstanceId.IsValid())
		{
			continue;
		}

		AssignSlotFromInventory(SavedSlot.SlotIndex, SavedSlot.ItemInstanceId);
	}

	if (Slots.IsValidIndex(InActiveSlotIndex) && Slots[InActiveSlotIndex].Lifetime == EShootItemLifetime::RuntimeOnly)
	{
		SetActiveSlotIndex(InActiveSlotIndex);
	}
}

bool UCombatComponent::SwapRuntimeQuickbarSlots(int32 SlotIndexA, int32 SlotIndexB)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || SlotIndexA == SlotIndexB ||
		!Slots.IsValidIndex(SlotIndexA) || !Slots.IsValidIndex(SlotIndexB))
	{
		return false;
	}

	if (Slots[SlotIndexA].Lifetime != EShootItemLifetime::RuntimeOnly ||
		Slots[SlotIndexB].Lifetime != EShootItemLifetime::RuntimeOnly)
	{
		return false;
	}

	Swap(Slots[SlotIndexA], Slots[SlotIndexB]);
	if (ActiveSlotIndex == SlotIndexA)
	{
		ActiveSlotIndex = SlotIndexB;
	}
	else if (ActiveSlotIndex == SlotIndexB)
	{
		ActiveSlotIndex = SlotIndexA;
	}

	// 会话槽位交换只改变 Controller 所属的本局引用，Equipment/WeaponActor 仍由当前 Pawn CombatComponent 负责。
	OnRep_Slots();
	OnRep_ActiveSlotIndex();
	return true;
}

void UCombatComponent::ClearRuntimeSlots()
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (Slots[Index].Lifetime == EShootItemLifetime::RuntimeOnly)
		{
			const bool bPreviousDropSpawnState = bDisableRuntimeDropSpawn;
			bDisableRuntimeDropSpawn = true;
			ReleaseSlot(Index);
			bDisableRuntimeDropSpawn = bPreviousDropSpawnState;
		}
	}

	// ResourceInventory = 数量型仓库，InventoryManager = 有身份背包
	// QuickBar 清理 RuntimeOnly 槽位时必须同步通知 InventoryManager 移除临时实例
	if (UShootInventoryManagerComponent* InventoryManager = GetInventoryManagerForOwner())
	{
		InventoryManager->RemoveRuntimeItems();
	}
}

void UCombatComponent::Server_SetQuickbarSlot(int32 SlotIndex, UShootInventoryItemInstance* ItemInstance)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!Slots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("Server_SetQuickbarSlot invalid index %d"), SlotIndex);
		return;
	}

	if (!ItemInstance)
	{
		Server_ClearQuickbarSlot(SlotIndex);
		return;
	}

	UShootInventoryManagerComponent* Inventory = GetInventoryManagerForOwner();
	if (!Inventory || !Inventory->GetAllItems().Contains(ItemInstance))
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("Server_SetQuickbarSlot: ItemInstance not in inventory"));
		return;
	}

	if (!IsItemAllowedInSlot(SlotIndex, ItemInstance))
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("Server_SetQuickbarSlot rejected: slot type mismatch (slot=%d, item=%s)"),
			SlotIndex, *GetNameSafe(ItemInstance));
		return;
	}

	FShootQuickbarSlot& Slot = Slots[SlotIndex];
	Slot.ItemInstanceId = ItemInstance->GetItemInstanceId();
	Slot.Lifetime = ItemInstance->GetItemLifetime();
	Slot.CachedItemInstance = ItemInstance;
	Slot.EquipmentInstance.Reset();
	Slot.WeaponInstance.Reset();

	// TODO: 槽位类型与物品类型匹配校验（主武器/副武器/近战），当前允许任意，后续补齐
}

void UCombatComponent::Server_ClearQuickbarSlot(int32 SlotIndex)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!Slots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("Server_ClearQuickbarSlot invalid index %d"), SlotIndex);
		return;
	}

	FShootQuickbarSlot& Slot = Slots[SlotIndex];
	Slot.ItemInstanceId.Invalidate();
	Slot.WeaponActor = nullptr;
	Slot.CachedItemInstance.Reset();
	Slot.EquipmentInstance.Reset();
	Slot.WeaponInstance.Reset();
}

void UCombatComponent::Server_SwapQuickbarSlots(int32 SlotIndexA, int32 SlotIndexB)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	if (!Slots.IsValidIndex(SlotIndexA) || !Slots.IsValidIndex(SlotIndexB))
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("Server_SwapQuickbarSlots invalid indices %d, %d"), SlotIndexA, SlotIndexB);
		return;
	}

	if (SlotIndexA == SlotIndexB)
	{
		return;
	}

	FShootQuickbarSlot Temp = Slots[SlotIndexA];
	Slots[SlotIndexA] = Slots[SlotIndexB];
	Slots[SlotIndexB] = Temp;
}

bool UCombatComponent::GrantAmmoToActiveWeapon(int32 MagazineDelta, int32 ReserveDelta)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!Slots.IsValidIndex(ActiveSlotIndex))
	{
		return false;
	}

	UShootWeaponInstance* WeaponInstance = GetActiveWeaponInstance();
	UShootRangedWeaponInstance* RangedInstance = Cast<UShootRangedWeaponInstance>(WeaponInstance);
	if (!RangedInstance)
	{
		return false;
	}

	int32 AddedMag = 0;
	int32 AddedReserve = 0;
	if (ReserveDelta > 0)
	{
		AddedReserve = RangedInstance->AddReserveAmmo(ReserveDelta);
	}
	if (MagazineDelta > 0)
	{
		AddedMag = RangedInstance->AddMagazineAmmo(MagazineDelta);
	}

	if (AddedMag > 0 || AddedReserve > 0)
	{
		UE_LOG(LogCombatComponent, Log, TEXT("[%s] Granted ammo to active weapon (Mag=%d, Reserve=%d)"),
			CombatComponentLog::GetAuthorityString(this), AddedMag, AddedReserve);
		return true;
	}

	return false;
}

UShootInventoryManagerComponent* UCombatComponent::GetInventoryManagerForOwner() const
{
	// QuickBar 只引用 PlayerState 上的 InventoryManager，避免与 ResourceInventory 或散落的武器 Actor 混淆
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (AShootPlayerState* PlayerState = Character->GetPlayerState<AShootPlayerState>())
		{
			return PlayerState->GetInventoryManagerComponent();
		}
	}

	return nullptr;
}

void UCombatComponent::PopulateQuickbarSlotData(FShootQuickbarSlot& Slot, FQuickbarSlotData& OutData) const
{
	OutData.ItemInstanceId = Slot.ItemInstanceId;
	OutData.Lifetime = Slot.Lifetime;
	OutData.ItemDefinition = nullptr;
	OutData.DisplayName = FText::GetEmpty();

	UShootWeaponInstance* WeaponInstance = ResolveWeaponInstance(Slot);
	RefreshSlotActorFromEquipment(Slot);

	if (UShootRangedWeaponInstance* RangedInstance = Cast<UShootRangedWeaponInstance>(WeaponInstance))
	{
		// Equipment FastArray、EquipmentInstance::Instigator 与 QuickBar Slots 分属独立复制状态。
		// WeaponInstance 已生成但 Instigator 尚未到达时，槽位 Guid 仍能解析同一个 Inventory Item；
		// UI 必须从这个权威实例读取弹药，不能因为表现层桥接暂时为空就发布 0/0 快照。
		UShootInventoryItemInstance* ItemInstance = RangedInstance->GetItemInstance();
		if (!ItemInstance)
		{
			ItemInstance = ResolveInventoryItem(Slot);
		}

		if (ItemInstance)
		{
			const FShootGameplayTags& ShootGameplayTags = FShootGameplayTags::Get();
			OutData.Ammo = ItemInstance->GetStatTagStackCount(ShootGameplayTags.Inventory_Ammo_Magazine);
			OutData.Reserve = ItemInstance->GetStatTagStackCount(ShootGameplayTags.Inventory_Ammo_Reserve);
			OutData.DisplayName = ItemInstance->GetItemDisplayName();
			OutData.ItemDefinition = ItemInstance->GetItemDef();
			OutData.Icon = ItemInstance->GetItemDisplayIcon();

			if (const TSubclassOf<UShootInventoryItemDefinition> ItemDefClass = ItemInstance->GetItemDef())
			{
				if (const UShootInventoryItemDefinition* ItemDef = ItemDefClass->GetDefaultObject<UShootInventoryItemDefinition>())
				{
					if (const UShootInventoryFragment_WeaponBasicConfig* BasicCfg =
						Cast<UShootInventoryFragment_WeaponBasicConfig>(
							ItemDef->FindFragmentByClass(UShootInventoryFragment_WeaponBasicConfig::StaticClass())))
					{
						OutData.WeaponId = BasicCfg->WeaponId;
						OutData.AmmoIcon = BasicCfg->AmmoIcon;
					}
				}
			}
		}
		else
		{
			// 无槽位 Item 的非库存装备仍保留 WeaponInstance 查询；正常 QuickBar 武器不会走这里。
			OutData.Ammo = RangedInstance->GetCurrentAmmo();
			OutData.Reserve = RangedInstance->GetCurrentReserve();
		}

		return;
	}

	// ResourceInventory = 数量型仓库，InventoryManager = 有身份背包
	// 当还没有生成武器实例时，从 InventoryItem 读取弹药数据以驱动 UI
	if (UShootInventoryItemInstance* ItemInstance = ResolveInventoryItem(Slot))
	{
		const FShootGameplayTags& ShootGameplayTags = FShootGameplayTags::Get();
		OutData.Ammo = ItemInstance->GetStatTagStackCount(ShootGameplayTags.Inventory_Ammo_Magazine);
		OutData.Reserve = ItemInstance->GetStatTagStackCount(ShootGameplayTags.Inventory_Ammo_Reserve);
		OutData.Icon = ItemInstance->GetItemDisplayIcon();
		OutData.DisplayName = ItemInstance->GetItemDisplayName();
		OutData.ItemDefinition = ItemInstance->GetItemDef();

		// 非激活槽可能暂时没有 Equipment/WeaponInstance，QuickBar 仍须直接从 ItemDefinition
		// 读取稳定的 WeaponId；否则丢弃当前枪时相邻槽会短暂显示成 None。
		// DisplayName/图标的唯一数据源是 ItemDefinition 根部；BasicConfig 不再保留重复字段。
		if (const TSubclassOf<UShootInventoryItemDefinition> ItemDefClass = ItemInstance->GetItemDef())
		{
			if (const UShootInventoryItemDefinition* ItemDef =
				ItemDefClass->GetDefaultObject<UShootInventoryItemDefinition>())
			{
				if (const UShootInventoryFragment_WeaponBasicConfig* BasicCfg =
					Cast<UShootInventoryFragment_WeaponBasicConfig>(
						ItemDef->FindFragmentByClass(UShootInventoryFragment_WeaponBasicConfig::StaticClass())))
				{
					OutData.WeaponId = BasicCfg->WeaponId;
					OutData.AmmoIcon = BasicCfg->AmmoIcon;
				}
			}
		}
		return;
	}

	OutData.DisplayName = FText::GetEmpty();
	OutData.ItemDefinition = nullptr;
}

AActor* UCombatComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	return RemoveItemFromSlotInternal(SlotIndex, true, true);
}

AActor* UCombatComponent::RemoveItemFromSlotInternal(int32 SlotIndex, bool bAutoSelectNext, bool bBroadcastState)
{
	AActor* Result = nullptr;
	const bool bRemovedActiveSlot = ActiveSlotIndex == SlotIndex;

	if (bRemovedActiveSlot)
	{
		UnequipItemInSlot();
		ActiveSlotIndex = -1;
	}

	if (Slots.IsValidIndex(SlotIndex))
	{
		FShootQuickbarSlot& Slot = Slots[SlotIndex];
		Result = Slot.WeaponActor;
		UShootInventoryItemInstance* DroppedInstance = ResolveInventoryItem(Slot);
		TSubclassOf<UShootInventoryItemDefinition> DroppedItemDef = DroppedInstance ? DroppedInstance->GetItemDef() : nullptr;
		FShootGameplayTagStackContainer DroppedStatSnapshot;
		if (DroppedInstance)
		{
			DroppedStatSnapshot = DroppedInstance->GetStatTags();
		}

		if (Result != nullptr || Slot.ItemInstanceId.IsValid())
		{
			const FGuid RemovedInstanceId = Slot.ItemInstanceId;
			const EShootItemLifetime RemovedLifetime = Slot.Lifetime;
			TWeakObjectPtr<UShootEquipmentInstance> EquipInstanceWeak = Slot.EquipmentInstance;

			Slot.WeaponActor = nullptr;
			Slot.ItemInstanceId.Invalidate();
			Slot.Lifetime = EShootItemLifetime::Persistent;
			Slot.CachedItemInstance.Reset();
			Slot.EquipmentInstance.Reset();
			Slot.WeaponInstance.Reset();

			if (UShootEquipmentInstance* EquipInstance = EquipInstanceWeak.Get())
			{
				if (UShootEquipmentManagerComponent* EquipmentManager = GetEquipmentManager())
				{
					EquipmentManager->UnequipItem(EquipInstance);
				}
			}

			UE_LOG(LogCombatComponent, Log,
				TEXT("[%s] Removed slot %d (Lifetime=%s, InstanceId=%s)."),
				CombatComponentLog::GetAuthorityString(this),
				SlotIndex,
				CombatComponentLog::GetLifetimeString(RemovedLifetime),
				RemovedInstanceId.IsValid() ? *RemovedInstanceId.ToString() : TEXT("Invalid"));

			if (RemovedLifetime == EShootItemLifetime::RuntimeOnly && RemovedInstanceId.IsValid())
			{
				if (UShootInventoryManagerComponent* InventoryManager = GetInventoryManagerForOwner())
				{
					if (DroppedInstance)
					{
						InventoryManager->RemoveItemInstance(DroppedInstance);
					}
					else
					{
						InventoryManager->RemoveItemByInstanceId(RemovedInstanceId);
					}
				}

				if (!bDisableRuntimeDropSpawn && GetOwner() && GetOwner()->HasAuthority())
				{
					HandleRuntimeWeaponDropVisual(DroppedItemDef, DroppedStatSnapshot, RemovedLifetime);
				}
				else if (RemovedLifetime == EShootItemLifetime::RuntimeOnly)
				{
					UE_LOG(LogCombatComponent, Log,
						TEXT("[%s] Runtime drop spawn suppressed (bDisableRuntimeDropSpawn=%d)."),
						CombatComponentLog::GetAuthorityString(this),
						bDisableRuntimeDropSpawn ? 1 : 0);
				}
			}

			if (bBroadcastState)
			{
				OnRep_Slots();
			}

			if (bRemovedActiveSlot && bAutoSelectNext)
			{
				// 玩家仍携带武器时，按 QuickBar 顺序自动切到下一把；只有最后一把也被丢弃时才进入空手。
				for (int32 Offset = 1; Offset <= Slots.Num(); ++Offset)
				{
					const int32 CandidateIndex = (SlotIndex + Offset) % Slots.Num();
					if (Slots[CandidateIndex].IsOccupied())
					{
						ActiveSlotIndex = CandidateIndex;
						EquipItemInSlot();
						break;
					}
				}
			}

			if (bRemovedActiveSlot && bBroadcastState)
			{
				OnRep_ActiveSlotIndex();
			}
		}
	}

	return Result;
}

void UCombatComponent::SetActiveSlotIndex_Implementation(int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex != NewIndex))
	{
		UnequipItemInSlot();

		ActiveSlotIndex = NewIndex;

		EquipItemInSlot();

		OnRep_ActiveSlotIndex();
	}
}

UShootInventoryItemInstance* UCombatComponent::ResolveInventoryItem(FShootQuickbarSlot& Slot) const
{
	if (Slot.CachedItemInstance.IsValid())
	{
		return Slot.CachedItemInstance.Get();
	}

	if (!Slot.ItemInstanceId.IsValid())
	{
		return nullptr;
	}

	if (UShootInventoryManagerComponent* InventoryManager = GetInventoryManagerForOwner())
	{
		if (UShootInventoryItemInstance* Instance = InventoryManager->FindItemByInstanceId(Slot.ItemInstanceId))
		{
			Slot.CachedItemInstance = Instance;
			return Instance;
		}
	}

	return nullptr;
}

UShootEquipmentManagerComponent* UCombatComponent::GetEquipmentManager() const
{
	if (const AActor* OwnerActor = GetOwner())
	{
		return OwnerActor->FindComponentByClass<UShootEquipmentManagerComponent>();
	}
	return nullptr;
}

UShootEquipmentInstance* UCombatComponent::ResolveEquipmentInstance(FShootQuickbarSlot& Slot) const
{
	if (UShootEquipmentInstance* EquipInstance = Slot.EquipmentInstance.Get())
	{
		return EquipInstance;
	}

	if (UShootInventoryItemInstance* ItemInstance = ResolveInventoryItem(Slot))
	{
		if (UShootEquipmentManagerComponent* EquipmentManager = GetEquipmentManager())
		{
			const TArray<UShootEquipmentInstance*> WeaponEquipInstances = EquipmentManager->GetEquipmentInstancesOfType(UShootWeaponInstance::StaticClass());
			for (UShootEquipmentInstance* Candidate : WeaponEquipInstances)
			{
				if (Candidate && Candidate->GetInstigator() == ItemInstance)
				{
					Slot.EquipmentInstance = Candidate;
					return Candidate;
				}
			}
		}
	}

	return nullptr;
}

UShootWeaponInstance* UCombatComponent::ResolveWeaponInstance(FShootQuickbarSlot& Slot) const
{
	if (UShootWeaponInstance* WeaponInstance = Cast<UShootWeaponInstance>(Slot.WeaponInstance.Get()))
	{
		return WeaponInstance;
	}

	if (UShootEquipmentInstance* EquipInstance = ResolveEquipmentInstance(Slot))
	{
		if (UShootWeaponInstance* WeaponFromEquip = Cast<UShootWeaponInstance>(EquipInstance))
		{
			Slot.WeaponInstance = WeaponFromEquip;
			return WeaponFromEquip;
		}
	}

	return nullptr;
}

UShootWeaponInstance* UCombatComponent::GetOrCreateWeaponInstanceFromInventory(FShootQuickbarSlot& Slot) const
{
	if (UShootWeaponInstance* ExistingInstance = ResolveWeaponInstance(Slot))
	{
		return ExistingInstance;
	}

	if (UShootInventoryItemInstance* ItemInstance = ResolveInventoryItem(Slot))
	{
		if (const UShootInventoryFragment_EquippableItem* EquipFragment = ItemInstance->FindFragmentByClass<
			UShootInventoryFragment_EquippableItem>())
		{
			if (EquipFragment->EquipmentDefinition)
			{
				if (UShootEquipmentManagerComponent* EquipmentManager = GetEquipmentManager())
				{
					if (UShootEquipmentInstance* EquipInstance = EquipmentManager->EquipItem(
						EquipFragment->EquipmentDefinition, ItemInstance))
					{
						Slot.EquipmentInstance = EquipInstance;
						if (UShootWeaponInstance* CreatedWeapon = Cast<UShootWeaponInstance>(EquipInstance))
						{
							Slot.WeaponInstance = CreatedWeapon;
							return CreatedWeapon;
						}
					}
				}
			}
		}
	}

	return ResolveWeaponInstance(Slot);
}

void UCombatComponent::RefreshSlotActorFromEquipment(FShootQuickbarSlot& Slot) const
{
	if (Slot.WeaponActor != nullptr)
	{
		return;
	}

	if (UShootEquipmentInstance* EquipInstance = Slot.EquipmentInstance.Get())
	{
		const TArray<AActor*> SpawnedActors = EquipInstance->GetSpawnedActors();
		for (AActor* SpawnedActor : SpawnedActors)
		{
			if (SpawnedActor && SpawnedActor->FindComponentByClass<USkeletalMeshComponent>())
			{
				// Lyra 的 B_Rifle/B_Pistol/B_Shotgun 本身就是完整武器 Actor；
				// 槽位只要求它提供 SkeletalMeshComponent，不再强制项目包装子类。
				Slot.WeaponActor = SpawnedActor;
				break;
			}
		}
	}
}

EShootQuickbarSlotType UCombatComponent::GetSlotTypeForIndex(int32 SlotIndex) const
{
	if (SlotTypeOverrides.IsValidIndex(SlotIndex))
	{
		return SlotTypeOverrides[SlotIndex];
	}

	// 默认规则来源：游戏设计完整文档 v7.0（主武器/副武器/近战槽位）
	if (SlotIndex == 0)
	{
		return EShootQuickbarSlotType::PrimaryWeapon;
	}

	if (SlotIndex == 1)
	{
		if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			if (const AShootPlayerState* PlayerState = Character->GetPlayerState<AShootPlayerState>())
			{
				return (PlayerState->GetPlayerLevel() >= 20)
					? EShootQuickbarSlotType::PrimaryWeapon
					: EShootQuickbarSlotType::SecondaryWeapon;
			}
		}

		return EShootQuickbarSlotType::SecondaryWeapon;
	}

	if (SlotIndex == 2)
	{
		return EShootQuickbarSlotType::MeleeWeapon;
	}

	return EShootQuickbarSlotType::Any;
}

bool UCombatComponent::IsItemAllowedInSlot(int32 SlotIndex, UShootInventoryItemInstance* ItemInstance) const
{
	if (!ItemInstance)
	{
		return true;
	}

	const EShootQuickbarSlotType SlotType = GetSlotTypeForIndex(SlotIndex);
	if (SlotType == EShootQuickbarSlotType::Any)
	{
		return true;
	}

	const UShootInventoryFragment_QuickbarSlotRules* SlotRules = ItemInstance->FindFragmentByClass<UShootInventoryFragment_QuickbarSlotRules>();
	if (!SlotRules || SlotRules->AllowedSlotTypes.Num() == 0)
	{
		// 未配置规则时默认放行，避免旧资产被强制拦截
		return true;
	}

	for (const EShootQuickbarSlotType Allowed : SlotRules->AllowedSlotTypes)
	{
		if (Allowed == EShootQuickbarSlotType::Any || Allowed == SlotType)
		{
			return true;
		}
	}

	return false;
}

void UCombatComponent::HandleRuntimeWeaponDropVisual(TSubclassOf<UShootInventoryItemDefinition> ItemDef,
	const FShootGameplayTagStackContainer& StatTags,
	EShootItemLifetime Lifetime) const
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemDef)
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("HandleRuntimeWeaponDropVisual skipped (Authority=%d, Class=%s, ItemDef=%s)"),
			GetOwner() ? GetOwner()->HasAuthority() : false,
			WeaponPickupActorClass ? *WeaponPickupActorClass->GetName() : TEXT("nullptr"),
			ItemDef ? *ItemDef->GetName() : TEXT("nullptr"));
		return;
	}

	TSubclassOf<AShootWeaponPickupActor> PickupClass = WeaponPickupActorClass;
	if (const UShootInventoryItemDefinition* ItemDefinitionCDO =
		ItemDef->GetDefaultObject<UShootInventoryItemDefinition>())
	{
		if (const UShootInventoryFragment_WeaponBasicConfig* BasicConfig =
			Cast<UShootInventoryFragment_WeaponBasicConfig>(
				ItemDefinitionCDO->FindFragmentByClass(UShootInventoryFragment_WeaponBasicConfig::StaticClass())))
		{
			if (BasicConfig->DroppedPickupActorClass)
			{
				PickupClass = BasicConfig->DroppedPickupActorClass;
			}
		}
	}
	if (!PickupClass)
	{
		UE_LOG(LogCombatComponent, Warning,
			TEXT("HandleRuntimeWeaponDropVisual skipped: ItemDef %s has no dropped pickup class."),
			*GetNameSafe(ItemDef));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 沿角色朝向向前放置 120cm，并抬高 40cm；距离大于角色胶囊与拾取球半径之和，
	// 避免掉落物生成时仍与玩家重叠。InitializeFromDrop 还会把掉落物切为 PressToInteract。
	const FVector SpawnLocation = GetOwner()->GetActorLocation()
		+ GetOwner()->GetActorForwardVector() * 120.f
		+ FVector(0.f, 0.f, 40.f);
	const FRotator SpawnRotation = GetOwner()->GetActorRotation();
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

	AShootWeaponPickupActor* PickupActor =
		World->SpawnActorDeferred<AShootWeaponPickupActor>(PickupClass, SpawnTransform, GetOwner());
	if (PickupActor)
	{
		PickupActor->InitializeFromDrop(ItemDef, Lifetime, StatTags);
		PickupActor->FinishSpawning(SpawnTransform);
		UE_LOG(LogCombatComponent, Log, TEXT("Spawned runtime weapon pickup %s at %s for ItemDef %s."),
			*GetNameSafe(PickupActor), *SpawnLocation.ToString(), *GetNameSafe(ItemDef));
	}
	else
	{
		UE_LOG(LogCombatComponent, Warning, TEXT("Failed to spawn weapon pickup actor for ItemDef %s."), *GetNameSafe(ItemDef));
	}
}
