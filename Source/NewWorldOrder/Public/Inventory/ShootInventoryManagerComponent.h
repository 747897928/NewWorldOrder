// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Inventory/SavedInventoryTypes.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "Misc/Guid.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "ShootInventoryManagerComponent.generated.h"

class UShootInventoryItemDefinition;
class UShootInventoryItemInstance;
class UShootInventoryManagerComponent;
class UObject;
struct FFrame;
struct FShootInventoryList;
struct FNetDeltaSerializeInfo;
struct FReplicationFlags;

USTRUCT(BlueprintType)
struct FShootInventoryItemInitData
{
	GENERATED_BODY()

	/** 物品生命周期（Persistent=账号资产，RuntimeOnly=副本临时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Inventory)
	EShootItemLifetime Lifetime = EShootItemLifetime::Persistent;

	/** 指定实例 Guid（可选），用于从存档恢复或 QuickBar 绑定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Inventory)
	FGuid DesiredInstanceId;

	/** 初始 StatTags（用于掉落武器还原弹药/耐久等状态） */
	UPROPERTY()
	FShootGameplayTagStackContainer InitialStatTags;

	FGuid ResolveInstanceId() const
	{
	 return DesiredInstanceId.IsValid() ? DesiredInstanceId : FGuid::NewGuid();
	}
};

/**
 * FShootInventoryChangeMessage
 *
 * 【库存变化消息】通过 GameplayMessageSubsystem 广播的库存变化事件
 *
 * 设计目的：
 *   - 解耦数据层与 UI 层：数据变化时，通过消息通知 UI
 *   - 支持多个监听者：多个 UI Widget 可以监听同一个消息
 *   - 避免轮询：UI 不需要每帧检查库存，而是被动接收消息
 *
 * 广播时机：
 *   - 添加物品：PreReplicatedAdd -> BroadcastChangeMessage(0 -> NewCount)
 *   - 移除物品：PreReplicatedRemove -> BroadcastChangeMessage(OldCount -> 0)
 *   - 修改堆栈：PostReplicatedChange -> BroadcastChangeMessage(LastObserved -> NewCount)
 *
 * 消息 Tag：
 *   - Inventory.Message.StackChanged（在 GameplayTags 中定义）
 *
 * 使用场景：
 *   - 背包 UI：监听消息，更新物品显示
 *   - 制作 UI：监听消息，更新材料数量和可制作状态
 *   - 拾取提示：监听消息，显示"+10 军用合金"
 *   - 任务系统：监听消息，检查任务物品是否收集完成
 */
USTRUCT(BlueprintType)
struct FShootInventoryChangeMessage
{
	GENERATED_BODY()

	/**
	 * 库存拥有组件
	 *
	 * 用途：
	 *   - 标识哪个角色/容器的库存发生了变化
	 *   - UI 可以过滤只关心玩家自己的库存变化
	 *
	 * @TODO: 是否改为基于 Tag 的名称+拥有 Actor，而非直接暴露组件？
	 *   - 优点：更抽象，支持多个库存容器（背包、仓库、商店等）
	 *   - 缺点：增加复杂度
	 */
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<UActorComponent> InventoryOwner = nullptr;

	/**
	 * 物品实例
	 *
	 * 用途：
	 *   - 标识哪个物品发生了变化
	 *   - UI 可以查询 Instance 的 ItemDef、Fragment、StatTags 等信息
	 */
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<UShootInventoryItemInstance> Instance = nullptr;

	/**
	 * 新数量
	 *
	 * 用途：
	 *   - 变化后的堆栈数量
	 *   - 0 表示物品被完全移除
	 */
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 NewCount = 0;

	/**
	 * 变化量
	 *
	 * 用途：
	 *   - Delta = NewCount - OldCount
	 *   - 正数表示增加，负数表示减少
	 *   - UI 可以显示"+10"或"-5"
	 */
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 Delta = 0;
};

/**
 * FShootInventoryEntry
 *
 * 【库存条目】单个库存槽位，包含物品实例和堆栈数量
 *
 * 设计目的：
 *   - 作为 FastArraySerializerItem，支持增量网络复制
 *   - 存储物品实例和堆栈数量
 *   - 跟踪变化（LastObservedCount）以触发消息广播
 *
 * 关键字段：
 *   1. Instance：物品实例（UObject，SubObject 复制）
 *   2. StackCount：堆栈数量（Entry 的数量，通常为 1）
 *   3. LastObservedCount：上次观察到的数量（不复制，用于检测变化）
 *
 * StackCount vs StatTags：
 *   - StackCount：表示"这个 Entry 本身的数量"（通常为 1）
 *   - StatTags：表示"物品实例内部的堆栈数据"（如弹药、材料）
 *
 * 示例：
 *   - Entry1: Instance=AK47_1, StackCount=1, StatTags={Inventory.Ammo.Rifle:120}
 *   - Entry2: Instance=MilitaryAlloy_1, StackCount=1, StatTags={Inventory.Material.MilitaryAlloy:50}
 *
 * LastObservedCount 作用：
 *   - 在复制钩子中检测变化
 *   - PostReplicatedChange：如果 StackCount != LastObservedCount，广播消息
 *   - NotReplicated：只在本地使用，不网络传输
 */
USTRUCT(BlueprintType)
struct FShootInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FShootInventoryEntry()
	{}

	// 调试用：返回 "Instance名称 (数量 x 定义名称)" 格式字符串
	FString GetDebugString() const;

private:
	/**
	 * friend 声明：允许 InventoryList 和 ManagerComponent 访问私有成员
	 *
	 * 作用：
	 *   - InventoryList 需要操作 Entry 的所有字段
	 *   - ManagerComponent 需要遍历 Entry
	 *   - 保持封装性，其他类无法访问
	 */
	friend FShootInventoryList;
	friend UShootInventoryManagerComponent;

	/**
	 * 物品实例
	 *
	 * 用途：
	 *   - 指向实际的物品实例（UObject）
	 *   - 通过 SubObject 复制机制复制到客户端
	 *
	 * 注意：
	 *   - Outer 必须是 Actor，不能是 Component（UE-127172）
	 *   - 必须通过 AddReplicatedSubObject 注册
	 */
	UPROPERTY()
	TObjectPtr<UShootInventoryItemInstance> Instance = nullptr;

	/**
	 * 堆栈数量
	 *
	 * 用途：
	 *   - 表示"这个 Entry 的数量"（通常为 1）
	 *   - 与 StatTags 不同，StatTags 是 Instance 内部的堆栈数据
	 *
	 * 复制：
	 *   - 自动复制到客户端
	 *   - FastArray 增量复制，仅传输变化
	 */
	UPROPERTY()
	int32 StackCount = 0;

	/**
	 * 上次观察到的数量（不复制）
	 *
	 * 用途：
	 *   - 在复制钩子中检测变化
	 *   - PostReplicatedChange：计算 Delta = StackCount - LastObservedCount
	 *   - 广播消息后，更新为当前 StackCount
	 *
	 * 为什么不复制：
	 *   - 仅本地使用，用于跟踪客户端的观察状态
	 *   - 服务器不需要知道客户端的 LastObservedCount
	 */
	UPROPERTY(NotReplicated)
	int32 LastObservedCount = INDEX_NONE;
};

/**
 * FShootInventoryList
 *
 * 【库存列表】管理所有库存条目，支持网络复制和消息广播
 *
 * 设计目的：
 *   - 作为 FastArraySerializer，支持增量网络复制
 *   - 管理所有 Entry（添加、移除、查询）
 *   - 在复制钩子中广播变化消息
 *
 * 核心机制：
 *   1. AddEntry：创建 Instance，设置 ItemDef，调用 Fragment 初始化，MarkItemDirty
 *   2. RemoveEntry：移除 Entry，MarkArrayDirty
 *   3. PreReplicatedRemove：广播 OldCount -> 0 消息
 *   4. PostReplicatedAdd：广播 0 -> NewCount 消息
 *   5. PostReplicatedChange：广播 LastObserved -> NewCount 消息
 *
 * 网络复制流程：
 *   1. 服务器：AddEntry/RemoveEntry
 *   2. MarkItemDirty/MarkArrayDirty 标记为脏
 *   3. FFastArraySerializer 检测变化，生成增量数据
 *   4. 客户端：接收增量数据
 *   5. 调用 Pre/Post ReplicatedRemove/Add/Change
 *   6. BroadcastChangeMessage 触发 UI 更新
 *
 * 与 ManagerComponent 的关系：
 *   - InventoryList 是 ManagerComponent 的成员
 *   - ManagerComponent 提供公共 API，内部调用 InventoryList
 *   - InventoryList 负责实际的数据管理和复制
 */
USTRUCT(BlueprintType)
struct FShootInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FShootInventoryList()
		: OwnerComponent(nullptr)
	{
	}

	FShootInventoryList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

	/**
	 * 获取所有物品实例
	 * @return 所有非空的 Instance 数组
	 *
	 * 用途：
	 *   - UI 遍历所有物品
	 *   - 查询库存中的所有物品
	 *
	 * 性能：
	 *   - O(n)，n = Entry 数量
	 *   - 创建新数组，有内存分配
	 */
	TArray<UShootInventoryItemInstance*> GetAllItems() const;

public:
	//~FFastArraySerializer contract
	/**
	 * 【复制钩子】在服务器移除 Entry 后、客户端应用移除前调用
	 *
	 * 作用：
	 *   - 广播变化消息：OldCount -> 0
	 *   - 更新 LastObservedCount = 0
	 *
	 * 时机：
	 *   - 服务器调用 RemoveEntry
	 *   - 客户端接收到移除的增量数据
	 *   - 在应用移除前调用此钩子
	 */
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	/**
	 * 【复制钩子】在客户端添加新 Entry 后调用
	 *
	 * 作用：
	 *   - 广播变化消息：0 -> NewCount
	 *   - 更新 LastObservedCount = StackCount
	 *
	 * 时机：
	 *   - 服务器调用 AddEntry
	 *   - 客户端接收到添加的增量数据
	 *   - 在应用添加后调用此钩子
	 */
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	/**
	 * 【复制钩子】在客户端修改 Entry 后调用
	 *
	 * 作用：
	 *   - 广播变化消息：LastObserved -> NewCount
	 *   - 更新 LastObservedCount = StackCount
	 *
	 * 时机：
	 *   - 服务器修改 Entry 的 StackCount
	 *   - 客户端接收到修改的增量数据
	 *   - 在应用修改后调用此钩子
	 */
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	/**
	 * NetDeltaSerialize：FFastArraySerializer 的核心序列化函数
	 *
	 * 作用：
	 *   - 由 UE 网络系统自动调用
	 *   - 实现增量复制（仅传输变化的 Entry）
	 */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FShootInventoryEntry, FShootInventoryList>(Entries, DeltaParms, *this);
	}

	/**
	 * 添加物品（通过 Definition 创建 Instance）
	 * @param ItemClass 物品定义类
	 * @param StackCount 堆栈数量
	 * @return 新创建的物品实例
	 *
	 * 执行流程：
	 *   1. 检查权威性（HasAuthority）
	 *   2. 创建 Instance：NewObject<UShootInventoryItemInstance>(Actor)
	 *   3. 设置 ItemDef
	 *   4. 遍历 Fragment，调用 OnInstanceCreated
	 *   5. 设置 StackCount
	 *   6. MarkItemDirty 标记为脏
	 *   7. 返回 Instance
	 *
	 * 注意：
	 *   - Outer 必须是 Actor（GetOwner()），不能是 Component（UE-127172）
	 *   - 仅服务器可调用（需要 HasAuthority）
	 */
	UShootInventoryItemInstance* AddEntry(TSubclassOf<UShootInventoryItemDefinition> ItemClass, int32 StackCount, const FShootInventoryItemInitData& InitData);

	/**
	 * 添加物品（直接添加已存在的 Instance）
	 * @param Instance 已创建的物品实例
	 *
	 * 用途：
	 *   - 从其他容器转移物品
	 *   - 从存档加载物品
	 *
	 * 注意：
	 *   - 当前未实现（unimplemented）
	 *   - 需要时再实现
	 */
	void AddEntry(UShootInventoryItemInstance* Instance);

	/**
	 * 移除物品实例
	 * @param Instance 要移除的物品实例
	 *
	 * 执行流程：
	 *   1. 遍历 Entries，查找匹配的 Instance
	 *   2. 移除 Entry
	 *   3. MarkArrayDirty 标记数组为脏
	 *
	 * 注意：
	 *   - 仅移除 Entry，不销毁 Instance
	 *   - ManagerComponent 需要调用 RemoveReplicatedSubObject
	 */
	void RemoveEntry(UShootInventoryItemInstance* Instance);

private:
	/**
	 * 广播变化消息
	 * @param Entry 变化的条目
	 * @param OldCount 旧数量
	 * @param NewCount 新数量
	 *
	 * 执行流程：
	 *   1. 创建 FShootInventoryChangeMessage
	 *   2. 填充字段：InventoryOwner、Instance、NewCount、Delta
	 *   3. 通过 GameplayMessageSubsystem 广播消息
	 *   4. Tag：Inventory.Message.StackChanged
	 *
	 * 监听者：
	 *   - 背包 UI
	 *   - 制作 UI
	 *   - 拾取提示
	 *   - 任务系统
	 */
	void BroadcastChangeMessage(FShootInventoryEntry& Entry, int32 OldCount, int32 NewCount);
	void NotifyOwnerItemAdded(UShootInventoryItemInstance* Instance) const;
	void NotifyOwnerItemRemoved(UShootInventoryItemInstance* Instance) const;

private:
	// friend 声明：允许 ManagerComponent 访问私有成员
	friend UShootInventoryManagerComponent;

private:
	/**
	 * 复制的条目数组
	 *
	 * 用途：
	 *   - 存储所有库存条目
	 *   - 网络复制：FFastArraySerializer 监控此数组的变化
	 *   - 增量传输：仅传输添加/删除/修改的 Entry
	 */
	UPROPERTY()
	TArray<FShootInventoryEntry> Entries;

	/**
	 * 拥有组件（不复制）
	 *
	 * 用途：
	 *   - 指向 UShootInventoryManagerComponent
	 *   - 用于获取 World、Actor 等上下文
	 *   - 广播消息时填充 InventoryOwner 字段
	 *
	 * 为什么不复制：
	 *   - 客户端可以通过 Component 反向查找到 List
	 *   - 避免循环引用
	 */
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

/**
 * 注册 NetDeltaSerializer 特性
 *
 * 作用：
 *   - 告知 UE 网络系统此结构体使用自定义增量序列化
 *   - 启用 FFastArraySerializer 机制
 */
template<>
struct TStructOpsTypeTraits<FShootInventoryList> : public TStructOpsTypeTraitsBase2<FShootInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * UShootInventoryManagerComponent
 *
 * 【库存管理组件】管理角色的库存系统，提供公共 API
 *
 * 设计目的：
 *   - 附加到 Actor（AShootCharacter）
 *   - 管理库存列表（FShootInventoryList）
 *   - 处理 SubObject 复制（RegisteredSubObjectList）
 *   - 提供蓝图可调用的 API
 *
 * 核心功能：
 *   1. 添加物品：AddItemDefinition（创建 Instance）、AddItemInstance（直接添加）
 *   2. 移除物品：RemoveItemInstance
 *   3. 查询物品：GetAllItems、FindFirstItemStackByDefinition、GetTotalItemCountByDefinition
 *   4. 消耗物品：ConsumeItemsByDefinition
 *
 * 网络复制机制：
 *   1. InventoryList 自动复制（DOREPLIFETIME）
 *   2. ItemInstance 通过 SubObject 复制：
 *      - 新机制：RegisteredSubObjectList（UE5）
 *      - 旧机制：ReplicateSubobjects（作为后备）
 *   3. 复制钩子触发消息广播
 *
 * SubObject 复制流程：
 *   1. AddItemDefinition：创建 Instance，调用 AddReplicatedSubObject(Instance)
 *   2. ReadyForReplication：注册所有已存在的 Instance
 *   3. RemoveItemInstance：调用 RemoveReplicatedSubObject(Instance)
 *   4. ReplicateSubobjects：作为后备，手动复制所有 Instance
 *
 * 使用场景：
 *   - 玩家背包：管理武器、材料、消耗品等
 *   - AI 背包：管理掉落物品（可精简）
 *   - 仓库容器：管理存储的物品
 *
 * 与其他系统的集成：
 *   - 拾取系统：调用 AddItemDefinition 添加物品
 *   - 制作系统：调用 ConsumeItemsByDefinition 消耗材料，AddItemDefinition 添加产物
 *   - GAS：AbilityCost 读取 StatTags，消耗弹药/材料
 *   - UI：监听 GameplayMessage，更新背包/制作界面
 */
UCLASS(MinimalAPI, BlueprintType)
class UShootInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShootInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 检查是否可以添加物品
	 * @param ItemDef 物品定义类
	 * @param StackCount 堆栈数量
	 * @return 是否可以添加
	 *
	 * 用途：
	 *   - 在添加前检查条件（堆栈上限、唯一性等）
	 *   - UI 显示是否可以拾取/制作
	 *
	 * @TODO: 实现堆栈上限检查、唯一性检查等
	 *   - 当前始终返回 true
	 *   - 需要扩展 Fragment 支持堆栈上限
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	bool CanAddItemDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	/**
	 * 添加物品（通过 Definition 创建 Instance）
	 * @param ItemDef 物品定义类
	 * @param StackCount 堆栈数量
	 * @return 新创建的物品实例
	 *
	 * 执行流程：
	 *   1. 调用 InventoryList.AddEntry(ItemDef, StackCount, InitData)
	 *   2. 如果使用 RegisteredSubObjectList，调用 AddReplicatedSubObject(Instance)
	 *   3. 返回 Instance
	 *
	 * 权限：
	 *   - BlueprintAuthorityOnly：仅服务器可调用
	 *
	 * 使用场景：
	 *   - 拾取物品：AddItemDefinition(BP_Item_MilitaryAlloy, 10)
	 *   - 制作物品：AddItemDefinition(BP_Item_AK47, 1)
	 *   - 任务奖励：AddItemDefinition(BP_Item_Gold, 1000)
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UShootInventoryItemInstance* AddItemDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	/** 账号资产：快捷入口，生命周期固定为 Persistent */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UShootInventoryItemInstance* AddPersistentItem(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	/** 副本临时物品：生命周期 RuntimeOnly，退出副本后需要清理 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UShootInventoryItemInstance* AddRuntimeItem(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount = 1);

	/** RuntimeOnly 武器拾取接口：允许自定义 InitData（例如复制拾取 Guid/初始弹药） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UShootInventoryItemInstance* AddRuntimeWeaponItem(TSubclassOf<UShootInventoryItemDefinition> ItemDef, const FShootInventoryItemInitData& InitData);

	/**
	 * 根据账号 Persistent 武器生成本局独立的 RuntimeOnly 战斗实例。
	 * 默认不复制动态 StatTag：弹药、临时 Buff 等必须由本局武器 Definition/模式规则重新初始化，
	 * 防止一局战斗的消耗污染账号来源物品。只有模式明确需要继承动态状态时才传 true。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UShootInventoryItemInstance* CreateRuntimeWeaponFromPersistent(const UShootInventoryItemInstance* PersistentSource, bool bCopyStatSnapshot = false);

	/**
	 * 添加物品实例（直接添加已存在的 Instance）
	 * @param ItemInstance 已创建的物品实例
	 *
	 * 用途：
	 *   - 从其他容器转移物品
	 *   - 从存档加载物品
	 *
	 * 权限：
	 *   - BlueprintAuthorityOnly：仅服务器可调用
	 *
	 * 注意：
	 *   - 当前 InventoryList.AddEntry(Instance) 未实现
	 *   - 需要时再实现
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddItemInstance(UShootInventoryItemInstance* ItemInstance);

	/**
	 * 移除物品实例
	 * @param ItemInstance 要移除的物品实例
	 *
	 * 执行流程：
	 *   1. 调用 InventoryList.RemoveEntry(ItemInstance)
	 *   2. 如果使用 RegisteredSubObjectList，调用 RemoveReplicatedSubObject(Instance)
	 *
	 * 权限：
	 *   - BlueprintAuthorityOnly：仅服务器可调用
	 *
	 * 使用场景：
	 *   - 装备武器：从背包移除，添加到装备槽
	 *   - 丢弃物品：从背包移除
	 *   - 消耗物品：使用后移除
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void RemoveItemInstance(UShootInventoryItemInstance* ItemInstance);

	/** 清理所有 RuntimeOnly 物品（副本结束或 Hub 切换时调用） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void RemoveRuntimeItems();

	/**
	 * 获取所有物品实例
	 * @return 所有非空的 Instance 数组
	 *
	 * 用途：
	 *   - UI 遍历所有物品
	 *   - 查询库存中的所有物品
	 *
	 * 注意：
	 *   - BlueprintPure=false：告知蓝图此函数不是纯函数（有副作用：创建数组）
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure=false)
	TArray<UShootInventoryItemInstance*> GetAllItems() const;

	/** 通过 Guid 查找物品实例（QuickBar/Save 使用） */
	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	UShootInventoryItemInstance* FindItemByInstanceId(FGuid ItemInstanceId) const;

	/** 通过 Guid 移除物品（Hub/制作消耗/QuickBar 释放） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	bool RemoveItemByInstanceId(FGuid ItemInstanceId);

	/**
	 * 查找第一个匹配定义的物品
	 * @param ItemDef 物品定义类
	 * @return 第一个匹配的 Instance，未找到返回 nullptr
	 *
	 * 用途：
	 *   - 检查是否拥有某种物品
	 *   - 获取物品实例以查询 StatTags
	 *
	 * 性能：
	 *   - O(n)，n = Entry 数量
	 *   - 返回第一个匹配项
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	UShootInventoryItemInstance* FindFirstItemStackByDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef) const;

	/**
	 * 获取指定定义的物品总数
	 * @param ItemDef 物品定义类
	 * @return 物品总数（Entry 数量，非 StackCount）
	 *
	 * 用途：
	 *   - 统计拥有多少个该物品
	 *   - UI 显示物品数量
	 *
	 * 注意：
	 *   - 返回 Entry 数量，不是 StackCount 总和
	 *   - 如果需要 StackCount 总和，需要遍历 Entry 累加
	 *
	 * 性能：
	 *   - O(n)，n = Entry 数量
	 */
	int32 GetTotalItemCountByDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef) const;

	/**
	 * 消耗指定定义的物品
	 * @param ItemDef 物品定义类
	 * @param NumToConsume 要消耗的数量
	 * @return 是否成功消耗
	 *
	 * 执行流程：
	 *   1. 检查权威性（HasAuthority）
	 *   2. 循环查找匹配的 Instance
	 *   3. 移除 Entry
	 *   4. 累加已消耗数量
	 *   5. 如果数量不足，返回 false
	 *   6. 如果成功消耗，返回 true
	 *
	 * 使用场景：
	 *   - 制作系统：消耗材料
	 *   - 任务系统：消耗任务物品
	 *
	 * 注意：
	 *   - 当前实现：移除整个 Entry（不支持部分消耗）
	 *   - @TODO: N 平方复杂度，需要加速结构
	 */
	bool ConsumeItemsByDefinition(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 NumToConsume);

	/** 构建 Persistent 物品的存档数据（仅服务器） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|SaveGame")
	void BuildPersistentItemsSaveData(TArray<FSavedInventoryItem>& OutItems) const;

	/** 根据存档数据恢复 Persistent 物品（仅服务器） */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Inventory|SaveGame")
	void ApplyPersistentItemsSaveData(const TArray<FSavedInventoryItem>& InItems);

	//~UObject interface
	/**
	 * 手动复制子对象（旧机制，作为后备）
	 *
	 * 作用：
	 *   - 在不支持 RegisteredSubObjectList 的 UE 版本中使用
	 *   - 手动复制所有 ItemInstance
	 *
	 * 执行流程：
	 *   1. 调用 Super::ReplicateSubobjects
	 *   2. 遍历所有 Entry
	 *   3. 对每个 Instance，调用 Channel->ReplicateSubobject
	 *   4. 返回是否写入了数据
	 */
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	/**
	 * 准备复制（注册已存在的 SubObject）
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
	 *   1. 调用 Super::ReadyForReplication
	 *   2. 如果使用 RegisteredSubObjectList
	 *   3. 遍历所有 Entry
	 *   4. 对每个 Instance，调用 AddReplicatedSubObject
	 */
	virtual void ReadyForReplication() override;
	//~End of UObject interface

private:
	/** 背包最大条目数量（<=0 表示不限制） */
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	int32 MaxEntryCount = 0;

	/**
	 * 库存列表
	 *
	 * 用途：
	 *   - 存储所有库存条目
	 *   - 处理网络复制和消息广播
	 *
	 * 复制：
	 *   - 自动复制到客户端
	 *   - FastArray 增量复制，仅传输变化
	 */
	UPROPERTY(Replicated)
	FShootInventoryList InventoryList;

	UPROPERTY(Transient)
	TMap<FGuid, TWeakObjectPtr<UShootInventoryItemInstance>> InstanceMap;

	UShootInventoryItemInstance* AddItemDefinitionInternal(TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 StackCount, const FShootInventoryItemInitData& InitData);

	void HandleItemInstanceAdded(UShootInventoryItemInstance* Instance);
	void HandleItemInstanceRemoved(UShootInventoryItemInstance* Instance);

	friend struct FShootInventoryList;
};
