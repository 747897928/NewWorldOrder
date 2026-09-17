// Copyright ZhaoYiJie

#include "Equipment/ShootEquipmentDefinition.h"
#include "Equipment/ShootEquipmentInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootEquipmentDefinition)

UShootEquipmentDefinition::UShootEquipmentDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 默认实例类型设置为基础 EquipmentInstance
	// 子类应该重写此值
	InstanceType = UShootEquipmentInstance::StaticClass();
}
