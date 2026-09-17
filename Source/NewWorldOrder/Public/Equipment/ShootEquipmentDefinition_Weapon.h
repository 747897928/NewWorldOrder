// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Equipment/ShootEquipmentDefinition.h"
#include "ShootEquipmentDefinition_Weapon.generated.h"

/**
 * UShootEquipmentDefinition_Weapon
 *
 * Lyra 架构下的武器装备定义，只负责提供默认 WeaponInstance 类型。
 * 装备/卸下角色动画由 ItemDefinition 的 RangedWeaponConfig.CharacterMontages 按当前角色
 * Skeleton 选择，不能在 EquipmentDefinition 再保存一份没有消费者的 Montage 引用。
 */
UCLASS(BlueprintType)
class UShootEquipmentDefinition_Weapon : public UShootEquipmentDefinition
{
	GENERATED_BODY()

public:
	UShootEquipmentDefinition_Weapon(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
