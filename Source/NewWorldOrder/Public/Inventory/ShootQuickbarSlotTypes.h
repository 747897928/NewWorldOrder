// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ShootQuickbarSlotTypes.generated.h"

/**
 * 快捷栏槽位类型
 */
UENUM(BlueprintType)
enum class EShootQuickbarSlotType : uint8
{
	Any,
	PrimaryWeapon,
	SecondaryWeapon,
	MeleeWeapon,
};
