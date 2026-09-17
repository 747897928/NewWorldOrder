// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"

#include "ShootGameplayTagStack.generated.h"

struct FShootGameplayTagStackContainer;
struct FNetDeltaSerializeInfo;

/**
 * FShootGameplayTagStack
 *
 * 【核心数据结构】单个 GameplayTag 的堆栈项（Tag + 计数）
 *
 * 设计目的：
 *   - 表示"某个标签的数量"，例如 Inventory.Material.MilitaryAlloy = 50
 *   - 作为 FastArraySerializerItem，支持 UE 网络增量复制
 *   - 用于存储材料、货币、弹药等可堆叠资源
 *
 * 关键特性：
 *   - 继承 FFastArraySerializerItem：获得网络复制追踪能力（ReplicationID、ReplicationKey）
 *   - 私有成员 Tag 和 StackCount：只能通过 Container 操作，避免直接修改导致缓存不同步
 *   - friend FShootGameplayTagStackContainer：允许容器访问私有成员
 *
 * 使用场景：
 *   - 不直接使用，由 FShootGameplayTagStackContainer 管理
 *   - 在背包中表示"军用合金 x50"、"金币 x5000"等堆栈
 */
USTRUCT(BlueprintType)
struct FShootGameplayTagStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FShootGameplayTagStack()
	{}

	FShootGameplayTagStack(FGameplayTag InTag, int32 InStackCount)
		: Tag(InTag)
		, StackCount(InStackCount)
	{
	}

	// 调试用：返回 "Tag x Count" 格式字符串
	FString GetDebugString() const;

private:
	// friend 声明：允许容器访问私有成员
	friend FShootGameplayTagStackContainer;

	// 标签标识符（例如 Inventory.Material.MilitaryAlloy）
	// 私有：防止外部直接修改，必须通过 Container 的 AddStack/RemoveStack
	UPROPERTY()
	FGameplayTag Tag;

	// 堆栈计数（数量）
	// 私有：确保所有修改都通过 Container，以便维护 TagToCountMap 缓存
	UPROPERTY()
	int32 StackCount = 0;
};

/**
 * FShootGameplayTagStackContainer
 *
 * 【核心容器】GameplayTag 堆栈的容器，支持网络复制和高效查询
 *
 * 设计目的：
 *   - 管理角色的所有材料/货币/弹药等堆栈资源
 *   - 使用 FastArray 实现增量网络复制（仅传输变化）
 *   - 提供 O(1) 查询性能（通过 TMap 缓存）
 *
 * 双重数据结构：
 *   1. Stacks (TArray<FShootGameplayTagStack>)：
 *      - 用于网络复制（FFastArraySerializer 机制）
 *      - 仅传输变化的堆栈（增量复制，节省带宽）
 *   2. TagToCountMap (TMap<FGameplayTag, int32>)：
 *      - 本地缓存，不复制
 *      - 提供 O(1) 查询性能（GetStackCount/ContainsTag）
 *
 * 复制流程：
 *   1. 服务器调用 AddStack/RemoveStack
 *   2. MarkItemDirty/MarkArrayDirty 标记项/数组为脏
 *   3. FFastArraySerializer 检测变化，生成增量数据
 *   4. 客户端接收增量数据
 *   5. 调用 PreReplicatedRemove/PostReplicatedAdd/PostReplicatedChange
 *   6. 更新客户端的 TagToCountMap 缓存
 *
 * 关键机制：
 *   - PreReplicatedRemove：在移除复制项前，从 Map 中删除对应 Tag
 *   - PostReplicatedAdd：在添加复制项后，向 Map 中添加 Tag-Count 映射
 *   - PostReplicatedChange：在修改复制项后，更新 Map 中的 Count
 *
 * 使用场景：
 *   - 嵌入到 UShootInventoryItemInstance 的 StatTags 中
 *   - 存储该物品实例的堆栈数据（如弹药数量、材料批次等）
 *   - 与 GAS AbilityCost 配合，实现材料消耗、弹药扣减
 */
USTRUCT(BlueprintType)
struct FShootGameplayTagStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FShootGameplayTagStackContainer()
	{
	}

public:
	/**
	 * 添加堆栈
	 * @param Tag 要添加的标签（如 Inventory.Material.MilitaryAlloy）
	 * @param StackCount 要添加的数量（必须 > 0）
	 *
	 * 逻辑：
	 *   - 如果标签已存在，增加其计数
	 *   - 如果标签不存在，创建新的堆栈项
	 *   - MarkItemDirty 标记为脏，触发网络复制
	 *   - 同步更新 TagToCountMap 缓存
	 */
	void AddStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 移除堆栈
	 * @param Tag 要移除的标签
	 * @param StackCount 要移除的数量（必须 > 0）
	 *
	 * 逻辑：
	 *   - 如果移除后计数 <= 0，完全移除该堆栈项
	 *   - 如果移除后计数 > 0，减少计数
	 *   - MarkItemDirty 或 MarkArrayDirty 标记为脏
	 *   - 同步更新 TagToCountMap 缓存
	 */
	void RemoveStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 获取指定标签的堆栈计数
	 * @param Tag 要查询的标签
	 * @return 计数（如果不存在返回 0）
	 *
	 * 性能：O(1)，直接查询 TMap
	 */
	int32 GetStackCount(FGameplayTag Tag) const
	{
		return TagToCountMap.FindRef(Tag);
	}

	/**
	 * 检查是否包含指定标签
	 * @param Tag 要检查的标签
	 * @return 是否至少有 1 个堆栈
	 *
	 * 性能：O(1)，直接查询 TMap
	 */
	bool ContainsTag(FGameplayTag Tag) const
	{
		return TagToCountMap.Contains(Tag);
	}

	template<typename FuncType>
	void ForEachStack(FuncType&& Func) const
	{
		for (const FShootGameplayTagStack& Stack : Stacks)
		{
			Func(Stack.Tag, Stack.StackCount);
		}
	}

	//~FFastArraySerializer contract
	// 【复制钩子】在服务器移除项后、客户端应用移除前调用
	// 作用：从 TagToCountMap 中删除对应 Tag，保持缓存一致性
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	// 【复制钩子】在客户端添加新项后调用
	// 作用：向 TagToCountMap 中添加 Tag-Count 映射
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	// 【复制钩子】在客户端修改项后调用
	// 作用：更新 TagToCountMap 中的 Count
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	// NetDeltaSerialize：FFastArraySerializer 的核心序列化函数
	// 由 UE 网络系统自动调用，实现增量复制
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FShootGameplayTagStack, FShootGameplayTagStackContainer>(Stacks, DeltaParms, *this);
	}

private:
	/**
	 * 复制的堆栈数组
	 *
	 * 用途：
	 *   - 网络复制：FFastArraySerializer 会监控此数组的变化
	 *   - 增量传输：仅传输添加/删除/修改的项
	 *
	 * 注意：
	 *   - 不要直接操作此数组，必须通过 AddStack/RemoveStack
	 *   - 直接修改会导致 TagToCountMap 缓存不同步
	 */
	UPROPERTY()
	TArray<FShootGameplayTagStack> Stacks;

	/**
	 * 加速查询的 Map（本地缓存，不复制）
	 *
	 * 用途：
	 *   - 提供 O(1) 查询性能（GetStackCount/ContainsTag）
	 *   - 由复制钩子自动维护（Pre/Post ReplicatedRemove/Add/Change）
	 *
	 * 注意：
	 *   - NotReplicated：不会通过网络传输（由 Stacks 推导）
	 *   - 服务器和客户端各自维护自己的 Map
	 */
	TMap<FGameplayTag, int32> TagToCountMap;
};

/**
 * 注册 NetDeltaSerializer 特性
 *
 * 作用：
 *   - 告知 UE 网络系统此结构体使用自定义增量序列化
 *   - 启用 FFastArraySerializer 机制
 */
template<>
struct TStructOpsTypeTraits<FShootGameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FShootGameplayTagStackContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
