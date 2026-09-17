// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "ShootInventoryItemDefinition_Weapon.generated.h"

/**
 * EShootWeaponItemRarity
 *
 * 武器物品稀有度（账号层静态信息，仅用于 UI / 排序）
 */
UENUM(BlueprintType)
enum class EShootWeaponItemRarity : uint8
{
	Common,
	Rare,
	Epic,
	Legendary,
	Mythic
};

/**
 * EShootWeaponItemCategory
 *
 * 武器物品所属大类，用于 Hub 背包筛选与制作面板
 */
UENUM(BlueprintType)
enum class EShootWeaponItemCategory : uint8
{
	Rifle,
	Sniper,
	Shotgun,
	RocketLauncher,
	GrenadeLauncher,
	Special,
	/** 追加在末尾以保持现有枚举序号稳定。 */
	Pistol
};

/**
 * UShootInventoryItemDefinition_Weapon
 *
 * 账号背包中的“武器类型”定义，挂载全部 Fragment 配置（基础信息、射击配置、投射物配置）
 * 作为 Inventory → Equipment → Weapon 架构的入口，不再依赖 UWeaponDefinition
 */
UCLASS(BlueprintType)
class UShootInventoryItemDefinition_Weapon : public UShootInventoryItemDefinition
{
	GENERATED_BODY()

public:
	UShootInventoryItemDefinition_Weapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 武器分类（步枪/狙击/霰弹等），供 UI 与制作过滤 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	EShootWeaponItemCategory WeaponCategory = EShootWeaponItemCategory::Rifle;

	/** 稀有度（影响 UI 配色/掉落展示） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	EShootWeaponItemRarity Rarity = EShootWeaponItemRarity::Common;

	/** 简要描述（用于拾取提示/背包详情） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon")
	FText ShortDescription;

	// 武器的基础、远程和投射物数值只允许配置为父类 Fragments[] 中的实际 Fragment 实例。
	// 不保留“Fragment 类”平行字段，避免配置看似有效却永远不参与 WeaponInstance 读取。
};
