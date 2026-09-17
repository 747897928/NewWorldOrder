// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Inventory/ShootGameplayTagStack.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Styling/SlateBrush.h"
#include "Misc/Guid.h"
#include "Templates/SubclassOf.h"

#include "ShootInventoryItemInstance.generated.h"

class FLifetimeProperty;

class UShootInventoryItemDefinition;
class UShootInventoryItemFragment;
class UShootInventoryItemInstance;
struct FFrame;
struct FGameplayTag;

/** ItemInstance 内部 StatTags 发生本地权威修改或客户端复制更新时广播。消费者重新查询所需 Tag。 */
DECLARE_MULTICAST_DELEGATE_OneParam(FShootItemStatTagsChanged, UShootInventoryItemInstance*);

UENUM(BlueprintType)
enum class EShootItemLifetime : uint8
{
	Persistent UMETA(DisplayName="Persistent"),
	RuntimeOnly UMETA(DisplayName="RuntimeOnly")
};

/**
 * UShootInventoryItemInstance
 *
 * 【物品实例】运行时的物品实例，存储动态状态和数据
 *
 * 设计目的：
 *   - 定义与实例分离：Definition 是静态的（共享），Instance 是动态的（独立）
 *   - 网络复制：作为 UObject SubObject 复制到客户端
 *   - 状态存储：使用 StatTags 存储堆栈数据（弹药、材料、耐久等）
 *
 * 关键特性：
 *   - UObject 形式：支持网络复制、GC 管理
 *   - IsSupportedForNetworking：启用网络支持
 *   - SubObject 复制：通过 InventoryManagerComponent 的 RegisteredSubObjectList 复制
 *
 * 数据结构：
 *   1. ItemDef：指向物品定义（静态数据）
 *   2. StatTags：堆栈数据容器（动态状态）
 *
 * ItemDef vs Instance 示例：
 *   - Definition：AK47 的基础属性（名称、图标、初始弹药容量、伤害等）
 *   - Instance1：玩家背包中的 AK47，当前弹药 30/120，耐久 80%
 *   - Instance2：玩家仓库中的 AK47，当前弹药 0/120，耐久 100%
 *
 * StatTags 使用场景：
 *   - 弹药：Inventory.Ammo.Rifle = 120（后备弹药数量）
 *   - 材料：Inventory.Material.MilitaryAlloy = 50（材料批次）
 *   - 耐久：Inventory.Durability = 80（装备耐久度）
 *   - 充能：Inventory.Charge = 3（可充能物品的当前充能数）
 *
 * 生命周期：
 *   1. 服务器：InventoryManager->AddItemDefinition(ItemDefClass, Count)
 *   2. 创建 Instance：NewObject<UShootInventoryItemInstance>(Owner)
 *   3. 设置 ItemDef：Instance->SetItemDef(ItemDefClass)
 *   4. 初始化 StatTags：Fragment->OnInstanceCreated(Instance)
 *   5. 注册子对象：InventoryManager->AddReplicatedSubObject(Instance)
 *   6. 网络复制：Instance 自动复制到客户端
 *   7. 移除：InventoryManager->RemoveReplicatedSubObject(Instance)
 *
 * 网络复制机制：
 *   - SubObject 复制：通过 InventoryManagerComponent 的 RegisteredSubObjectList
 *   - StatTags 复制：FShootGameplayTagStackContainer 使用 FastArray 增量复制
 *   - ItemDef 复制：TSubclassOf 直接复制类引用
 *
 * 与 GAS 集成：
 *   - AbilityCost 可以读取/修改 StatTags（如弹药消耗）
 *   - GameplayEffect 可以修改 StatTags（如耐久损耗）
 *
 * 注意事项：
 *   - Outer 必须是 Actor，不能是 Component（UE 限制，见 UE-127172）
 *   - 所有修改 StatTags 的操作必须在服务器上执行（BlueprintAuthorityOnly）
 *   - Instance 不应该直接修改 ItemDef（Definition 是 Const）
 */
UCLASS(BlueprintType)
class UShootInventoryItemInstance : public UObject
{
	GENERATED_BODY()

public:
	UShootInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
	/**
	 * 启用网络支持
	 *
	 * 作用：
	 *   - 告知 UE 网络系统此 UObject 支持网络复制
	 *   - 允许作为 SubObject 复制
	 */
	virtual bool IsSupportedForNetworking() const override { return true; }
	//~End of UObject interface

	/**
	 * 添加 StatTag 堆栈
	 * @param Tag 要添加的标签（如 Inventory.Ammo.Rifle）
	 * @param StackCount 要添加的数量（必须 > 0）
	 *
	 * 用途：
	 *   - 增加弹药：AddStatTagStack(Inventory.Ammo.Rifle, 30)
	 *   - 增加材料：AddStatTagStack(Inventory.Material.MilitaryAlloy, 10)
	 *   - 增加耐久：AddStatTagStack(Inventory.Durability, 20)
	 *
	 * 权限：
	 *   - BlueprintAuthorityOnly：仅服务器可调用
	 *   - 客户端调用会被忽略（避免预测问题）
	 *
	 * 网络复制：
	 *   - 服务器调用后，StatTags 自动复制到客户端
	 *   - 使用 FastArray 增量复制，仅传输变化
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 移除 StatTag 堆栈
	 * @param Tag 要移除的标签
	 * @param StackCount 要移除的数量（必须 > 0）
	 *
	 * 用途：
	 *   - 消耗弹药：RemoveStatTagStack(Inventory.Ammo.Rifle, 1)
	 *   - 消耗材料：RemoveStatTagStack(Inventory.Material.MilitaryAlloy, 50)
	 *   - 损耗耐久：RemoveStatTagStack(Inventory.Durability, 10)
	 *
	 * 权限：
	 *   - BlueprintAuthorityOnly：仅服务器可调用
	 *
	 * 注意：
	 *   - 如果移除后数量 <= 0，该 Tag 会从 StatTags 中完全移除
	 *   - 如果尝试移除不存在的 Tag，什么都不做（不报错）
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 获取 StatTag 堆栈计数
	 * @param Tag 要查询的标签
	 * @return 计数（如果不存在返回 0）
	 *
	 * 用途：
	 *   - 查询弹药：GetStatTagStackCount(Inventory.Ammo.Rifle) -> 120
	 *   - 查询材料：GetStatTagStackCount(Inventory.Material.MilitaryAlloy) -> 50
	 *
	 * 性能：
	 *   - O(1)，通过 TagToCountMap 缓存查询
	 *
	 * 客户端可用：
	 *   - 客户端可以查询（StatTags 已复制）
	 *   - 用于 UI 显示、条件检查等
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	/**
	 * 检查是否包含 StatTag
	 * @param Tag 要检查的标签
	 * @return 是否至少有 1 个堆栈
	 *
	 * 用途：
	 *   - 检查是否有弹药：HasStatTag(Inventory.Ammo.Rifle)
	 *   - 检查是否有材料：HasStatTag(Inventory.Material.MilitaryAlloy)
	 *
	 * 性能：
	 *   - O(1)，通过 TagToCountMap 缓存查询
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory)
	bool HasStatTag(FGameplayTag Tag) const;

	/** 获取实例唯一 ID（QuickBar/SaveGame 使用） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=Inventory)
	FGuid GetItemInstanceId() const { return ItemInstanceId; }

	/** 获取物品生命周期（Persistent / RuntimeOnly） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=Inventory)
	EShootItemLifetime GetItemLifetime() const { return ItemLifetime; }

	/**
	 * 获取物品定义
	 * @return 物品定义类
	 *
	 * 用途：
	 *   - 查询物品的静态数据（名称、图标、Fragment 等）
	 *   - 判断物品类型
	 */
	TSubclassOf<UShootInventoryItemDefinition> GetItemDef() const
	{
		return ItemDef;
	}

	/** 获取用于 UI 的缓存显示名（来自 ItemDefinition） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=Inventory)
	FText GetItemDisplayName() const { return CachedDisplayName; }

	/** 获取用于 UI 的缓存图标（来自 ItemDefinition） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category=Inventory)
	const FSlateBrush& GetItemDisplayIcon() const { return CachedIcon; }

	/**
	 * 查找指定类型的 Fragment（蓝图版本）
	 * @param FragmentClass Fragment 类型
	 * @return 找到的 Fragment，未找到返回 nullptr
	 *
	 * 用途：
	 *   - 查询物品功能（如是否可装备、是否可消耗）
	 *   - 获取 Fragment 数据（如获取武器的配置 Fragment）
	 *
	 * 实现：
	 *   - 通过 ItemDef 的 CDO 查找 Fragment
	 *   - Fragment 存储在 Definition 中，Instance 不存储副本
	 *
	 * 元数据：
	 *   - DeterminesOutputType：蓝图编辑器根据 FragmentClass 推导返回类型
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, meta=(DeterminesOutputType=FragmentClass))
	const UShootInventoryItemFragment* FindFragmentByClass(TSubclassOf<UShootInventoryItemFragment> FragmentClass) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	void ApplyStatTagSnapshot(const FShootGameplayTagStackContainer& Snapshot);

	const FShootGameplayTagStackContainer& GetStatTags() const { return StatTags; }

	/**
	 * Combat/QuickBar 只监听已绑定的 ItemInstance，收到通知后重新查询弹匣和备弹。
	 * 委托不携带 UI 类型，InventoryManager 仍然只负责有身份物品实例。
	 */
	FShootItemStatTagsChanged& OnStatTagsChanged() { return StatTagsChanged; }

	/**
	 * 查找指定类型的 Fragment（C++ 模板版本）
	 * @return 找到的 Fragment，未找到返回 nullptr
	 *
	 * 用途：
	 *   - C++ 中使用，提供类型安全的查询
	 *
	 * 示例：
	 *   ```cpp
	 *   const UShootInventoryFragment_RangedWeaponConfig* RangedConfig =
	 *       Instance->FindFragmentByClass<UShootInventoryFragment_RangedWeaponConfig>();
	 *   if (RangedConfig)
	 *   {
	 *       // 这是一个远程武器；当前弹药和容量另由 ItemInstance StatTags 提供。
	 *   }
	 *   ```
	 */
	template <typename ResultClass>
	const ResultClass* FindFragmentByClass() const
	{
		return (ResultClass*)FindFragmentByClass(ResultClass::StaticClass());
	}

private:
#if UE_WITH_IRIS
	/**
	 * 注册 Iris 复制片段（UE5 新网络系统）
	 *
	 * 作用：
	 *   - Iris：UE5 的下一代网络复制系统
	 *   - 注册此对象的复制片段，支持 Iris 网络
	 */
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS

	/**
	 * 设置物品定义
	 * @param InDef 物品定义类
	 *
	 * 用途：
	 *   - 在 InventoryList::AddEntry 中调用
	 *   - 创建 Instance 后立即设置 ItemDef
	 *
	 * 权限：
	 *   - 私有：仅 FShootInventoryList 可以调用（friend 声明）
	 *   - 防止外部随意修改 ItemDef
	 */
	void SetItemDef(TSubclassOf<UShootInventoryItemDefinition> InDef);

	/**
	 * friend 声明：允许 InventoryList 访问私有成员
	 *
	 * 作用：
	 *   - InventoryList 需要调用 SetItemDef
	 *   - 保持封装性，其他类无法访问
	 */
	friend struct FShootInventoryList;

private:
	/**
	 * StatTag 堆栈容器
	 *
	 * 用途：
	 *   - 存储物品实例的动态状态（弹药、材料、耐久等）
	 *   - 使用 FShootGameplayTagStackContainer 实现
	 *
	 * 复制：
	 *   - 自动复制到客户端
	 *   - 使用 FastArray 增量复制，仅传输变化
	 *
	 * 示例数据：
	 *   - Inventory.Ammo.Rifle = 120
	 *   - Inventory.Material.MilitaryAlloy = 50
	 *   - Inventory.Durability = 80
	 */
	UPROPERTY(ReplicatedUsing=OnRep_StatTags)
	FShootGameplayTagStackContainer StatTags;

	/**
	 * 物品定义（指向静态数据）
	 *
	 * 用途：
	 *   - 关联物品的静态数据（DisplayName、Fragments 等）
	 *   - 通过 GetDefault<UShootInventoryItemDefinition>(ItemDef) 访问
	 *
	 * 复制：
	 *   - 复制类引用（TSubclassOf）
	 *   - 客户端通过 CDO 访问静态数据
	 */
	UPROPERTY(ReplicatedUsing=OnRep_ItemDef)
	TSubclassOf<UShootInventoryItemDefinition> ItemDef;

	/** 唯一实例 ID，QuickBar / SaveGame / RuntimeOnly 清理使用 */
	UPROPERTY(Replicated)
	FGuid ItemInstanceId;

	/** 物品生命周期（Persistent = 账号资产，RuntimeOnly = 副本临时） */
	UPROPERTY(Replicated)
	EShootItemLifetime ItemLifetime = EShootItemLifetime::Persistent;

	void InitializeInstanceLifecycle(EShootItemLifetime InLifetime, const FGuid& InInstanceId);

	void RefreshDisplayDataFromDefinition();

	UFUNCTION()
	void OnRep_ItemDef();

	UFUNCTION()
	void OnRep_StatTags();

	void BroadcastStatTagsChanged();

	FShootItemStatTagsChanged StatTagsChanged;

	UPROPERTY(BlueprintReadOnly, Transient, Category=Inventory, meta=(AllowPrivateAccess="true"))
	FText CachedDisplayName;

	UPROPERTY(BlueprintReadOnly, Transient, Category=Inventory, meta=(AllowPrivateAccess="true"))
	FSlateBrush CachedIcon;
};
