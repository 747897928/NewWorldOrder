// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Inventory/ResourceInventoryComponent.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/Fragments/ShootInventoryFragment_StackRules.h"
#include "Net/UnrealNetwork.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "ShootGameplayTags.h"

// 重要提醒：ResourceInventory = 数量型账号仓库（材料/货币/徽章/设计图）
// InventoryManager = 有身份背包（武器/装备/可实例化消耗品）
// QuickBar/Equipment 只能引用 InventoryManager 的实例，禁止直接依赖 ResourceInventory

UResourceInventoryComponent::UResourceInventoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UResourceInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, Entries);
}

bool UResourceInventoryComponent::AddResource(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 Delta)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemDef || Delta <= 0)
	{
		return false;
	}

	// 数量型资源统一走 PlayerState 仓库，禁止在战斗内背包私自堆叠
	const int32 Index = FindOrAddEntry(ItemDef);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	int32& CountRef = Entries[Index].Count;
	const int32 OldCount = CountRef;
	const int32 MaxAllowed = GetMaxAllowedCount(ItemDef);
	if (MaxAllowed > 0 && (OldCount + Delta) > MaxAllowed)
	{
		// 资源达到上限，不允许继续增加
		return false;
	}
	CountRef = OldCount + Delta;
	BroadcastChange(ItemDef, OldCount, CountRef);
	// 客户端 OnRep 时用不到 LastKnownCounts（仅本地），但服务器更新一下方便调试
	LastKnownCounts.FindOrAdd(ItemDef) = CountRef;
	return true;
}

bool UResourceInventoryComponent::ConsumeResource(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 Delta)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemDef || Delta <= 0)
	{
		return false;
	}

	const int32 Index = FindOrAddEntry(ItemDef);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	int32& CountRef = Entries[Index].Count;
	if (CountRef < Delta)
	{
		return false;
	}

	const int32 OldCount = CountRef;
	CountRef = OldCount - Delta;
	BroadcastChange(ItemDef, OldCount, CountRef);
	LastKnownCounts.FindOrAdd(ItemDef) = CountRef;
	return true;
}

int32 UResourceInventoryComponent::GetResourceCount(TSubclassOf<UShootInventoryItemDefinition> ItemDef) const
{
	if (!ItemDef)
	{
		return 0;
	}
	for (const FResourceEntry& Entry : Entries)
	{
		if (Entry.ItemDef == ItemDef)
		{
			return Entry.Count;
		}
	}
	return 0;
}

void UResourceInventoryComponent::GetAllResources(TArray<FResourceEntry>& OutEntries) const
{
	// 客户端 UI 会调用该函数刷新面板数据
	OutEntries = Entries;
}

bool UResourceInventoryComponent::HasEnoughResource(const FResourceCost& ResourceCost) const
{
	for (const FResourceCostEntry& Entry : ResourceCost.Entries)
	{
		if (Entry.RequiredCount <= 0)
		{
			continue;
		}

		if (!Entry.ItemDef)
		{
			return false;
		}

		const int32 CurrentCount = GetResourceCount(Entry.ItemDef);
		if (CurrentCount < Entry.RequiredCount)
		{
			return false;
		}
	}

	return true;
}

void UResourceInventoryComponent::BuildResourceSaveData(TArray<FResourceEntry>& OutEntries) const
{
	OutEntries.Reset();

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	for (const FResourceEntry& Entry : Entries)
	{
		if (!Entry.ItemDef || Entry.Count <= 0)
		{
			continue;
		}

		FResourceEntry SavedEntry;
		SavedEntry.ItemDef = Entry.ItemDef;
		SavedEntry.Count = Entry.Count;
		OutEntries.Add(SavedEntry);
	}
}

void UResourceInventoryComponent::ApplyResourceSaveData(const TArray<FResourceEntry>& InEntries)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	TMap<TSubclassOf<UShootInventoryItemDefinition>, int32> OldCounts;
	for (const FResourceEntry& Entry : Entries)
	{
		if (Entry.ItemDef && Entry.Count > 0)
		{
			OldCounts.FindOrAdd(Entry.ItemDef) += Entry.Count;
		}
	}

	TMap<TSubclassOf<UShootInventoryItemDefinition>, int32> NewCounts;
	for (const FResourceEntry& Entry : InEntries)
	{
		if (Entry.ItemDef && Entry.Count > 0)
		{
			NewCounts.FindOrAdd(Entry.ItemDef) += Entry.Count;
		}
	}

	Entries.Reset();
	Entries.Reserve(NewCounts.Num());
	for (const TPair<TSubclassOf<UShootInventoryItemDefinition>, int32>& Pair : NewCounts)
	{
		FResourceEntry& NewEntry = Entries.AddDefaulted_GetRef();
		NewEntry.ItemDef = Pair.Key;
		NewEntry.Count = Pair.Value;
	}

	// 服务器端本地 UI 需要广播变化，避免 ListenServer 不触发 OnRep
	for (const TPair<TSubclassOf<UShootInventoryItemDefinition>, int32>& Pair : OldCounts)
	{
		const int32 NewCount = NewCounts.FindRef(Pair.Key);
		if (Pair.Value != NewCount)
		{
			BroadcastChange(Pair.Key, Pair.Value, NewCount);
		}
	}

	for (const TPair<TSubclassOf<UShootInventoryItemDefinition>, int32>& Pair : NewCounts)
	{
		if (!OldCounts.Contains(Pair.Key))
		{
			BroadcastChange(Pair.Key, 0, Pair.Value);
		}
	}

	LastKnownCounts = MoveTemp(NewCounts);
}

int32 UResourceInventoryComponent::FindOrAddEntry(TSubclassOf<UShootInventoryItemDefinition> ItemDef)
{
	if (!ItemDef)
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (Entries[Index].ItemDef == ItemDef)
		{
			return Index;
		}
	}

	FResourceEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.ItemDef = ItemDef;
	NewEntry.Count = 0;
	return Entries.Num() - 1;
}

int32 UResourceInventoryComponent::GetMaxAllowedCount(TSubclassOf<UShootInventoryItemDefinition> ItemDef) const
{
	if (!ItemDef)
	{
		return 0;
	}

	const UShootInventoryItemDefinition* ItemCDO = ItemDef->GetDefaultObject<UShootInventoryItemDefinition>();
	if (!ItemCDO)
	{
		return 0;
	}

	const UShootInventoryFragment_StackRules* StackRules = Cast<UShootInventoryFragment_StackRules>(
		ItemCDO->FindFragmentByClass(UShootInventoryFragment_StackRules::StaticClass()));
	if (!StackRules)
	{
		return 0;
	}

	if (StackRules->bUnique)
	{
		return 1;
	}

	return StackRules->MaxStackCount;
}

void UResourceInventoryComponent::BroadcastChange(const TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 OldCount, int32 NewCount)
{
	if (!ItemDef)
	{
		return;
	}

	FResourceChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ItemDef = ItemDef;
	Message.NewCount = NewCount;
	Message.Delta = NewCount - OldCount;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(FShootGameplayTags::Get().Inventory_Resource_Message_Changed, Message);

	// HUD 上的“+10 军用合金”提示走 UI Toast 频道，只在资源增加时推送
	if (Message.Delta > 0)
	{
		MessageSystem.BroadcastMessage(FShootGameplayTags::Get().UI_Toast_ResourcePickup, Message);
	}
}

void UResourceInventoryComponent::OnRep_Entries()
{
	for (const FResourceEntry& Entry : Entries)
	{
		const int32* LastPtr = LastKnownCounts.Find(Entry.ItemDef);
		const int32 OldCount = LastPtr ? *LastPtr : 0;
		if (!LastPtr || OldCount != Entry.Count)
		{
			BroadcastChange(Entry.ItemDef, OldCount, Entry.Count);
			LastKnownCounts.FindOrAdd(Entry.ItemDef) = Entry.Count;
		}
	}
}
