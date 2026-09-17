// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "AbilitySystem/ShootAbilitySet.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ShootEquipmentManagerComponent.generated.h"

class UShootAbilitySystemComponent;
class UShootEquipmentDefinition;
class UShootEquipmentInstance;
class UShootEquipmentManagerComponent;
struct FShootEquipmentList;
struct FNetDeltaSerializeInfo;
struct FReplicationFlags;

/**
 * FShootAppliedEquipmentEntry
 *
 * 【装备条目】单个已装备物品的簿记条目
 *
 * 作用：
 *   - 关联 EquipmentDefinition 和 EquipmentInstance
 *   - 跟踪授予的 AbilitySet Handles（用于卸载时移除）
 *   - 作为 FastArraySerializerItem，支持增量复制
 *
 * FastArray 增量复制：
 *   - 添加装备：PreReplicatedRemove 不调用
 *   - 添加装备：PostReplicatedAdd 调用 → 客户端创建 SpawnedActors
 *   - 修改装备：PostReplicatedChange 调用 → 客户端更新数据
 *   - 移除装备：PreReplicatedRemove 调用 → 客户端销毁 SpawnedActors
 *
 * 为什么需要 Entry：
 *   - 记录 EquipmentDefinition（知道装备来自哪个 Definition）
 *   - 记录 EquipmentInstance（实际的装备 UObject）
 *   - 记录 GrantedHandles（用于卸载时移除 Ability）
 *   - 这三者绑定在一起，方便管理
 */
USTRUCT(BlueprintType)
struct FShootAppliedEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FShootAppliedEquipmentEntry()
	{}

	/**
	 * 获取调试字符串
	 *
	 * 用于日志输出和调试
	 */
	FString GetDebugString() const;

private:
	friend FShootEquipmentList;
	friend UShootEquipmentManagerComponent;

	/**
	 * 装备定义
	 *
	 * 记录这个装备来自哪个 Definition
	 * 主要用于调试和日志
	 *
	 * 复制到客户端：是
	 */
	UPROPERTY()
	TSubclassOf<UShootEquipmentDefinition> EquipmentDefinition;

	/**
	 * 装备实例
	 *
	 * 实际的装备 UObject（如 UShootRangedWeaponInstance）
	 *
	 * 复制到客户端：是（通过 SubObject 机制）
	 *
	 * 重要性：
	 *   - 服务器和客户端都需要访问装备实例
	 *   - 客户端通过 Instance 访问武器数据（如扩散角度、Mesh 等）
	 */
	UPROPERTY()
	TObjectPtr<UShootEquipmentInstance> Instance = nullptr;

	/**
	 * 授予的 Ability Handles
	 *
	 * 装备时授予的 AbilitySet 句柄
	 * 用于卸载时移除 Ability
	 *
	 * 复制到客户端：否（NotReplicated）
	 *
	 * 为什么不复制：
	 *   - Ability 授予/移除只在服务器上执行
	 *   - AbilitySystemComponent 会自动复制 Ability Specs
	 *   - 客户端不需要知道 Handle（无法主动移除 Ability）
	 */
	UPROPERTY(NotReplicated)
	FShootAbilitySet_GrantedHandles GrantedHandles;
};

/**
 * FShootEquipmentList
 *
 * 【装备列表】管理所有已装备物品，支持网络复制
 *
 * 用途：
 *   - 使用 FastArray 增量复制已装备物品
 *   - 管理装备的添加和移除
 *   - 在复制钩子中触发 OnEquipped/OnUnequipped
 *
 * FastArray 增量复制机制：
 *   - 服务器修改 Entries（添加/移除）
 *   - FastArrayDeltaSerialize 计算增量
 *   - 仅发送变化的 Entry 到客户端
 *   - 客户端调用复制钩子（PreReplicatedRemove, PostReplicatedAdd, PostReplicatedChange）
 *
 * 为什么使用 FastArray 而不是 TArray：
 *   - 性能：仅复制变化的 Entry（不是整个数组）
 *   - 复制钩子：自动调用 Add/Remove/Change 回调
 *   - 稳定性：每个 Entry 有唯一 ID，不会因为数组重排导致错误
 *
 * 示例流程：
 *   ```
 *   [服务器]
 *   1. EquipmentList.AddEntry(EquipmentDef)
 *   2. 创建 Instance，生成 SpawnedActors，授予 Ability
 *   3. MarkItemDirty(NewEntry) → 触发复制
 *
 *   [网络传输]
 *   仅发送新增的 Entry（不是整个数组）
 *
 *   [客户端]
 *   4. PostReplicatedAdd(NewEntry)
 *   5. Instance->OnEquipped()（蓝图可响应）
 *   ```
 */
USTRUCT(BlueprintType)
struct FShootEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FShootEquipmentList()
		: OwnerComponent(nullptr)
	{
	}

	FShootEquipmentList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

public:
	// ========================================================================
	// FFastArraySerializer Contract
	// ========================================================================

	/**
	 * Entry 移除前回调（客户端）
	 *
	 * @param RemovedIndices 被移除的 Entry 索引
	 * @param FinalSize 移除后的数组大小
	 *
	 * 调用时机：客户端收到"移除装备"复制时
	 *
	 * 执行流程：
	 *   1. 遍历 RemovedIndices
	 *   2. 为每个移除的 Entry：
	 *      a. Instance->OnUnequipped()（通知卸载）
	 *      b. Instance->DestroyEquipmentActors()（销毁 Mesh 等）
	 *
	 * 重要性：
	 *   - 客户端需要销毁 SpawnedActors（避免泄漏）
	 *   - 客户端需要执行卸载逻辑（如播放音效、隐藏 UI）
	 */
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);

	/**
	 * Entry 添加后回调（客户端）
	 *
	 * @param AddedIndices 被添加的 Entry 索引
	 * @param FinalSize 添加后的数组大小
	 *
	 * 调用时机：客户端收到"添加装备"复制时
	 *
	 * 执行流程：
	 *   1. 遍历 AddedIndices
	 *   2. 为每个添加的 Entry：
	 *      a. Instance->SpawnEquipmentActors()（生成 Mesh 等）
	 *      b. Instance->OnEquipped()（通知装备）
	 *
	 * 重要性：
	 *   - 客户端需要生成 SpawnedActors（显示武器 Mesh）
	 *   - 客户端需要执行装备逻辑（如播放音效、显示 UI）
	 */
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);

	/**
	 * Entry 修改后回调（客户端）
	 *
	 * @param ChangedIndices 被修改的 Entry 索引
	 * @param FinalSize 修改后的数组大小
	 *
	 * 调用时机：客户端收到"修改装备"复制时
	 *
	 * 执行流程：
	 *   目前不执行特殊逻辑
	 *   未来可能用于：
	 *     - 更新武器皮肤
	 *     - 更新武器附件
	 */
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	// ========================================================================
	// NetDeltaSerialize
	// ========================================================================

	/**
	 * 自定义网络序列化
	 *
	 * FastArray 的核心：计算增量并序列化
	 * UE 网络系统会自动调用此方法
	 *
	 * @param DeltaParms 增量序列化参数
	 * @return 是否有数据需要复制
	 */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FShootAppliedEquipmentEntry, FShootEquipmentList>(Entries, DeltaParms, *this);
	}

	// ========================================================================
	// 装备管理（服务器）
	// ========================================================================

	/**
	 * 添加装备（服务器）
	 *
	 * @param EquipmentDefinition 装备定义
	 * @return 创建的装备实例
	 *
	 * 权限：仅服务器
	 *
	 * 执行流程：
	 *   1. 创建 EquipmentInstance
	 *      - 类型：EquipmentDefinition->InstanceType
	 *      - Outer：Pawn（重要！）
	 *   2. 生成 SpawnedActors
	 *      - 遍历 EquipmentDefinition->ActorsToSpawn
	 *      - SpawnActor 并附加到 Pawn
	 *   3. 授予 AbilitySets
	 *      - 遍历 EquipmentDefinition->AbilitySetsToGrant
	 *      - GiveToAbilitySystem(..., SourceObject=Instance)
	 *      - 记录 GrantedHandles（用于后续移除）
	 *   4. 调用 OnEquipped()
	 *      - Instance->OnEquipped()
	 *   5. 添加到 Entries 并标记为脏
	 *      - MarkItemDirty(NewEntry) → 触发网络复制
	 *
	 * 重要性：
	 *   - Outer 必须是 Pawn（SubObject 复制要求）
	 *   - SourceObject 设置为 Instance（Ability 可以访问武器实例）
	 *   - MarkItemDirty 触发复制到客户端
	 */
	UShootEquipmentInstance* AddEntry(TSubclassOf<UShootEquipmentDefinition> EquipmentDefinition, UObject* Instigator);

	/**
	 * 移除装备（服务器）
	 *
	 * @param Instance 要移除的装备实例
	 *
	 * 权限：仅服务器
	 *
	 * 执行流程：
	 *   1. 查找 Entry（通过 Instance）
	 *   2. 调用 OnUnequipped()
	 *      - Instance->OnUnequipped()
	 *   3. 移除 AbilitySets
	 *      - GrantedHandles.TakeFromAbilitySystem(ASC)
	 *   4. 销毁 SpawnedActors
	 *      - Instance->DestroyEquipmentActors()
	 *   5. 从 Entries 移除并标记为脏
	 *      - MarkArrayDirty() → 触发网络复制
	 *
	 * 重要性：
	 *   - 必须先移除 Ability，再销毁 Actor（避免悬空引用）
	 *   - MarkArrayDirty 触发复制到客户端
	 */
	void RemoveEntry(UShootEquipmentInstance* Instance);

private:
	/**
	 * 获取 ASC（用于授予/移除 Ability）
	 *
	 * @return OwnerComponent 的 Pawn 的 AbilitySystemComponent
	 */
	UShootAbilitySystemComponent* GetAbilitySystemComponent() const;

	friend UShootEquipmentManagerComponent;

private:
	/**
	 * 已装备条目数组
	 *
	 * FastArray 增量复制的核心数据
	 */
	UPROPERTY()
	TArray<FShootAppliedEquipmentEntry> Entries;

	/**
	 * 拥有者组件（不复制）
	 *
	 * 指向 EquipmentManagerComponent
	 * 用于访问 Pawn、ASC 等
	 */
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

/**
 * 注册 NetDeltaSerializer
 *
 * 告诉 UE 网络系统：FShootEquipmentList 使用自定义增量序列化
 * 必须在 .h 文件中定义（模板特化）
 */
template<>
struct TStructOpsTypeTraits<FShootEquipmentList> : public TStructOpsTypeTraitsBase2<FShootEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};

/**
 * UShootEquipmentManagerComponent
 *
 * 【装备管理组件】管理 Pawn 的装备系统
 *
 * 用途：
 *   - 装备/卸载物品
 *   - 管理装备列表（FastArray）
 *   - 处理 SubObject 复制
 *
 * 与 InventoryManager 的区别：
 *   - InventoryManager：附加到 PlayerController，管理所有物品（库存）
 *   - EquipmentManager：附加到 Pawn，管理当前装备的物品（装备）
 *
 * 所有权：
 *   - 拥有者：Pawn（AShootCharacter）
 *   - 复制：Server + Autonomous Proxy + Simulated Proxies
 *
 * 组件生命周期：
 *   ```
 *   1. Pawn 创建时：
 *      - 创建 EquipmentManagerComponent
 *      - SetIsReplicated(true)
 *   2. InitializeComponent()
 *      - 初始化 EquipmentList
 *   3. ReadyForReplication()
 *      - 注册现有 SubObject（如果有）
 *   4. 运行时：
 *      - EquipItem() / UnequipItem()
 *      - SubObject 复制到客户端
 *   5. UninitializeComponent()
 *      - 清理资源
 *   ```
 *
 * SubObject 复制机制：
 *   - UE5 新机制：RegisteredSubObjectList（推荐）
 *   - UE4 旧机制：ReplicateSubobjects（后备）
 *   - 我们同时实现两者，确保兼容性
 *
 * 示例用法：
 *   ```cpp
 *   // 装备武器
 *   UShootEquipmentInstance* EquippedWeapon = EquipmentManager->EquipItem(DA_Equipment_Rifle);
 *   EquippedWeapon->SetInstigator(ItemInstance); // 连接库存和装备
 *
 *   // 卸载武器
 *   EquipmentManager->UnequipItem(EquippedWeapon);
 *   ```
 */
UCLASS(MinimalAPI, BlueprintType)
class UShootEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UShootEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ========================================================================
	// 装备管理 API
	// ========================================================================

	/**
	 * 装备物品
	 *
	 * @param EquipmentDefinition 装备定义
	 * @return 创建的装备实例
	 *
	 * 权限：BlueprintAuthorityOnly（仅服务器）
	 *
	 * 执行流程：
	 *   1. 调用 EquipmentList.AddEntry(EquipmentDefinition)
	 *   2. 注册 SubObject（如果使用 RegisteredSubObjectList）
	 *   3. 返回 Instance
	 *
	 * 示例：
	 *   ```cpp
	 *   UShootEquipmentInstance* Weapon = EquipmentManager->EquipItem(DA_Equipment_Rifle);
	 *   Weapon->SetInstigator(ItemInstance);
	 *   ```
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UShootEquipmentInstance* EquipItem(TSubclassOf<UShootEquipmentDefinition> EquipmentDefinition, UObject* Instigator = nullptr);

	/**
	 * 卸载物品
	 *
	 * @param ItemInstance 要卸载的装备实例
	 *
	 * 权限：BlueprintAuthorityOnly（仅服务器）
	 *
	 * 执行流程：
	 *   1. 调用 EquipmentList.RemoveEntry(ItemInstance)
	 *   2. 取消注册 SubObject（如果使用 RegisteredSubObjectList）
	 *
	 * 示例：
	 *   ```cpp
	 *   EquipmentManager->UnequipItem(Weapon);
	 *   ```
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void UnequipItem(UShootEquipmentInstance* ItemInstance);

	// ========================================================================
	// UObject Interface
	// ========================================================================

	/**
	 * 手动复制 SubObject（旧机制，作为后备）
	 *
	 * UE4 使用此方法复制 SubObject
	 * UE5 推荐使用 RegisteredSubObjectList，但我们保留此方法作为后备
	 *
	 * @param Channel Actor 通道
	 * @param Bunch 输出 Bunch
	 * @param RepFlags 复制标志
	 * @return 是否有 SubObject 被复制
	 *
	 * 执行流程：
	 *   1. 遍历 EquipmentList.Entries
	 *   2. 为每个 Entry：
	 *      - Channel->ReplicateSubobject(Entry.Instance)
	 *   3. 返回 true（如果有 Instance）
	 */
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	// ========================================================================
	// UActorComponent Interface
	// ========================================================================

	/**
	 * 组件初始化
	 *
	 * 调用时机：BeginPlay 之前
	 *
	 * 执行流程：
	 *   1. 初始化 EquipmentList
	 *   2. 设置 OwnerComponent 引用
	 */
	virtual void InitializeComponent() override;

	/**
	 * 组件反初始化
	 *
	 * 调用时机：组件销毁时
	 *
	 * 执行流程：
	 *   1. 移除所有装备
	 *   2. 清理资源
	 */
	virtual void UninitializeComponent() override;

	/**
	 * 准备复制（注册已存在的 SubObject）
	 *
	 * 调用时机：组件准备好网络复制时
	 *
	 * 执行流程：
	 *   1. 遍历现有的 EquipmentInstance
	 *   2. 注册到 RegisteredSubObjectList
	 *
	 * 重要性：
	 *   - 确保所有现有的 Instance 都会被复制
	 *   - 通常在运行时动态创建的 Instance 会立即注册
	 *   - 但如果有预先存在的 Instance（如关卡中放置的），需要在这里注册
	 */
	virtual void ReadyForReplication() override;

	// ========================================================================
	// 查询 API
	// ========================================================================

	/**
	 * 获取第一个指定类型的装备实例
	 *
	 * @param InstanceType 装备实例类型（如 UShootRangedWeaponInstance::StaticClass()）
	 * @return 找到的实例，未找到返回 nullptr
	 *
	 * 示例：
	 *   ```cpp
	 *   UShootRangedWeaponInstance* Weapon = Cast<UShootRangedWeaponInstance>(
	 *       EquipmentManager->GetFirstInstanceOfType(UShootRangedWeaponInstance::StaticClass())
	 *   );
	 *   ```
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UShootEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UShootEquipmentInstance> InstanceType);

	/**
	 * 获取所有指定类型的装备实例
	 *
	 * @param InstanceType 装备实例类型
	 * @return 找到的实例数组
	 *
	 * 用途：
	 *   - 双持武器：获取所有武器实例
	 *   - 多件护甲：获取所有护甲实例
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<UShootEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<UShootEquipmentInstance> InstanceType) const;

	/**
	 * 获取第一个指定类型的装备实例（C++ 模板版本）
	 *
	 * 类型安全，不需要 Cast
	 *
	 * 示例：
	 *   ```cpp
	 *   UShootRangedWeaponInstance* Weapon = EquipmentManager->GetFirstInstanceOfType<UShootRangedWeaponInstance>();
	 *   ```
	 */
	template <typename T>
	T* GetFirstInstanceOfType()
	{
		return (T*)GetFirstInstanceOfType(T::StaticClass());
	}

private:
	/**
	 * 装备列表
	 *
	 * FastArray 增量复制
	 */
	UPROPERTY(Replicated)
	FShootEquipmentList EquipmentList;
};
