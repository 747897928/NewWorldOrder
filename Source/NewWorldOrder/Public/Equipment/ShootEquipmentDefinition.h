// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "ShootEquipmentDefinition.generated.h"

class AActor;
class UShootAbilitySet;
class UShootEquipmentInstance;

/**
 * FShootEquipmentActorToSpawn
 *
 * 【装备生成 Actor 配置】定义装备时要生成的 Actor 及其附加方式
 *
 * 用途：
 *   - 配置要生成的 Actor 类（如武器 Mesh、特效等）
 *   - 指定附加到哪个 Socket
 *   - 设置附加变换（位置、旋转、缩放）
 *
 * 示例：
 *   - ActorToSpawn: BP_WeaponMesh_Rifle
 *   - AttachSocket: hand_r_socket
 *   - AttachTransform: (Location=(X=0, Y=0, Z=0), Rotation=(Pitch=0, Yaw=0, Roll=0))
 *
 * 工作流程：
 *   1. EquipmentManager->EquipItem()
 *   2. EquipmentInstance->SpawnEquipmentActors()
 *   3. SpawnActor(ActorToSpawn)
 *   4. Actor->AttachToComponent(OwnerMesh, AttachSocket, AttachTransform)
 */
USTRUCT(BlueprintType)
struct FShootEquipmentActorToSpawn
{
	GENERATED_BODY()

	/**
	 * 要生成的 Actor 类
	 *
	 * 通常是武器 Mesh Actor：
	 *   - BP_WeaponMesh_Rifle：步枪模型
	 *   - BP_WeaponMesh_Shotgun：霰弹枪模型
	 *   - BP_MuzzleFlashEffect：枪口特效（可选）
	 */
	UPROPERTY(EditAnywhere, Category=Equipment)
	TSubclassOf<AActor> ActorToSpawn;

	/**
	 * 附加到的 Socket 名称
	 *
	 * 常用 Socket：
	 *   - hand_r_socket：右手（第一人称）
	 *   - hand_l_socket：左手（双持武器）
	 *   - weapon_muzzle：枪口（特效）
	 */
	UPROPERTY(EditAnywhere, Category=Equipment)
	FName AttachSocket;

	/**
	 * 附加变换
	 *
	 * 相对于 Socket 的位置、旋转、缩放
	 * 用于微调武器在手中的位置
	 */
	UPROPERTY(EditAnywhere, Category=Equipment)
	FTransform AttachTransform;
};

/**
 * UShootEquipmentDefinition
 *
 * 【装备定义】定义装备的类型、能力和生成的 Actor
 *
 * 用途：
 *   - 指定装备实例类型（如 UShootRangedWeaponInstance）
 *   - 定义装备时授予的 AbilitySet（开火、装填等）
 *   - 配置要生成的 Actor（如武器 Mesh）
 *
 * 与 ItemDefinition 的关系：
 *   - ItemDefinition：定义库存中的物品（名称、图标、Fragment）
 *   - EquipmentDefinition：定义装备时的行为（Ability、Mesh）
 *   - 桥接：ItemDefinition 包含 EquippableItem Fragment，Fragment 引用此 EquipmentDefinition
 *
 * 装备流程：
 *   ```
 *   1. 玩家拥有 ItemInstance（在 InventoryManager 中）
 *   2. ItemInstance->ItemDef 包含 EquippableItem Fragment
 *   3. Fragment->EquipmentDefinition 指向此 Definition
 *   4. QuickBar->EquipItemInSlot()
 *      → EquipmentManager->EquipItem(EquipmentDefinition)
 *      → 创建 EquipmentInstance（类型 = InstanceType）
 *      → 生成 SpawnedActors
 *      → 授予 AbilitySets
 *   5. 玩家现在可以使用装备的 Ability（开火、装填等）
 *   ```
 *
 * 设计优势：
 *   - 数据驱动：不同武器只需创建新的 DataAsset
 *   - 解耦：装备逻辑与库存逻辑分离
 *   - 可扩展：未来可以添加护甲、工具等装备
 *
 * 示例用法：
 *   创建 DataAsset：DA_Equipment_Rifle
 *   ```
 *   InstanceType: UShootRangedWeaponInstance
 *   AbilitySetsToGrant:
 *     [0]: AS_Rifle (包含 GA_Fire, GA_Reload, GA_ToggleFireMode)
 *   ActorsToSpawn:
 *     [0]:
 *       ActorToSpawn: BP_WeaponMesh_Rifle
 *       AttachSocket: hand_r_socket
 *       AttachTransform: (Location=(X=0, Y=0, Z=0), Rotation=...)
 *   ```
 */
UCLASS(Blueprintable, Const, Abstract, BlueprintType)
class UShootEquipmentDefinition : public UObject
{
	GENERATED_BODY()

public:
	UShootEquipmentDefinition(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 装备实例类型
	 *
	 * 指定创建哪种装备实例：
	 *   - UShootRangedWeaponInstance：远程武器（步枪、霰弹枪、狙击枪等）
	 *   - UShootMeleeWeaponInstance：近战武器（未来实现）
	 *   - UShootArmorInstance：护甲（未来实现）
	 *
	 * 重要性：
	 *   - EquipmentManager 使用此类型创建实例
	 *   - 不同武器类型有不同的实例类（提供不同的功能）
	 *
	 * 示例：
	 *   - 步枪、手枪、狙击枪：UShootRangedWeaponInstance
	 *   - 霰弹枪（如果需要特殊逻辑）：UShootShotgunWeaponInstance（继承自 RangedWeaponInstance）
	 */
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TSubclassOf<UShootEquipmentInstance> InstanceType;

	/**
	 * 装备时授予的 AbilitySet
	 *
	 * AbilitySet 包含一组 Gameplay Ability：
	 *   - 开火 Ability（GA_Weapon_Fire）
	 *   - 装填 Ability（GA_Weapon_Reload）
	 *   - 切换射击模式 Ability（GA_Weapon_ToggleFireMode）
	 *   - 瞄准 Ability（GA_Weapon_Aim，如果有）
	 *
	 * 工作流程：
	 *   1. 装备武器时：EquipmentManager 授予所有 AbilitySet
	 *   2. 使用期间：玩家可以激活这些 Ability
	 *   3. 卸载武器时：EquipmentManager 移除所有 AbilitySet
	 *
	 * 为什么使用 AbilitySet：
	 *   - 相关 Ability 打包在一起（易于管理）
	 *   - 可以复用（步枪和手枪可能共享相同的 AbilitySet）
	 *   - 数据驱动（在编辑器中配置，无需代码）
	 *
	 * 示例：
	 *   - AS_Rifle：包含步枪的所有 Ability
	 *   - AS_Shotgun：包含霰弹枪的所有 Ability
	 */
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<TObjectPtr<const UShootAbilitySet>> AbilitySetsToGrant;

	/**
	 * 装备时生成的 Actor
	 *
	 * 通常包含：
	 *   - 武器 Mesh Actor：显示在角色手中的 3D 模型
	 *   - 特效 Actor（可选）：枪口光晕、瞄准激光等
	 *
	 * 工作流程：
	 *   1. 装备武器时：
	 *      → 遍历 ActorsToSpawn
	 *      → SpawnActor(ActorToSpawn)
	 *      → AttachToComponent(OwnerMesh, AttachSocket, AttachTransform)
	 *      → 记录到 EquipmentInstance->SpawnedActors
	 *   2. 卸载武器时：
	 *      → 遍历 SpawnedActors
	 *      → Actor->Destroy()
	 *
	 * 为什么使用 SpawnedActors 而不是组件：
	 *   - 灵活性：不同武器可以有不同数量的 Actor
	 *   - 网络复制：Actor 自动复制（UObject 需要手动复制）
	 *   - 物理交互：可以添加物理（掉落武器、投掷手榴弹等）
	 *
	 * 示例：
	 *   步枪装备：
	 *     [0]:
	 *       ActorToSpawn: BP_WeaponMesh_Rifle
	 *       AttachSocket: hand_r_socket
	 *       AttachTransform: (Location=(X=5, Y=2, Z=-3), Rotation=(Pitch=0, Yaw=90, Roll=0))
	 *
	 *   双持手枪装备：
	 *     [0]: 右手手枪
	 *       ActorToSpawn: BP_WeaponMesh_Pistol
	 *       AttachSocket: hand_r_socket
	 *     [1]: 左手手枪
	 *       ActorToSpawn: BP_WeaponMesh_Pistol
	 *       AttachSocket: hand_l_socket
	 */
	UPROPERTY(EditDefaultsOnly, Category=Equipment)
	TArray<FShootEquipmentActorToSpawn> ActorsToSpawn;
};
