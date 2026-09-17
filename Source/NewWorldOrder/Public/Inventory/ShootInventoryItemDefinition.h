// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "Styling/SlateBrush.h"
#include "ShootInventoryItemDefinition.generated.h"

template <typename T> class TSubclassOf;

class UShootInventoryItemInstance;
class UShootInventoryManagerComponent;
struct FFrame;

/**
 * UShootInventoryItemFragment
 *
 * 【功能片段基类】物品定义的功能片段，使用组合模式扩展物品功能
 *
 * 设计目的：
 *   - 避免继承爆炸：不需要为每种物品组合创建新的 Definition 子类
 *   - 灵活组合：一个物品可以包含多个 Fragment（如武器 = WeaponData + SetBonus + QuestItem）
 *   - 可复用：Fragment 可以在多个物品间共享（如所有步枪共享 RifleAmmoFragment）
 *
 * 关键特性：
 *   - DefaultToInstanced：每个 Definition 拥有自己的 Fragment 实例（非共享）
 *   - EditInlineNew：可以在 Definition 的编辑器中直接创建和编辑 Fragment
 *   - Abstract：必须继承使用，不能直接创建 Fragment 基类实例
 *
 * 生命周期：
 *   - 在编辑器中作为 Definition 的内联对象创建
 *   - 运行时当 ItemInstance 创建时，调用 OnInstanceCreated 进行初始化
 *
 * 扩展点：
 *   - OnInstanceCreated：当物品实例被创建时调用，用于初始化 Instance 的状态
 *     例如：WeaponFragment 可以在此设置初始弹药数量到 StatTags
 *
 * 使用场景：
 *   - 武器：WeaponBasicConfig/RangedConfig/ProjectileConfig 等 Fragment 提供静态配置
 *   - 消耗品：存储使用效果、冷却时间等
 *   - 制作配方：存储材料需求、产物等
 *   - UShootInventoryItemFragment_SetBonus：存储套装奖励数据
 */
UCLASS(MinimalAPI, DefaultToInstanced, EditInlineNew, Abstract)
class UShootInventoryItemFragment : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 当物品实例创建时调用
	 * @param Instance 新创建的物品实例
	 *
	 * 用途：
	 *   - 初始化 Instance 的 StatTags（如设置初始弹药数量）
	 *   - 添加 Instance 的动态属性
	 *   - 执行 Fragment 特定的初始化逻辑
	 *
	 * 时机：
	 *   - 在 InventoryManagerComponent::AddItemDefinition 中调用
	 *   - 在 InventoryList::AddEntry 创建 Instance 后立即调用
	 *
	 * 注意：
	 *   - 此函数在服务器上调用（仅权威端创建 Instance）
	 *   - const 标记：Fragment 本身是只读的，不能修改 Fragment 数据
	 */
	virtual void OnInstanceCreated(UShootInventoryItemInstance* Instance) const {}
};

/**
 * UShootInventoryItemDefinition
 *
 * 【物品定义】静态的物品数据资产（DataAsset），定义物品的固有属性
 *
 * 设计目的：
 *   - 定义与实例分离：Definition 是静态的（Const），Instance 是动态的
 *   - 数据资产形式：在编辑器中创建 Blueprint 子类，配置数据
 *   - Fragment 组合：通过添加不同 Fragment 实现功能扩展
 *
 * 关键特性：
 *   - Blueprintable：可以在编辑器中创建 Blueprint 子类
 *   - Const：运行时不可修改（所有实例共享同一个 CDO）
 *   - Abstract：必须继承使用，不能直接创建基类实例
 *
 * 数据结构：
 *   - DisplayName：显示名称（UI 显示）
 *   - Fragments：功能片段数组（组合模式）
 *
 * 创建流程：
 *   1. 在编辑器中创建 Blueprint 子类（如 BP_Item_MilitaryAlloy）
 *   2. 设置 DisplayName（如"军用合金"）
 *   3. 添加 Fragment（如 MaterialFragment，设置堆叠上限 999）
 *   4. 保存为 DataAsset
 *
 * 使用流程：
 *   1. 调用 InventoryManager->AddItemDefinition(ItemDefClass, Count)
 *   2. InventoryManager 创建 ItemInstance
 *   3. 遍历所有 Fragment，调用 OnInstanceCreated 初始化 Instance
 *   4. 返回 Instance 给调用者
 *
 * 与 Instance 的关系：
 *   - Definition：静态数据，所有实例共享（如"AK47 的基础属性"）
 *   - Instance：动态状态，每个实例独立（如"玩家背包中的 AK47，当前耐久度 80%"）
 *
 * 扩展场景：
 *   - 材料物品：DisplayName="军用合金" + MaterialFragment(MaxStack=999)
 *   - 武器物品：DisplayName="AK47" + WeaponFragment(WeaponDef, InitialAmmo) + SetBonusFragment
 *   - 消耗品：DisplayName="医疗包" + ConsumableFragment(HealAmount, Cooldown)
 *   - 制作配方：DisplayName="传说武器配方" + CraftingRecipeFragment(Materials, Product)
 */
UCLASS(Blueprintable, Const, Abstract)
class UShootInventoryItemDefinition : public UObject
{
	GENERATED_BODY()

public:
	UShootInventoryItemDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 显示名称
	 *
	 * 用途：
	 *   - UI 显示（背包、拾取提示、制作界面等）
	 *   - 调试输出
	 *
	 * 示例：
	 *   - "军用合金"
	 *   - "AK47"
	 *   - "医疗包"
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FText DisplayName;

	/** UI 图标，用于 QuickBar/背包等界面展示 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display)
	FSlateBrush Icon;

	/**
	 * 功能片段数组
	 *
	 * 用途：
	 *   - 组合模式：通过添加不同 Fragment 实现功能扩展
	 *   - 可复用：Fragment 可以在多个物品间共享配置
	 *
	 * 特性：
	 *   - Instanced：每个 Definition 拥有自己的 Fragment 实例副本
	 *   - EditInlineNew：可以在编辑器中直接创建和编辑
	 *
	 * 示例：
	 *   - 武器物品：[WeaponFragment, SetBonusFragment]
	 *   - 材料物品：[MaterialFragment]
	 *   - 消耗品：[ConsumableFragment, QuestItemFragment]
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Display, Instanced)
	TArray<TObjectPtr<UShootInventoryItemFragment>> Fragments;

public:
	/**
	 * 查找指定类型的 Fragment
	 * @param FragmentClass Fragment 类型
	 * @return 找到的 Fragment，未找到返回 nullptr
	 *
	 * 用途：
	 *   - 查询物品是否具有某种功能（如是否可装备、是否可消耗）
	 *   - 获取 Fragment 数据（如获取武器的配置 Fragment）
	 *
	 * 示例：
	 *   ```cpp
	 *   const auto* RangedCfg = Cast<UShootInventoryFragment_RangedWeaponConfig>(
	 *           ItemDef->FindFragmentByClass(UShootInventoryFragment_RangedWeaponConfig::StaticClass()));
	 *   if (RangedCfg)
	 *   {
	 *       // 这是一个远程武器，可以获取射程和动画配置。
	 *       // 当前弹药与容量属于 ItemInstance StatTags；伤害 GE 属于 WeaponInstance。
	 *   }
	 *   ```
	 *
	 * 性能：
	 *   - O(n)，n = Fragment 数量（通常很小，<= 5）
	 */
	const UShootInventoryItemFragment* FindFragmentByClass(TSubclassOf<UShootInventoryItemFragment> FragmentClass) const;
};

/**
 * UShootInventoryFunctionLibrary
 *
 * 【蓝图函数库】提供蓝图可调用的库存相关工具函数
 *
 * 设计目的：
 *   - 为蓝图提供便捷的 Fragment 查询接口
 *   - 避免在蓝图中手动调用 GetDefaultObject + FindFragmentByClass
 *
 * 使用场景：
 *   - 在蓝图中查询物品是否具有某种功能
 *   - 在蓝图中获取 Fragment 数据
 *
 * @TODO: 是否改为 Subsystem？
 *   - 优点：可以缓存查询结果，支持更复杂的逻辑
 *   - 缺点：增加系统复杂度
 */
UCLASS()
class UShootInventoryFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/**
	 * 查找物品定义的指定 Fragment
	 * @param ItemDef 物品定义类
	 * @param FragmentClass Fragment 类型
	 * @return 找到的 Fragment，未找到返回 nullptr
	 *
	 * 蓝图示例：
	 *   - 输入：ItemDef = BP_Item_AK47, FragmentClass = BP_Fragment_Weapon
	 *   - 输出：WeaponFragment（类型由 DeterminesOutputType 推导）
	 *
	 * 元数据：
	 *   - DeterminesOutputType：告知蓝图编辑器根据 FragmentClass 推导返回类型
	 */
	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType=FragmentClass))
	static const UShootInventoryItemFragment* FindItemDefinitionFragment(TSubclassOf<UShootInventoryItemDefinition> ItemDef, TSubclassOf<UShootInventoryItemFragment> FragmentClass);

public:
	/**
	 * 根据上下文对象（Actor / 组件 / PlayerState / Controller）查找对应玩家的 InventoryManager
	 *
	 * 支持的输入：
	 * - AShootPlayerState / PlayerState 子类
	 * - APawn / ACharacter（内部调用 GetPlayerState）
	 * - AController（内部调用 GetPlayerState）
	 * - UActorComponent（取 Owner 再递归查找）
	 */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(DefaultToSelf="WorldContextObject"))
	static UShootInventoryManagerComponent* GetInventoryManager(const UObject* WorldContextObject);
};
