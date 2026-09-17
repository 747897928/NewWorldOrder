// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "ShootEquipmentInstance.generated.h"

class AActor;
class APawn;
struct FShootEquipmentActorToSpawn;

/**
 * UShootEquipmentInstance
 *
 * 【装备实例】运行时的装备实例，管理生成的 Actor 和装备状态
 *
 * 用途：
 *   - 管理装备的生命周期（装备/卸载）
 *   - 生成和销毁 SpawnedActors（如武器 Mesh）
 *   - 提供访问拥有者 Pawn 的接口
 *   - 连接库存系统和装备系统（通过 Instigator）
 *
 * 生命周期：
 *   ```
 *   1. EquipmentManager->EquipItem(EquipmentDefinition)
 *      ↓
 *   2. 创建 Instance：NewObject<EquipmentInstance>(Pawn, EquipmentDefinition->InstanceType)
 *      ↓
 *   3. 设置 Instigator（ItemInstance） ← 重要！连接库存和装备
 *      ↓
 *   4. SpawnEquipmentActors(EquipmentDefinition->ActorsToSpawn)
 *      ↓
 *   5. 授予 AbilitySets（SourceObject = this） ← 重要！Ability 可以访问武器实例
 *      ↓
 *   6. OnEquipped() ← 子类可以重写，实现装备逻辑
 *      ↓
 *   7. ...使用中...
 *      ↓
 *   8. EquipmentManager->UnequipItem(Instance)
 *      ↓
 *   9. OnUnequipped() ← 子类可以重写，实现卸载逻辑
 *      ↓
 *  10. 移除 AbilitySets
 *      ↓
 *  11. DestroyEquipmentActors()
 *      ↓
 *  12. 从 EquipmentList 移除 Entry
 *   ```
 *
 * 网络复制：
 *   - 作为 SubObject 复制（通过 EquipmentManagerComponent）
 *   - Instigator 自动复制（客户端可以访问 ItemInstance）
 *   - SpawnedActors 自动复制（Actor 本身会复制）
 *
 * Instigator 的作用：
 *   - 连接库存和装备系统
 *   - 对于武器，Instigator 是 ItemInstance
 *   - 通过 Instigator 访问弹药（ItemInstance->StatTags）
 *   - 示例：
 *     ```cpp
 *     UShootInventoryItemInstance* ItemInstance = Cast<UShootInventoryItemInstance>(GetInstigator());
 *     int32 Ammo = ItemInstance->GetStatTagStackCount(Tag_Weapon_Ammo_Magazine);
 *     ```
 *
 * 为什么使用 UObject 而不是 Actor：
 *   - 性能：UObject 更轻量，不需要 Tick、Transform、Collision 等
 *   - SubObject 复制：UE5 支持 UObject 作为 SubObject 复制（RegisteredSubObjectList）
 *   - 逻辑与表现分离：EquipmentInstance (UObject) 存储逻辑，SpawnedActors 负责表现
 *   - 网络带宽：只复制必要的逻辑数据，Mesh 等表现由 SpawnedActors 处理
 */
UCLASS(BlueprintType, Blueprintable)
class UShootEquipmentInstance : public UObject
{
	GENERATED_BODY()

public:
	UShootEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// ========================================================================
	// UObject Interface
	// ========================================================================

	/**
	 * 启用网络支持
	 *
	 * 返回 true 允许此 UObject 通过网络复制
	 * 必须重写此方法，否则 UObject 不会被复制
	 */
	virtual bool IsSupportedForNetworking() const override { return true; }

	/**
	 * 获取 World（从 Outer Pawn 获取）
	 *
	 * UObject 没有直接的 World 引用，需要从 Outer 获取
	 * EquipmentInstance 的 Outer 是 Pawn，所以从 Pawn 获取 World
	 *
	 * 为什么需要：
	 *   - SpawnActor 需要 World
	 *   - 各种系统调用需要 World（如 Timer、GameplayMessage 等）
	 */
	virtual UWorld* GetWorld() const override final;

	// ========================================================================
	// Instigator（连接库存系统）
	// ========================================================================

	/**
	 * 获取 Instigator（触发装备的对象）
	 *
	 * 对于武器，Instigator 是 ItemInstance
	 * 可以通过 Instigator 访问弹药等数据
	 *
	 * 示例：
	 *   ```cpp
	 *   UShootInventoryItemInstance* ItemInstance = Cast<UShootInventoryItemInstance>(GetInstigator());
	 *   if (ItemInstance)
	 *   {
	 *       int32 Ammo = ItemInstance->GetStatTagStackCount(Tag_Weapon_Ammo_Magazine);
	 *   }
	 *   ```
	 */
	UFUNCTION(BlueprintPure, Category=Equipment)
	UObject* GetInstigator() const { return Instigator; }

	/**
	 * 设置 Instigator
	 *
	 * 调用时机：EquipmentManager->EquipItem() 中
	 * 通常在 QuickBar 中调用：
	 *   ```cpp
	 *   UShootEquipmentInstance* EquippedItem = EquipmentManager->EquipItem(EquipmentDef);
	 *   EquippedItem->SetInstigator(ItemInstance); // 连接库存和装备
	 *   ```
	 */
	void SetInstigator(UObject* InInstigator) { Instigator = InInstigator; }

	// ========================================================================
	// Pawn Access（访问拥有者）
	// ========================================================================

	/**
	 * 获取拥有者 Pawn
	 *
	 * EquipmentInstance 的 Outer 是 Pawn
	 * 此方法提供便捷访问
	 *
	 * 返回值：
	 *   - 如果 Outer 是 Pawn，返回 Pawn
	 *   - 否则返回 nullptr
	 */
	UFUNCTION(BlueprintPure, Category=Equipment)
	APawn* GetPawn() const;

	/**
	 * 获取拥有者 Pawn（带类型转换）
	 *
	 * 蓝图友好版本，允许指定 Pawn 类型
	 *
	 * @param PawnType 期望的 Pawn 类型（如 AShootCharacter）
	 * @return 转换后的 Pawn，失败返回 nullptr
	 *
	 * 示例：
	 *   ```blueprint
	 *   GetTypedPawn(AShootCharacter) → AShootCharacter*
	 *   ```
	 */
	UFUNCTION(BlueprintPure, Category=Equipment, meta=(DeterminesOutputType=PawnType))
	APawn* GetTypedPawn(TSubclassOf<APawn> PawnType) const;

	// ========================================================================
	// SpawnedActors（生成的 Actor 管理）
	// ========================================================================

	/**
	 * 获取生成的 Actor
	 *
	 * 返回装备时生成的所有 Actor（如武器 Mesh）
	 *
	 * 用途：
	 *   - 蓝图中访问武器 Mesh（设置材质、播放动画等）
	 *   - C++ 中访问特定 Actor（如枪口位置）
	 */
	UFUNCTION(BlueprintPure, Category=Equipment)
	TArray<AActor*> GetSpawnedActors() const { return SpawnedActors; }

	/**
	 * 生成装备 Actor
	 *
	 * @param ActorsToSpawn Actor 配置列表（来自 EquipmentDefinition）
	 *
	 * 调用时机：EquipmentManager->EquipItem() 中
	 *
	 * 执行流程：
	 *   1. 遍历 ActorsToSpawn
	 *   2. 为每个配置：
	 *      a. SpawnActor(ActorToSpawn)
	 *      b. AttachToComponent(OwnerMesh, AttachSocket, AttachTransform)
	 *      c. 添加到 SpawnedActors 数组
	 *
	 * 网络：
	 *   - 仅在服务器上执行生成
	 *   - Actor 本身会自动复制到客户端
	 *   - SpawnedActors 数组会复制，客户端可以访问
	 */
	virtual void SpawnEquipmentActors(const TArray<FShootEquipmentActorToSpawn>& ActorsToSpawn);

	/**
	 * 销毁装备 Actor
	 *
	 * 调用时机：EquipmentManager->UnequipItem() 中
	 *
	 * 执行流程：
	 *   1. 遍历 SpawnedActors
	 *   2. 为每个 Actor：
	 *      a. Actor->Destroy()
	 *   3. 清空 SpawnedActors 数组
	 *
	 * 网络：
	 *   - 仅在服务器上执行销毁
	 *   - Actor 销毁会自动复制到客户端
	 */
	virtual void DestroyEquipmentActors();

	// ========================================================================
	// 装备生命周期回调
	// ========================================================================

	/**
	 * 装备时回调
	 *
	 * 调用时机：EquipmentManager->EquipItem() 最后调用
	 *
	 * 子类可以重写，实现装备特定逻辑：
	 *   - 武器：初始化扩散、设置默认瞄准状态
	 *   - 护甲：应用护甲 Buff
	 *   - 工具：初始化工具状态
	 *
	 * 示例：
	 *   ```cpp
	 *   void UShootRangedWeaponInstance::OnEquipped()
	 *   {
	 *       Super::OnEquipped();
	 *       CurrentSpreadAngle = 0.0f; // 重置扩散
	 *       CurrentHeat = 0.0f; // 重置热量
	 *   }
	 *   ```
	 */
	virtual void OnEquipped();

	/**
	 * 卸载时回调
	 *
	 * 调用时机：EquipmentManager->UnequipItem() 第一步调用
	 *
	 * 子类可以重写，实现卸载特定逻辑：
	 *   - 武器：保存当前弹药（如果需要）
	 *   - 护甲：移除护甲 Buff
	 *   - 工具：清理工具状态
	 *
	 * 示例：
	 *   ```cpp
	 *   void UShootRangedWeaponInstance::OnUnequipped()
	 *   {
	 *       Super::OnUnequipped();
	 *       // 清理瞄准状态、停止射击等
	 *   }
	 *   ```
	 */
	virtual void OnUnequipped();

protected:
	/**
	 * Instigator 已复制到客户端后的子类扩展点。
	 * EquipmentList 的 FastArray 与装备子对象属性分别复制，二者回调顺序没有保证；
	 * 子类可在这里补齐依赖 ItemInstance / ItemDefinition 的客户端表现。
	 */
	virtual void OnInstigatorReplicated();

#if UE_WITH_IRIS
	/**
	 * 注册 Iris 复制片段（UE5 新网络系统）
	 *
	 * Iris 是 UE5 的新网络复制系统
	 * 如果项目使用 Iris，需要注册复制片段
	 */
	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;
#endif // UE_WITH_IRIS

	/**
	 * 蓝图可实现的装备回调
	 *
	 * 允许蓝图在装备时执行逻辑
	 * 例如：播放装备音效、显示 UI 提示等
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnEquipped"))
	void K2_OnEquipped();

	/**
	 * 蓝图可实现的卸载回调
	 *
	 * 允许蓝图在卸载时执行逻辑
	 * 例如：播放卸载音效、隐藏 UI 等
	 */
	UFUNCTION(BlueprintImplementableEvent, Category=Equipment, meta=(DisplayName="OnUnequipped"))
	void K2_OnUnequipped();

private:
	/**
	 * Instigator 复制回调
	 *
	 * 当 Instigator 在客户端复制时调用，并转发给 OnInstigatorReplicated 扩展点。
	 */
	UFUNCTION()
	void OnRep_Instigator();

private:
	/**
	 * Instigator（触发装备的对象）
	 *
	 * 对于武器：ItemInstance
	 * 通过 Instigator 访问弹药和其他库存数据
	 *
	 * 网络复制：使用 ReplicatedUsing 自动复制到客户端
	 */
	UPROPERTY(ReplicatedUsing=OnRep_Instigator)
	TObjectPtr<UObject> Instigator;

	/**
	 * 生成的 Actor 列表
	 *
	 * 装备时生成的所有 Actor（如武器 Mesh、特效等）
	 *
	 * 网络复制：
	 *   - Actor 引用会复制
	 *   - Actor 本身有自己的网络复制机制
	 *   - 客户端可以通过此数组访问复制的 Actor
	 */
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> SpawnedActors;
};
