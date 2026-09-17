// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Inventory/ShootInventoryItemInstance.h"

#include "Inventory/ShootInventoryItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

#if UE_WITH_IRIS
#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#endif // UE_WITH_IRIS

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootInventoryItemInstance)

class FLifetimeProperty;

UShootInventoryItemInstance::UShootInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/**
 * 注册复制属性
 *
 * 作用：
 *   - 告知 UE 网络系统哪些属性需要复制
 *   - DOREPLIFETIME：注册无条件复制（所有客户端都接收）
 *
 * 复制属性：
 *   1. StatTags：堆栈数据容器
 *      - FastArray 增量复制，仅传输变化
 *      - 客户端接收后，通过复制钩子更新 TagToCountMap
 *   2. ItemDef：物品定义类
 *      - 复制类引用，客户端通过 CDO 访问静态数据
 *
 * 条件复制：
 *   - 当前使用无条件复制（DOREPLIFETIME）
 *   - 可优化为仅复制给拥有者（DOREPLIFETIME_CONDITION(ThisClass, StatTags, COND_OwnerOnly)）
 */
void UShootInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, StatTags);
	DOREPLIFETIME(ThisClass, ItemDef);
	DOREPLIFETIME(ThisClass, ItemInstanceId);
	DOREPLIFETIME(ThisClass, ItemLifetime);
}

#if UE_WITH_IRIS
/**
 * 注册 Iris 复制片段（UE5 新网络系统）
 *
 * 作用：
 *   - Iris：UE5 的下一代网络复制系统
 *   - 为此对象创建和注册复制片段
 *   - 支持更高效的网络复制和带宽优化
 *
 * 执行流程：
 *   1. 调用 CreateAndRegisterFragmentsForObject
 *   2. 为此对象创建属性复制片段（StatTags、ItemDef）
 *   3. 注册到 Iris 复制系统
 *
 * 注意：
 *   - 仅在启用 Iris 时编译（UE_WITH_IRIS）
 *   - 与传统复制系统共存，无需手动切换
 */
void UShootInventoryItemInstance::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	using namespace UE::Net;

	// 为此对象创建和注册复制片段
	FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}
#endif // UE_WITH_IRIS

/**
 * 添加 StatTag 堆栈实现
 *
 * 执行流程：
 *   1. 调用 StatTags.AddStack(Tag, StackCount)
 *   2. StatTags 内部处理：
 *      - 查找或创建堆栈项
 *      - 增加计数
 *      - MarkItemDirty 标记为脏
 *      - 更新 TagToCountMap 缓存
 *   3. 网络复制：
 *      - FFastArraySerializer 检测变化
 *      - 下次网络更新时，传输增量数据到客户端
 *      - 客户端调用 PostReplicatedAdd/Change 更新缓存
 *
 * 使用场景：
 *   - 拾取弹药：Instance->AddStatTagStack(Inventory.Ammo.Rifle, 30)
 *   - 制作材料：Instance->AddStatTagStack(Inventory.Material.MilitaryAlloy, 50)
 *   - 修复装备：Instance->AddStatTagStack(Inventory.Durability, 20)
 */
void UShootInventoryItemInstance::AddStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	const int32 PreviousCount = StatTags.GetStackCount(Tag);
	StatTags.AddStack(Tag, StackCount);
	if (StatTags.GetStackCount(Tag) != PreviousCount)
	{
		BroadcastStatTagsChanged();
	}
}

/**
 * 移除 StatTag 堆栈实现
 *
 * 执行流程：
 *   1. 调用 StatTags.RemoveStack(Tag, StackCount)
 *   2. StatTags 内部处理：
 *      - 查找堆栈项
 *      - 减少计数或移除项
 *      - MarkItemDirty/MarkArrayDirty 标记为脏
 *      - 更新 TagToCountMap 缓存
 *   3. 网络复制：
 *      - FFastArraySerializer 检测变化
 *      - 传输增量数据到客户端
 *      - 客户端调用 PreReplicatedRemove/PostReplicatedChange 更新缓存
 *
 * 使用场景：
 *   - 消耗弹药：Instance->RemoveStatTagStack(Inventory.Ammo.Rifle, 1)
 *   - 消耗材料：Instance->RemoveStatTagStack(Inventory.Material.MilitaryAlloy, 50)
 *   - 损耗装备：Instance->RemoveStatTagStack(Inventory.Durability, 10)
 */
void UShootInventoryItemInstance::RemoveStatTagStack(FGameplayTag Tag, int32 StackCount)
{
	const int32 PreviousCount = StatTags.GetStackCount(Tag);
	StatTags.RemoveStack(Tag, StackCount);
	if (StatTags.GetStackCount(Tag) != PreviousCount)
	{
		BroadcastStatTagsChanged();
	}
}

void UShootInventoryItemInstance::OnRep_StatTags()
{
	// FastArray 已先更新查询缓存；拥有端在这里广播服务器纠正后的最终值。
	BroadcastStatTagsChanged();
}

void UShootInventoryItemInstance::BroadcastStatTagsChanged()
{
	StatTagsChanged.Broadcast(this);
}

/**
 * 获取 StatTag 堆栈计数实现
 *
 * 执行流程：
 *   1. 调用 StatTags.GetStackCount(Tag)
 *   2. 通过 TagToCountMap.FindRef(Tag) 查询
 *   3. 返回计数（不存在返回 0）
 *
 * 性能：
 *   - O(1)，直接查询 TMap
 *
 * 客户端使用：
 *   - 客户端可以查询（StatTags 已复制）
 *   - 用于 UI 显示（弹药数量、材料数量等）
 *   - 用于条件检查（是否有足够材料制作）
 */
int32 UShootInventoryItemInstance::GetStatTagStackCount(FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

/**
 * 检查是否包含 StatTag 实现
 *
 * 执行流程：
 *   1. 调用 StatTags.ContainsTag(Tag)
 *   2. 通过 TagToCountMap.Contains(Tag) 查询
 *   3. 返回是否存在
 *
 * 性能：
 *   - O(1)，直接查询 TMap
 *
 * 使用场景：
 *   - 检查是否有弹药：Instance->HasStatTag(Inventory.Ammo.Rifle)
 *   - 检查是否有材料：Instance->HasStatTag(Inventory.Material.MilitaryAlloy)
 */
bool UShootInventoryItemInstance::HasStatTag(FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

/**
 * 设置物品定义实现
 *
 * 作用：
 *   - 关联 Instance 与 Definition
 *   - 在 InventoryList::AddEntry 中调用
 *
 * 执行流程：
 *   1. 设置 ItemDef = InDef
 *   2. 标记为脏（如果已启用复制）
 *   3. 下次网络更新时，复制 ItemDef 到客户端
 *
 * 注意：
 *   - 私有函数，仅 InventoryList 可以调用（friend 声明）
 *   - 一旦设置，通常不应再修改（Definition 是固定的）
 */
void UShootInventoryItemInstance::SetItemDef(TSubclassOf<UShootInventoryItemDefinition> InDef)
{
	ItemDef = InDef;
	RefreshDisplayDataFromDefinition();
}

void UShootInventoryItemInstance::InitializeInstanceLifecycle(EShootItemLifetime InLifetime, const FGuid& InInstanceId)
{
	ItemLifetime = InLifetime;
	ItemInstanceId = InInstanceId.IsValid() ? InInstanceId : FGuid::NewGuid();
}

void UShootInventoryItemInstance::OnRep_ItemDef()
{
	RefreshDisplayDataFromDefinition();

	// ItemDef 与 StatTags 也是独立复制属性。若 StatTags 更早到达，CombatComponent 当时
	// 还无法从 Definition 解析 WeaponId；定义就绪后沿用现有状态委托再广播一次，
	// 让拥有端 HUD 用同一 ItemInstance 的最终弹匣/备弹值完成纠正。
	BroadcastStatTagsChanged();
}

void UShootInventoryItemInstance::RefreshDisplayDataFromDefinition()
{
	if (ItemDef)
	{
		if (const UShootInventoryItemDefinition* ItemCDO = ItemDef->GetDefaultObject<UShootInventoryItemDefinition>())
		{
			CachedDisplayName = ItemCDO->DisplayName;
			CachedIcon = ItemCDO->Icon;
			return;
		}
	}

	CachedDisplayName = FText::GetEmpty();
	CachedIcon = FSlateBrush();
}

/**
 * 查找指定类型的 Fragment 实现
 *
 * 执行流程：
 *   1. 验证 ItemDef 和 FragmentClass 不为 nullptr
 *   2. 通过 GetDefault<UShootInventoryItemDefinition>(ItemDef) 获取 CDO
 *   3. 调用 CDO->FindFragmentByClass(FragmentClass)
 *   4. 返回结果
 *
 * 为什么查询 Definition：
 *   - Fragment 存储在 Definition 中，不存储在 Instance 中
 *   - Instance 仅存储动态状态（StatTags），不复制静态数据
 *   - 所有 Instance 共享同一个 Definition 的 Fragment
 *
 * 使用场景：
 *   - 查询武器数据：Instance->FindFragmentByClass<UShootInventoryItemFragment_Weapon>()
 *   - 查询消耗品数据：Instance->FindFragmentByClass<UShootInventoryItemFragment_Consumable>()
 *
 * 性能：
 *   - GetDefault：O(1)，访问 CDO
 *   - FindFragmentByClass：O(n)，n = Fragment 数量（通常 <= 5）
 */
const UShootInventoryItemFragment* UShootInventoryItemInstance::FindFragmentByClass(TSubclassOf<UShootInventoryItemFragment> FragmentClass) const
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UShootInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}

	return nullptr;
}

void UShootInventoryItemInstance::ApplyStatTagSnapshot(const FShootGameplayTagStackContainer& Snapshot)
{
	const AActor* OwnerActor = GetTypedOuter<AActor>();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	TArray<TPair<FGameplayTag, int32>> ExistingStacks;
	StatTags.ForEachStack([&ExistingStacks](FGameplayTag Tag, int32 Count)
	{
		ExistingStacks.Emplace(Tag, Count);
	});

	for (const TPair<FGameplayTag, int32>& Pair : ExistingStacks)
	{
		if (Pair.Value > 0)
		{
			StatTags.RemoveStack(Pair.Key, Pair.Value);
		}
	}

	Snapshot.ForEachStack([this](FGameplayTag Tag, int32 Count)
	{
		if (Count > 0)
		{
			StatTags.AddStack(Tag, Count);
		}
	});
}
