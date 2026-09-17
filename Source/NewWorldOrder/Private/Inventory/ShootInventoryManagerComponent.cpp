// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Inventory/ShootInventoryManagerComponent.h"

#include "Engine/ActorChannel.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/Fragments/ShootInventoryFragment_StackRules.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "ShootGameplayTags.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootInventoryManagerComponent)

class FLifetimeProperty;
struct FReplicationFlags;

//////////////////////////////////////////////////////////////////////
// FShootInventoryEntry

/**
 * 获取调试字符串
 *
 * 格式：
 *   - "Instance名称 (数量 x 定义名称)"
 *   - 例如："AK47_Instance (1 x BP_Item_AK47)"
 *
 * 用途：
 *   - 日志输出
 *   - 调试界面显示
 */
FString FShootInventoryEntry::GetDebugString() const
{
	TSubclassOf<UShootInventoryItemDefinition> ItemDef;
	if (Instance != nullptr)
	{
		ItemDef = Instance->GetItemDef();
	}

	return FString::Printf(TEXT("%s (%d x %s)"), *GetNameSafe(Instance), StackCount, *GetNameSafe(ItemDef));
}

//////////////////////////////////////////////////////////////////////
// FShootInventoryList

/**
 * PreReplicatedRemove：在客户端应用移除前调用
 *
 * 执行流程：
 *   1. 遍历被移除的索引
 *   2. 对每个 Entry，广播变化消息：OldCount -> 0
 *   3. 更新 LastObservedCount = 0
 *
 * 广播消息：
 *   - InventoryOwner：此组件
 *   - Instance：被移除的物品实例
 *   - NewCount：0（物品被完全移除）
 *   - Delta：-StackCount（负数表示减少）
 *
 * UI 响应：
 *   - 背包界面：移除物品槽
 *   - 拾取提示：显示"-10 军用合金"（如果是材料）
 *   - 任务系统：更新任务物品计数
 */
void FShootInventoryList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		FShootInventoryEntry& Stack = Entries[Index];
		NotifyOwnerItemRemoved(Stack.Instance);
		// 广播：从 StackCount 减少到 0
		BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.StackCount, /*NewCount=*/ 0);
		Stack.LastObservedCount = 0;
	}
}

/**
 * PostReplicatedAdd：在客户端添加新 Entry 后调用
 *
 * 执行流程：
 *   1. 遍历新添加的索引
 *   2. 对每个 Entry，广播变化消息：0 -> NewCount
 *   3. 更新 LastObservedCount = StackCount
 *
 * 广播消息：
 *   - InventoryOwner：此组件
 *   - Instance：新添加的物品实例
 *   - NewCount：StackCount（新物品数量）
 *   - Delta：+StackCount（正数表示增加）
 *
 * UI 响应：
 *   - 背包界面：添加新的物品槽
 *   - 拾取提示：显示"+10 军用合金"
 *   - 任务系统：更新任务物品计数
 */
void FShootInventoryList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		FShootInventoryEntry& Stack = Entries[Index];
		// 广播：从 0 增加到 StackCount
		BroadcastChangeMessage(Stack, /*OldCount=*/ 0, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
		NotifyOwnerItemAdded(Stack.Instance);
	}
}

/**
 * PostReplicatedChange：在客户端修改 Entry 后调用
 *
 * 执行流程：
 *   1. 遍历被修改的索引
 *   2. 检查 LastObservedCount 有效（不应为 INDEX_NONE）
 *   3. 对每个 Entry，广播变化消息：LastObserved -> NewCount
 *   4. 更新 LastObservedCount = StackCount
 *
 * 广播消息：
 *   - InventoryOwner：此组件
 *   - Instance：被修改的物品实例
 *   - NewCount：StackCount（新数量）
 *   - Delta：StackCount - LastObservedCount（变化量）
 *
 * UI 响应：
 *   - 背包界面：更新物品槽的数量显示
 *   - 制作界面：更新材料数量和可制作状态
 *
 * 注意：
 *   - check(Stack.LastObservedCount != INDEX_NONE)：确保 LastObservedCount 已初始化
 *   - 如果失败，说明复制钩子调用顺序有问题
 */
void FShootInventoryList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		FShootInventoryEntry& Stack = Entries[Index];
		check(Stack.LastObservedCount != INDEX_NONE);
		// 广播：从 LastObservedCount 变化到 StackCount
		BroadcastChangeMessage(Stack, /*OldCount=*/ Stack.LastObservedCount, /*NewCount=*/ Stack.StackCount);
		Stack.LastObservedCount = Stack.StackCount;
	}
}

/**
 * 广播变化消息实现
 *
 * 执行流程：
 *   1. 创建 FShootInventoryChangeMessage
 *   2. 填充消息字段：
 *      - InventoryOwner：此组件
 *      - Instance：变化的物品实例
 *      - NewCount：新数量
 *      - Delta：NewCount - OldCount
 *   3. 通过 GameplayMessageSubsystem 广播消息
 *   4. 使用 Tag：Inventory.Message.StackChanged
 *
 * GameplayMessageSubsystem：
 *   - UE 的全局消息系统
 *   - 支持基于 Tag 的消息订阅和广播
 *   - 解耦发送者和接收者
 *
 * 订阅方式：
 *   - C++：UGameplayMessageSubsystem::Get(World)->RegisterListener(Tag, Callback)
 *   - 蓝图：Add GameplayMessage Listener
 *
 * 性能：
 *   - 消息广播：O(n)，n = 监听者数量（通常很少）
 *   - 异步：消息在当前帧广播，监听者立即响应
 */
void FShootInventoryList::BroadcastChangeMessage(FShootInventoryEntry& Entry, int32 OldCount, int32 NewCount)
{
	// 创建消息结构体
	FShootInventoryChangeMessage Message;
	Message.InventoryOwner = OwnerComponent;
	Message.Instance = Entry.Instance;
	Message.NewCount = NewCount;
	Message.Delta = NewCount - OldCount;

	// 获取 GameplayMessageSubsystem 并广播消息
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(OwnerComponent->GetWorld());
	MessageSystem.BroadcastMessage(FShootGameplayTags::Get().Inventory_Message_StackChanged, Message);
}

void FShootInventoryList::NotifyOwnerItemAdded(UShootInventoryItemInstance* Instance) const
{
	if (!OwnerComponent || !Instance)
	{
		return;
	}

	if (UShootInventoryManagerComponent* Manager = Cast<UShootInventoryManagerComponent>(OwnerComponent))
	{
		Manager->HandleItemInstanceAdded(Instance);
	}
}

void FShootInventoryList::NotifyOwnerItemRemoved(UShootInventoryItemInstance* Instance) const
{
	if (!OwnerComponent || !Instance)
	{
		return;
	}

	if (UShootInventoryManagerComponent* Manager = Cast<UShootInventoryManagerComponent>(OwnerComponent))
	{
		Manager->HandleItemInstanceRemoved(Instance);
	}
}

/**
 * 添加物品（通过 Definition 创建 Instance）实现
 *
 * 执行流程：
 *   1. 检查 ItemDef 和 OwnerComponent 不为 nullptr
 *   2. 检查权威性（HasAuthority）
 *   3. 创建新的 Entry（AddDefaulted_GetRef）
 *   4. 创建 Instance：NewObject<UShootInventoryItemInstance>(Actor)
 *      - Outer 必须是 Actor，不能是 Component（UE-127172）
 *   5. 设置 ItemDef：Instance->SetItemDef(ItemDef)
 *   6. 遍历所有 Fragment，调用 OnInstanceCreated(Instance)
 *      - Fragment 可以初始化 Instance 的 StatTags
 *      - 例如：WeaponFragment 设置初始弹药数量
 *   7. 设置 StackCount
 *   8. MarkItemDirty(NewEntry)：标记为脏，触发网络复制
 *   9. 返回 Instance
 *
 * 为什么 Outer 是 Actor：
 *   - UE 限制：SubObject 的 Outer 必须是被复制的 Actor
 *   - 如果 Outer 是 Component，网络复制会失败
 *   - 参考：UE-127172（Epic 的 Bug 报告）
 *
 * Fragment 初始化：
 *   - Definition 包含多个 Fragment
 *   - 每个 Fragment 可以扩展物品功能
 *   - OnInstanceCreated：Fragment 在 Instance 创建时的初始化钩子
 *
 * 网络复制：
 *   - MarkItemDirty：标记 Entry 为脏
 *   - FFastArraySerializer 检测变化
 *   - 下次网络更新时，传输增量数据到客户端
 *   - 客户端调用 PostReplicatedAdd
 */
UShootInventoryItemInstance* FShootInventoryList::AddEntry(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount, const FShootInventoryItemInitData& InitData)
{
	UShootInventoryItemInstance* Result = nullptr;

	// 验证参数
	check(ItemDef != nullptr);
	check(OwnerComponent);

	AActor* OwningActor = OwnerComponent->GetOwner();
	check(OwningActor->HasAuthority());

	// 创建新的 Entry
	FShootInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();

	// 创建 Instance（Outer 必须是 Actor，不能是 Component）
	// @TODO: 使用 Actor 而非 Component 作为 Outer，因为 UE-127172
	NewEntry.Instance = NewObject<UShootInventoryItemInstance>(OwnerComponent->GetOwner());

	// 设置 ItemDef
	NewEntry.Instance->SetItemDef(ItemDef);
	NewEntry.Instance->InitializeInstanceLifecycle(InitData.Lifetime, InitData.ResolveInstanceId());

	// 遍历所有 Fragment，调用 OnInstanceCreated 初始化 Instance
	for (UShootInventoryItemFragment* Fragment : GetDefault<UShootInventoryItemDefinition>(ItemDef)->Fragments)
	{
		if (Fragment != nullptr)
		{
			Fragment->OnInstanceCreated(NewEntry.Instance);
		}
	}

	bool bHasSnapshot = false;
	InitData.InitialStatTags.ForEachStack([&bHasSnapshot](FGameplayTag Tag, int32 Count)
	{
		if (Count > 0)
		{
			bHasSnapshot = true;
		}
	});

	if (bHasSnapshot)
	{
		NewEntry.Instance->ApplyStatTagSnapshot(InitData.InitialStatTags);
	}

	// 设置 StackCount
	NewEntry.StackCount = StackCount;
	Result = NewEntry.Instance;

	// 标记为脏，触发网络复制
	MarkItemDirty(NewEntry);
	NotifyOwnerItemAdded(NewEntry.Instance);

	return Result;
}

/**
 * 添加物品实例（直接添加已存在的 Instance）实现
 *
 * 当前状态：
 *   - 未实现（unimplemented）
 *
 * 用途：
 *   - 从其他容器转移物品
 *   - 从存档加载物品
 *
 * @TODO: 实现此函数
 *   1. 验证 Instance 不为 nullptr
 *   2. 创建新的 Entry
 *   3. 设置 Entry.Instance = Instance
 *   4. 设置 StackCount（默认 1 或从 Instance 获取）
 *   5. MarkItemDirty
 */
void FShootInventoryList::AddEntry(UShootInventoryItemInstance* Instance)
{
	if (!Instance || !OwnerComponent)
	{
		return;
	}

	AActor* OwningActor = OwnerComponent->GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return;
	}

	if (!Instance->GetItemDef())
	{
		return;
	}

	for (const FShootInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance == Instance)
		{
			return;
		}
	}

	FShootInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = Instance;
	// 直接添加已存在的 Instance 时，默认以 1 作为 StackCount
	NewEntry.StackCount = 1;
	MarkItemDirty(NewEntry);
	NotifyOwnerItemAdded(Instance);
}

/**
 * 移除物品实例实现
 *
 * 执行流程：
 *   1. 遍历 Entries（使用迭代器，因为需要移除）
 *   2. 查找匹配的 Instance
 *   3. 移除 Entry（RemoveCurrent）
 *   4. MarkArrayDirty：标记整个数组为脏，触发网络复制
 *
 * 网络复制：
 *   - MarkArrayDirty：标记数组为脏
 *   - FFastArraySerializer 检测到 Entry 被移除
 *   - 下次网络更新时，传输移除的增量数据
 *   - 客户端调用 PreReplicatedRemove
 *
 * 注意：
 *   - 仅移除 Entry，不销毁 Instance
 *   - ManagerComponent 需要调用 RemoveReplicatedSubObject 取消注册
 *   - Instance 会在没有引用时由 GC 销毁
 */
void FShootInventoryList::RemoveEntry(UShootInventoryItemInstance* Instance)
{
	for (auto EntryIt = Entries.CreateIterator(); EntryIt; ++EntryIt)
	{
		FShootInventoryEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			NotifyOwnerItemRemoved(Instance);
			// 移除 Entry
			EntryIt.RemoveCurrent();
			// 标记整个数组为脏（因为移除了项）
			MarkArrayDirty();
		}
	}
}

/**
 * 获取所有物品实例实现
 *
 * 执行流程：
 *   1. 创建结果数组，预留空间（Reserve）
 *   2. 遍历所有 Entry
 *   3. 对每个非空的 Instance，添加到结果数组
 *   4. 返回结果数组
 *
 * 性能：
 *   - O(n)，n = Entry 数量
 *   - Reserve：预先分配内存，避免多次重新分配
 *
 * 注意：
 *   - 创建新数组，有内存分配
 *   - 如果频繁调用，考虑缓存结果
 *
 * @TODO: 是否应该在更深层隐藏 nullptr 检查？
 *   - 当前：调用者需要处理 nullptr
 *   - 改进：InventoryList 内部保证 Instance 不为 nullptr
 */
TArray<UShootInventoryItemInstance*> FShootInventoryList::GetAllItems() const
{
	TArray<UShootInventoryItemInstance*> Results;
	Results.Reserve(Entries.Num());
	for (const FShootInventoryEntry& Entry : Entries)
	{
		if (Entry.Instance != nullptr) //@TODO: 是否应该在更深层隐藏此检查？
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}

//////////////////////////////////////////////////////////////////////
// UShootInventoryManagerComponent

/**
 * 构造函数
 *
 * 初始化：
 *   1. InventoryList(this)：传递 this 作为 OwnerComponent
 *      - InventoryList 需要 OwnerComponent 来获取 World、广播消息等
 *   2. SetIsReplicatedByDefault(true)：启用网络复制
 *
 * 注意：
 *   - InventoryList 是值类型成员，在构造函数初始化列表中初始化
 *   - this 指针在构造函数体执行前已有效，可以传递给 InventoryList
 */
UShootInventoryManagerComponent::UShootInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, InventoryList(this)  // 传递 this 作为 OwnerComponent
{
	// 启用网络复制
	SetIsReplicatedByDefault(true);
}

/**
 * 注册复制属性
 *
 * 作用：
 *   - 告知 UE 网络系统哪些属性需要复制
 *   - DOREPLIFETIME：注册无条件复制（所有客户端都接收）
 *
 * 复制属性：
 *   - InventoryList：库存列表
 *     - FastArray 增量复制，仅传输变化的 Entry
 *     - 客户端接收后，通过复制钩子更新并广播消息
 *
 * 条件复制：
 *   - 当前使用无条件复制（DOREPLIFETIME）
 *   - 可优化为仅复制给拥有者（DOREPLIFETIME_CONDITION(ThisClass, InventoryList, COND_OwnerOnly)）
 */
void UShootInventoryManagerComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, InventoryList);
}

/**
 * 检查是否可以添加物品实现
 *
 * 当前状态：
 *   - 始终返回 true（未实现检查逻辑）
 *
 * @TODO: 添加以下检查：
 *   1. 堆栈上限检查：
 *      - Fragment 定义 MaxStackCount
 *      - 检查当前 StackCount + 新增数量 <= MaxStackCount
 *   2. 唯一性检查：
 *      - Fragment 定义 bUnique
 *      - 检查是否已拥有该物品（徽章、设计图等）
 *   3. 容量检查：
 *      - 如果有背包容量限制，检查是否有空间
 *   4. 权限检查：
 *      - 检查是否有权限拥有该物品（如等级限制）
 */
bool UShootInventoryManagerComponent::CanAddItemDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount)
{
	if (!ItemDef || StackCount <= 0)
	{
		return false;
	}

	const AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}

	if (MaxEntryCount > 0 && InventoryList.Entries.Num() >= MaxEntryCount)
	{
		return false;
	}

	const UShootInventoryItemDefinition* ItemCDO = GetDefault<UShootInventoryItemDefinition>(ItemDef);
	if (ItemCDO)
	{
		const UShootInventoryFragment_StackRules* StackRules = Cast<UShootInventoryFragment_StackRules>(
			ItemCDO->FindFragmentByClass(UShootInventoryFragment_StackRules::StaticClass()));
		if (StackRules)
		{
			if (StackRules->bUnique && FindFirstItemStackByDefinition(ItemDef))
			{
				return false;
			}

			if (StackRules->MaxStackCount > 0 && StackCount > StackRules->MaxStackCount)
			{
				return false;
			}
		}
	}

	return true;
}

/**
 * 添加物品（通过 Definition 创建 Instance）实现
 *
 * 执行流程：
 *   1. 验证 ItemDef 不为 nullptr
 *   2. 调用 InventoryList.AddEntry(ItemDef, StackCount)
 *      - 创建 Instance
 *      - 设置 ItemDef
 *      - 调用 Fragment 初始化
 *      - MarkItemDirty 触发复制
 *   3. 如果使用 RegisteredSubObjectList 且已准备好复制：
 *      - 调用 AddReplicatedSubObject(Result)
 *      - 注册 Instance 到 SubObject 列表
 *   4. 返回 Instance
 *
 * SubObject 复制机制：
 *   - IsUsingRegisteredSubObjectList：检查是否使用新机制（UE5）
 *   - IsReadyForReplication：检查是否已准备好复制
 *   - AddReplicatedSubObject：注册 Instance 到 SubObject 列表
 *
 * 为什么需要 SubObject 复制：
 *   - InventoryList 复制 Entry（包含 Instance 指针）
 *   - 但 Instance 本身是 UObject，需要单独复制
 *   - SubObject 机制：自动复制 UObject 及其属性
 */
UShootInventoryItemInstance* UShootInventoryManagerComponent::AddItemDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount)
{
	return AddPersistentItem(ItemDef, StackCount);
}

UShootInventoryItemInstance* UShootInventoryManagerComponent::AddPersistentItem(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount)
{
	FShootInventoryItemInitData InitData;
	InitData.Lifetime = EShootItemLifetime::Persistent;
	return AddItemDefinitionInternal(ItemDef, StackCount, InitData);
}

UShootInventoryItemInstance* UShootInventoryManagerComponent::AddRuntimeItem(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount)
{
	FShootInventoryItemInitData InitData;
	InitData.Lifetime = EShootItemLifetime::RuntimeOnly;
	return AddItemDefinitionInternal(ItemDef, StackCount, InitData);
}

UShootInventoryItemInstance* UShootInventoryManagerComponent::AddRuntimeWeaponItem(TSubclassOf<UShootInventoryItemDefinition> ItemDef, const FShootInventoryItemInitData& InitData)
{
	FShootInventoryItemInitData RuntimeInit = InitData;
	if (RuntimeInit.Lifetime != EShootItemLifetime::RuntimeOnly)
	{
		return AddItemDefinitionInternal(ItemDef, /*StackCount=*/1, RuntimeInit);
	}

	RuntimeInit.Lifetime = EShootItemLifetime::RuntimeOnly;
	if (!RuntimeInit.DesiredInstanceId.IsValid())
	{
		RuntimeInit.DesiredInstanceId = FGuid::NewGuid();
	}
	return AddItemDefinitionInternal(ItemDef, /*StackCount=*/1, RuntimeInit);
}

UShootInventoryItemInstance* UShootInventoryManagerComponent::CreateRuntimeWeaponFromPersistent(
	const UShootInventoryItemInstance* PersistentSource, bool bCopyStatSnapshot)
{
	if (!PersistentSource || PersistentSource->GetItemLifetime() != EShootItemLifetime::Persistent)
	{
		return nullptr;
	}

	const TSubclassOf<UShootInventoryItemDefinition> ItemDef = PersistentSource->GetItemDef();
	if (!ItemDef)
	{
		return nullptr;
	}

	FShootInventoryItemInitData RuntimeInit;
	RuntimeInit.Lifetime = EShootItemLifetime::RuntimeOnly;
	if (bCopyStatSnapshot)
	{
		RuntimeInit.InitialStatTags = PersistentSource->GetStatTags();
	}

	return AddRuntimeWeaponItem(ItemDef, RuntimeInit);
}

/**
 * 添加物品实例（直接添加已存在的 Instance）实现
 *
 * 执行流程：
 *   1. 调用 InventoryList.AddEntry(ItemInstance)
 *   2. 如果使用 RegisteredSubObjectList 且已准备好复制：
 *      - 调用 AddReplicatedSubObject(ItemInstance)
 *
 * 注意：
 *   - InventoryList.AddEntry(Instance) 当前未实现（unimplemented）
 *   - 需要时再实现
 */
void UShootInventoryManagerComponent::AddItemInstance(UShootInventoryItemInstance* ItemInstance)
{
	InventoryList.AddEntry(ItemInstance);
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && ItemInstance)
	{
		AddReplicatedSubObject(ItemInstance);
	}
}

/**
 * 移除物品实例实现
 *
 * 执行流程：
 *   1. 调用 InventoryList.RemoveEntry(ItemInstance)
 *      - 移除 Entry
 *      - MarkArrayDirty 触发复制
 *   2. 如果使用 RegisteredSubObjectList：
 *      - 调用 RemoveReplicatedSubObject(ItemInstance)
 *      - 取消注册 Instance
 *
 * SubObject 取消注册：
 *   - RemoveReplicatedSubObject：从 SubObject 列表中移除
 *   - Instance 不再自动复制
 *   - Instance 会在没有引用时由 GC 销毁
 */
void UShootInventoryManagerComponent::RemoveItemInstance(UShootInventoryItemInstance* ItemInstance)
{
	InventoryList.RemoveEntry(ItemInstance);

	// 取消注册 SubObject
	if (ItemInstance && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(ItemInstance);
	}
}

/**
 * 获取所有物品实例实现
 *
 * 执行流程：
 *   - 直接调用 InventoryList.GetAllItems()
 *
 * 返回：
 *   - 所有非空的 Instance 数组
 */
TArray<UShootInventoryItemInstance*> UShootInventoryManagerComponent::GetAllItems() const
{
	return InventoryList.GetAllItems();
}

/**
 * 查找第一个匹配定义的物品实现
 *
 * 执行流程：
 *   1. 遍历 InventoryList.Entries
 *   2. 对每个 Entry：
 *      - 验证 Instance 有效（IsValid）
 *      - 检查 Instance->GetItemDef() == ItemDef
 *      - 如果匹配，返回 Instance
 *   3. 如果未找到，返回 nullptr
 *
 * 性能：
 *   - O(n)，n = Entry 数量
 *   - 返回第一个匹配项（如果有多个，只返回第一个）
 *
 * 使用场景：
 *   - 检查是否拥有某种物品
 *   - 获取物品实例以查询 StatTags
 */
UShootInventoryItemInstance* UShootInventoryManagerComponent::FindFirstItemStackByDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef) const
{
	for (const FShootInventoryEntry& Entry : InventoryList.Entries)
	{
		UShootInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				return Instance;
			}
		}
	}

	return nullptr;
}

/**
 * 获取指定定义的物品总数实现
 *
 * 执行流程：
 *   1. 初始化 TotalCount = 0
 *   2. 遍历 InventoryList.Entries
 *   3. 对每个 Entry：
 *      - 验证 Instance 有效
 *      - 检查 Instance->GetItemDef() == ItemDef
 *      - 如果匹配，TotalCount++
 *   4. 返回 TotalCount
 *
 * 注意：
 *   - 返回 Entry 数量，不是 StackCount 总和
 *   - 如果需要 StackCount 总和，需要累加 Entry.StackCount
 *
 * 性能：
 *   - O(n)，n = Entry 数量
 */
int32 UShootInventoryManagerComponent::GetTotalItemCountByDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef) const
{
	int32 TotalCount = 0;
	for (const FShootInventoryEntry& Entry : InventoryList.Entries)
	{
		UShootInventoryItemInstance* Instance = Entry.Instance;

		if (IsValid(Instance))
		{
			if (Instance->GetItemDef() == ItemDef)
			{
				++TotalCount;
			}
		}
	}

	return TotalCount;
}

/**
 * 消耗指定定义的物品实现
 *
 * 执行流程：
 *   1. 检查权威性（HasAuthority）
 *   2. 初始化 TotalConsumed = 0
 *   3. 循环直到 TotalConsumed >= NumToConsume：
 *      - 查找第一个匹配的 Instance（FindFirstItemStackByDefinition）
 *      - 如果找到：
 *        - 移除 Entry（InventoryList.RemoveEntry）
 *        - TotalConsumed++
 *      - 如果未找到：
 *        - 返回 false（数量不足）
 *   4. 返回 true（成功消耗）
 *
 * 注意：
 *   - 当前实现：每次移除整个 Entry（不支持部分消耗）
 *   - 如果 Entry.StackCount > 1，会一次性全部消耗
 *   - 对于材料物品，通常 StackCount = 1，StatTags 存储实际数量
 *
 * @TODO: 优化性能
 *   - 当前：O(n^2)，每次消耗都遍历一次
 *   - 改进：使用加速结构（如 TMap<ItemDef, TArray<Instance>>）
 *
 * @TODO: 支持部分消耗
 *   - 当前：移除整个 Entry
 *   - 改进：减少 StackCount，如果 <= 0 再移除
 */
bool UShootInventoryManagerComponent::ConsumeItemsByDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 NumToConsume)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return false;
	}

	if (!ItemDef || NumToConsume <= 0)
	{
		return false;
	}

	int32 TotalAvailable = 0;
	for (const FShootInventoryEntry& Entry : InventoryList.Entries)
	{
		if (IsValid(Entry.Instance) && Entry.Instance->GetItemDef() == ItemDef)
		{
			TotalAvailable += FMath::Max(0, Entry.StackCount);
		}
	}

	if (TotalAvailable < NumToConsume)
	{
		return false;
	}

	int32 Remaining = NumToConsume;
	TArray<UShootInventoryItemInstance*> InstancesToRemove;
	for (FShootInventoryEntry& Entry : InventoryList.Entries)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (!IsValid(Entry.Instance) || Entry.Instance->GetItemDef() != ItemDef)
		{
			continue;
		}

		const int32 Available = FMath::Max(0, Entry.StackCount);
		if (Available <= 0)
		{
			continue;
		}

		if (Available <= Remaining)
		{
			Remaining -= Available;
			InstancesToRemove.Add(Entry.Instance);
		}
		else
		{
			Entry.StackCount -= Remaining;
			Remaining = 0;
			InventoryList.MarkItemDirty(Entry);
		}
	}

	for (UShootInventoryItemInstance* Instance : InstancesToRemove)
	{
		RemoveItemInstance(Instance);
	}

	return Remaining <= 0;
}

void UShootInventoryManagerComponent::BuildPersistentItemsSaveData(TArray<FSavedInventoryItem>& OutItems) const
{
	OutItems.Reset();

	const AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return;
	}

	for (const FShootInventoryEntry& Entry : InventoryList.Entries)
	{
		UShootInventoryItemInstance* Instance = Entry.Instance;
		if (!IsValid(Instance) || Instance->GetItemLifetime() != EShootItemLifetime::Persistent)
		{
			continue;
		}

		FSavedInventoryItem SavedItem;
		SavedItem.ItemInstanceId = Instance->GetItemInstanceId();
		SavedItem.ItemDefinition = Instance->GetItemDef();
		SavedItem.StackCount = Entry.StackCount;

		Instance->GetStatTags().ForEachStack(
			[&SavedItem](const FGameplayTag& Tag, int32 Count)
			{
				if (Tag.IsValid() && Count > 0)
				{
					FSavedTagStack Stack;
					Stack.Tag = Tag;
					Stack.Count = Count;
					SavedItem.TagStacks.Add(Stack);
				}
			});

		OutItems.Add(SavedItem);
	}
}

void UShootInventoryManagerComponent::ApplyPersistentItemsSaveData(const TArray<FSavedInventoryItem>& InItems)
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return;
	}

	// Remove existing persistent items
	const TArray<UShootInventoryItemInstance*> ExistingItems = GetAllItems();
	for (UShootInventoryItemInstance* Instance : ExistingItems)
	{
		if (Instance && Instance->GetItemLifetime() == EShootItemLifetime::Persistent)
		{
			RemoveItemInstance(Instance);
		}
	}

	for (const FSavedInventoryItem& SavedItem : InItems)
	{
		if (!SavedItem.ItemDefinition.IsValid())
		{
			continue;
		}

		TSubclassOf<UShootInventoryItemDefinition> DefinitionClass = SavedItem.ItemDefinition.LoadSynchronous();
		if (!DefinitionClass)
		{
			continue;
		}

		FShootInventoryItemInitData InitData;
		InitData.Lifetime = EShootItemLifetime::Persistent;
		InitData.DesiredInstanceId = SavedItem.ItemInstanceId;

		UShootInventoryItemInstance* NewInstance = AddItemDefinitionInternal(DefinitionClass, SavedItem.StackCount, InitData);
		if (!NewInstance)
		{
			continue;
		}

		for (const FSavedTagStack& TagStack : SavedItem.TagStacks)
		{
			if (!TagStack.Tag.IsValid() || TagStack.Count <= 0)
			{
				continue;
			}

			const int32 ExistingCount = NewInstance->GetStatTagStackCount(TagStack.Tag);
			if (ExistingCount > 0)
			{
				NewInstance->RemoveStatTagStack(TagStack.Tag, ExistingCount);
			}

			NewInstance->AddStatTagStack(TagStack.Tag, TagStack.Count);
		}
	}
}

/**
 * 准备复制（注册已存在的 SubObject）实现
 *
 * 时机：
 *   - 在组件开始复制前调用
 *   - 通常在 Actor BeginPlay 或网络生成时调用
 *
 * 作用：
 *   - 注册所有已存在的 ItemInstance 到 RegisteredSubObjectList
 *   - 确保这些 Instance 会被复制
 *
 * 执行流程：
 *   1. 调用 Super::ReadyForReplication()
 *   2. 如果使用 RegisteredSubObjectList：
 *      - 遍历 InventoryList.Entries
 *      - 对每个有效的 Instance，调用 AddReplicatedSubObject(Instance)
 *
 * 使用场景：
 *   - 从存档加载：Instance 已存在，需要注册到 SubObject 列表
 *   - 客户端连接：需要将所有 Instance 复制给新客户端
 */
void UShootInventoryManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();

	// 注册所有已存在的 UShootInventoryItemInstance
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FShootInventoryEntry& Entry : InventoryList.Entries)
		{
			UShootInventoryItemInstance* Instance = Entry.Instance;

			if (IsValid(Instance))
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
}

/**
 * 手动复制子对象（旧机制，作为后备）实现
 *
 * 作用：
 *   - 在不支持 RegisteredSubObjectList 的 UE 版本中使用
 *   - 手动复制所有 ItemInstance
 *
 * 执行流程：
 *   1. 调用 Super::ReplicateSubobjects
 *   2. 遍历 InventoryList.Entries
 *   3. 对每个有效的 Instance：
 *      - 调用 Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags)
 *      - 将 Instance 序列化到网络包
 *   4. 返回是否写入了数据
 *
 * 新旧机制对比：
 *   - 新机制（RegisteredSubObjectList）：
 *     - 自动管理 SubObject 列表
 *     - 更高效，避免重复检查
 *   - 旧机制（ReplicateSubobjects）：
 *     - 每次复制都遍历所有 Instance
 *     - 作为后备，确保兼容性
 *
 * 返回值：
 *   - true：写入了数据
 *   - false：没有写入数据（所有 Instance 都已同步）
 */
bool UShootInventoryManagerComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (FShootInventoryEntry& Entry : InventoryList.Entries)
	{
		UShootInventoryItemInstance* Instance = Entry.Instance;

		if (Instance && IsValid(Instance))
		{
			WroteSomething |= Channel->ReplicateSubobject(Instance, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

UShootInventoryItemInstance* UShootInventoryManagerComponent::AddItemDefinitionInternal(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount, const FShootInventoryItemInitData& InitData)
{
	UShootInventoryItemInstance* Result = nullptr;
	if (ItemDef != nullptr)
	{
		if (!CanAddItemDefinition(ItemDef, StackCount))
		{
			return nullptr;
		}

		Result = InventoryList.AddEntry(ItemDef, StackCount, InitData);

		if (IsUsingRegisteredSubObjectList() && IsReadyForReplication() && Result)
		{
			AddReplicatedSubObject(Result);
		}
	}
	return Result;
}

void UShootInventoryManagerComponent::RemoveRuntimeItems()
{
	AActor* OwningActor = GetOwner();
	if (!OwningActor || !OwningActor->HasAuthority())
	{
		return;
	}

	TArray<FGuid> RuntimeIds;
	RuntimeIds.Reserve(InstanceMap.Num());

	for (const TPair<FGuid, TWeakObjectPtr<UShootInventoryItemInstance>>& Pair : InstanceMap)
	{
		if (UShootInventoryItemInstance* Instance = Pair.Value.Get())
		{
			if (Instance->GetItemLifetime() == EShootItemLifetime::RuntimeOnly)
			{
				RuntimeIds.Add(Pair.Key);
			}
		}
	}

	for (const FGuid& InstanceId : RuntimeIds)
	{
		RemoveItemByInstanceId(InstanceId);
	}
}

UShootInventoryItemInstance* UShootInventoryManagerComponent::FindItemByInstanceId(FGuid ItemInstanceId) const
{
	if (!ItemInstanceId.IsValid())
	{
		return nullptr;
	}

	if (const TWeakObjectPtr<UShootInventoryItemInstance>* Found = InstanceMap.Find(ItemInstanceId))
	{
		return Found->Get();
	}

	return nullptr;
}

bool UShootInventoryManagerComponent::RemoveItemByInstanceId(FGuid ItemInstanceId)
{
	if (UShootInventoryItemInstance* Instance = FindItemByInstanceId(ItemInstanceId))
	{
		RemoveItemInstance(Instance);
		return true;
	}
	return false;
}

void UShootInventoryManagerComponent::HandleItemInstanceAdded(UShootInventoryItemInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	InstanceMap.FindOrAdd(Instance->GetItemInstanceId()) = Instance;
}

void UShootInventoryManagerComponent::HandleItemInstanceRemoved(UShootInventoryItemInstance* Instance)
{
	if (!Instance)
	{
		return;
	}

	const FGuid InstanceId = Instance->GetItemInstanceId();
	if (InstanceId.IsValid())
	{
		InstanceMap.Remove(InstanceId);
	}
}
