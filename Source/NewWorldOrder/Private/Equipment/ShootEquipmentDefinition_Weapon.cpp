// Copyright ZhaoYiJie

#include "Equipment/ShootEquipmentDefinition_Weapon.h"
#include "Weapons/ShootRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootEquipmentDefinition_Weapon)

UShootEquipmentDefinition_Weapon::UShootEquipmentDefinition_Weapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Phase2 默认采用 Lyra 风格的 UObject 武器实例
	if (!InstanceType)
	{
		InstanceType = UShootRangedWeaponInstance::StaticClass();
	}
}

