// Copyright ZhaoYiJie

#include "Equipment/ShootEquipmentManagerComponent.h"
#include "Equipment/ShootEquipmentDefinition.h"
#include "Equipment/ShootEquipmentInstance.h"
#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/ActorChannel.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/ShootRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootEquipmentManagerComponent)

class FLifetimeProperty;
struct FReplicationFlags;

// ============================================================================
// FShootAppliedEquipmentEntry
// ============================================================================

FString FShootAppliedEquipmentEntry::GetDebugString() const
{
	return FString::Printf(TEXT("Definition: %s, Instance: %s"),
		*GetNameSafe(EquipmentDefinition),
		*GetNameSafe(Instance));
}

// ============================================================================
// FShootEquipmentList
// ============================================================================

UShootAbilitySystemComponent* FShootEquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	AActor* OwningActor = OwnerComponent->GetOwner();
	return Cast<UShootAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
}

UShootEquipmentInstance* FShootEquipmentList::AddEntry(TSubclassOf<UShootEquipmentDefinition> EquipmentDefinition, UObject* Instigator)
{
	UShootEquipmentInstance* Result = nullptr;

	check(EquipmentDefinition != nullptr);
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());

	// 获取 Definition 的 CDO
	const UShootEquipmentDefinition* EquipmentCDO = GetDefault<UShootEquipmentDefinition>(EquipmentDefinition);

	// ========================================================================
	// 1. 创建 EquipmentInstance
	// ========================================================================

	TSubclassOf<UShootEquipmentInstance> InstanceType = EquipmentCDO->InstanceType;
	if (InstanceType == nullptr)
	{
		InstanceType = UShootEquipmentInstance::StaticClass();
	}

	// 重要：Outer 必须是 Pawn（SubObject 复制要求）
	AActor* OwnerActor = OwnerComponent->GetOwner();
	UShootEquipmentInstance* NewInstance = NewObject<UShootEquipmentInstance>(OwnerActor, InstanceType);

	if (Instigator)
	{
		NewInstance->SetInstigator(Instigator);
	}

	// ========================================================================
	// 2. 生成 SpawnedActors
	// ========================================================================

	NewInstance->SpawnEquipmentActors(EquipmentCDO->ActorsToSpawn);

	// ========================================================================
	// 3. 授予 AbilitySets
	// ========================================================================

	FShootAppliedEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.EquipmentDefinition = EquipmentDefinition;
	NewEntry.Instance = NewInstance;

	if (UShootAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		for (const TObjectPtr<const UShootAbilitySet>& AbilitySet : EquipmentCDO->AbilitySetsToGrant)
		{
			if (AbilitySet)
			{
				// 重要：SourceObject 设置为 Instance（Ability 可以访问武器实例）
				AbilitySet->GiveToAbilitySystem(ASC, &NewEntry.GrantedHandles, NewInstance);
			}
		}
	}

	// ========================================================================
	// 4. 调用 OnEquipped
	// ========================================================================

	NewInstance->OnEquipped();

	Result = NewInstance;

	// ========================================================================
	// 5. 标记为脏，触发网络复制
	// ========================================================================

	MarkItemDirty(NewEntry);

	return Result;
}

void FShootEquipmentList::RemoveEntry(UShootEquipmentInstance* Instance)
{
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());

	// 查找 Entry
	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
	{
		FShootAppliedEquipmentEntry& Entry = Entries[EntryIndex];
		if (Entry.Instance == Instance)
		{
			// ============================================================
			// 1. 调用 OnUnequipped
			// ============================================================

			if (Instance)
			{
				Instance->OnUnequipped();
			}

			// ============================================================
			// 2. 移除 AbilitySets
			// ============================================================

			if (UShootAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				Entry.GrantedHandles.TakeFromAbilitySystem(ASC);
			}

			// ============================================================
			// 3. 销毁 SpawnedActors
			// ============================================================

			if (Instance)
			{
				Instance->DestroyEquipmentActors();
			}

			// ============================================================
			// 4. 从 Entries 移除
			// ============================================================

			Entries.RemoveAt(EntryIndex);

			// ============================================================
			// 5. 标记数组为脏，触发网络复制
			// ============================================================

			MarkArrayDirty();

			return;
		}
	}
}

void FShootEquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// 客户端：装备被移除时调用
	for (int32 Index : RemovedIndices)
	{
		const FShootAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance)
		{
			// 调用 OnUnequipped（蓝图可响应）
			Entry.Instance->OnUnequipped();

			// 销毁 SpawnedActors
			Entry.Instance->DestroyEquipmentActors();
		}
	}
}

void FShootEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	// 客户端：装备被添加时调用
	for (int32 Index : AddedIndices)
	{
		const FShootAppliedEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance)
		{
			// 注意：服务器已经生成了 SpawnedActors，客户端也需要生成
			// 但 Actor 本身会通过网络复制，所以这里不需要手动复制 Actor
			// Instance->SpawnEquipmentActors() 在服务器已经调用
			// 客户端会收到复制的 SpawnedActors 数组

			// 调用 OnEquipped（蓝图可响应）
			Entry.Instance->OnEquipped();
		}
	}
}

void FShootEquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	// 客户端：装备被修改时调用
	// 目前不执行特殊逻辑
	// 未来可能用于：
	//   - 更新武器皮肤
	//   - 更新武器附件
}

// ============================================================================
// UShootEquipmentManagerComponent
// ============================================================================

UShootEquipmentManagerComponent::UShootEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EquipmentList(this)
{
	// 启用网络复制
	SetIsReplicatedByDefault(true);
	// 与 LyraEquipmentManagerComponent 保持一致：组件注册阶段必须进入 InitializeComponent，
	// 同时构造时就绑定 FastArray Owner，避免首帧拾取早于初始化回调时触发空指针断言。
	bWantsInitializeComponent = true;

	// 不需要 Tick
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UShootEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UShootEquipmentManagerComponent, EquipmentList);
}

UShootEquipmentInstance* UShootEquipmentManagerComponent::EquipItem(TSubclassOf<UShootEquipmentDefinition> EquipmentDefinition, UObject* Instigator)
{
	UShootEquipmentInstance* Result = nullptr;

	// 仅服务器
	if (GetOwner()->HasAuthority())
	{
		Result = EquipmentList.AddEntry(EquipmentDefinition, Instigator);

		// UE5：注册 SubObject 到 RegisteredSubObjectList
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
		{
			AddReplicatedSubObject(Result);
		}
	}

	return Result;
}

void UShootEquipmentManagerComponent::UnequipItem(UShootEquipmentInstance* ItemInstance)
{
	// 仅服务器
	if (GetOwner()->HasAuthority())
	{
		// UE5：取消注册 SubObject
		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
		{
			RemoveReplicatedSubObject(ItemInstance);
		}

		EquipmentList.RemoveEntry(ItemInstance);
	}
}

bool UShootEquipmentManagerComponent::ReplicateSubobjects(UActorChannel* Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool bWroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	// UE4 旧机制：手动复制 SubObject
	for (FShootAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance)
		{
			bWroteSomething |= Channel->ReplicateSubobject(Entry.Instance, *Bunch, *RepFlags);
		}
	}

	return bWroteSomething;
}

void UShootEquipmentManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// 设置 EquipmentList 的 OwnerComponent 引用
	EquipmentList.OwnerComponent = this;
}

void UShootEquipmentManagerComponent::UninitializeComponent()
{
	// 移除所有装备
	TArray<UShootEquipmentInstance*> AllEquipmentInstances;

	for (const FShootAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		AllEquipmentInstances.Add(Entry.Instance);
	}

	for (UShootEquipmentInstance* Instance : AllEquipmentInstances)
	{
		UnequipItem(Instance);
	}

	Super::UninitializeComponent();
}

void UShootEquipmentManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// UE5：注册现有的 SubObject
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FShootAppliedEquipmentEntry& Entry : EquipmentList.Entries)
		{
			if (Entry.Instance)
			{
				AddReplicatedSubObject(Entry.Instance);
			}
		}
	}
}

UShootEquipmentInstance* UShootEquipmentManagerComponent::GetFirstInstanceOfType(TSubclassOf<UShootEquipmentInstance> InstanceType)
{
	for (FShootAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance && Entry.Instance->IsA(InstanceType))
		{
			return Entry.Instance;
		}
	}

	return nullptr;
}

TArray<UShootEquipmentInstance*> UShootEquipmentManagerComponent::GetEquipmentInstancesOfType(TSubclassOf<UShootEquipmentInstance> InstanceType) const
{
	TArray<UShootEquipmentInstance*> Results;

	for (const FShootAppliedEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance && Entry.Instance->IsA(InstanceType))
		{
			Results.Add(Entry.Instance);
		}
	}

	return Results;
}
