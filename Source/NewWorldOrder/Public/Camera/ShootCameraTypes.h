#pragma once

#include "CoreMinimal.h"
#include "ShootCameraTypes.generated.h"

/**
 * Experience 选择的基础玩家视角。
 * ADS 是栈上的临时模式，不会改写这个基础视角。
 */
UENUM(BlueprintType)
enum class EShootCameraPerspective : uint8
{
	ThirdPerson,
	FirstPerson
};
