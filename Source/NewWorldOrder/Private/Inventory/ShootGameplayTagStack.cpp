// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Inventory/ShootGameplayTagStack.h"

#include "UObject/Stack.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGameplayTagStack)

//////////////////////////////////////////////////////////////////////
// FShootGameplayTagStack

FString FShootGameplayTagStack::GetDebugString() const
{
	return FString::Printf(TEXT("%sx%d"), *Tag.ToString(), StackCount);
}

//////////////////////////////////////////////////////////////////////
// FShootGameplayTagStackContainer

/**
 * 添加堆栈实现
 *
 * 执行流程：
 *   1. 验证 Tag 有效性（防止无效标签导致数据混乱）
 *   2. 验证 StackCount > 0（忽略无效的添加请求）
 *   3. 遍历现有堆栈数组，查找匹配的 Tag
 *      - 如果找到：增加计数，更新 Map 缓存，MarkItemDirty 标记项为脏
 *      - 如果未找到：创建新堆栈项，MarkItemDirty 标记项为脏，添加到 Map
 *
 * 网络复制：
 *   - MarkItemDirty(Stack)：标记单个项为脏，FFastArraySerializer 检测到变化
 *   - 下次网络更新时，仅传输此项的增量数据到客户端
 *   - 客户端收到后，调用 PostReplicatedAdd 或 PostReplicatedChange
 *
 * 性能：
 *   - 查找：O(n)，n = 堆栈数量（通常很小，材料种类有限）
 *   - Map 更新：O(1)
 */
void FShootGameplayTagStackContainer::AddStack(FGameplayTag Tag, int32 StackCount)
{
	// 验证标签有效性
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to AddStack"), ELogVerbosity::Warning);
		return;
	}

	// 忽略无效的堆栈数量
	if (StackCount > 0)
	{
		// 遍历现有堆栈，查找匹配的 Tag
		for (FShootGameplayTagStack& Stack : Stacks)
		{
			if (Stack.Tag == Tag)
			{
				// 找到匹配项，增加计数
				const int32 NewCount = Stack.StackCount + StackCount;
				Stack.StackCount = NewCount;

				// 更新 Map 缓存（保持一致性）
				TagToCountMap[Tag] = NewCount;

				// 标记此项为脏，触发网络复制
				// 仅此项会被复制，其他项不受影响（增量复制）
				MarkItemDirty(Stack);
				return;
			}
		}

		// 未找到匹配项，创建新堆栈
		FShootGameplayTagStack& NewStack = Stacks.Emplace_GetRef(Tag, StackCount);

		// 标记新项为脏，触发网络复制
		MarkItemDirty(NewStack);

		// 添加到 Map 缓存
		TagToCountMap.Add(Tag, StackCount);
	}
}

/**
 * 移除堆栈实现
 *
 * 执行流程：
 *   1. 验证 Tag 有效性
 *   2. 验证 StackCount > 0
 *   3. 遍历堆栈数组，查找匹配的 Tag
 *      - 如果移除后 <= 0：完全移除此项，从 Map 中删除，MarkArrayDirty
 *      - 如果移除后 > 0：减少计数，更新 Map，MarkItemDirty
 *
 * 网络复制：
 *   - MarkArrayDirty()：标记整个数组为脏（因为移除了项）
 *   - MarkItemDirty(Stack)：标记单个项为脏（因为修改了计数）
 *   - 客户端收到后，调用 PreReplicatedRemove 或 PostReplicatedChange
 *
 * 注意：
 *   - 如果尝试移除不存在的 Tag，什么都不做（不报错）
 *   - 如果尝试移除的数量超过现有数量，完全移除该堆栈（不会变成负数）
 *
 * 性能：
 *   - 查找：O(n)，n = 堆栈数量
 *   - Map 更新/移除：O(1)
 */
void FShootGameplayTagStackContainer::RemoveStack(FGameplayTag Tag, int32 StackCount)
{
	// 验证标签有效性
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to RemoveStack"), ELogVerbosity::Warning);
		return;
	}

	// @TODO: 是否应该在尝试移除不存在的堆栈或数量不足时报错？
	// 当前行为：静默忽略（与 Lyra 保持一致）
	if (StackCount > 0)
	{
		// 使用迭代器遍历，因为可能需要移除项
		for (auto It = Stacks.CreateIterator(); It; ++It)
		{
			FShootGameplayTagStack& Stack = *It;
			if (Stack.Tag == Tag)
			{
				// 找到匹配项
				if (Stack.StackCount <= StackCount)
				{
					// 移除后数量 <= 0，完全移除此堆栈项
					It.RemoveCurrent();

					// 从 Map 中删除
					TagToCountMap.Remove(Tag);

					// 标记整个数组为脏（因为移除了项）
					MarkArrayDirty();
				}
				else
				{
					// 移除后数量 > 0，减少计数
					const int32 NewCount = Stack.StackCount - StackCount;
					Stack.StackCount = NewCount;

					// 更新 Map 缓存
					TagToCountMap[Tag] = NewCount;

					// 标记此项为脏
					MarkItemDirty(Stack);
				}
				return;
			}
		}
	}
}

/**
 * PreReplicatedRemove：在客户端应用移除前调用
 *
 * 时机：
 *   - 服务器移除了某些堆栈项
 *   - FFastArraySerializer 检测到移除
 *   - 在客户端应用移除前，调用此钩子
 *
 * 作用：
 *   - 从 TagToCountMap 中删除被移除的 Tag
 *   - 保持 Map 缓存与 Stacks 数组一致
 *
 * 参数：
 *   - RemovedIndices：被移除的索引数组
 *   - FinalSize：移除后的最终大小
 *
 * 执行流程：
 *   - 遍历 RemovedIndices
 *   - 对每个索引，从 Map 中删除对应的 Tag
 */
void FShootGameplayTagStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FGameplayTag Tag = Stacks[Index].Tag;
		TagToCountMap.Remove(Tag);
	}
}

/**
 * PostReplicatedAdd：在客户端添加新项后调用
 *
 * 时机：
 *   - 服务器添加了新的堆栈项
 *   - 客户端接收到增量数据并添加到数组后
 *   - 调用此钩子
 *
 * 作用：
 *   - 向 TagToCountMap 中添加新的 Tag-Count 映射
 *   - 保持 Map 缓存与 Stacks 数组一致
 *
 * 参数：
 *   - AddedIndices：新添加的索引数组
 *   - FinalSize：添加后的最终大小
 *
 * 执行流程：
 *   - 遍历 AddedIndices
 *   - 对每个索引，向 Map 添加 Tag 和 StackCount
 */
void FShootGameplayTagStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FShootGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap.Add(Stack.Tag, Stack.StackCount);
	}
}

/**
 * PostReplicatedChange：在客户端修改项后调用
 *
 * 时机：
 *   - 服务器修改了现有堆栈项的计数
 *   - 客户端接收到增量数据并更新后
 *   - 调用此钩子
 *
 * 作用：
 *   - 更新 TagToCountMap 中的计数
 *   - 保持 Map 缓存与 Stacks 数组一致
 *
 * 参数：
 *   - ChangedIndices：被修改的索引数组
 *   - FinalSize：修改后的最终大小
 *
 * 执行流程：
 *   - 遍历 ChangedIndices
 *   - 对每个索引，更新 Map 中的计数
 */
void FShootGameplayTagStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FShootGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap[Stack.Tag] = Stack.StackCount;
	}
}
