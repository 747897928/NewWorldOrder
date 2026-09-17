// 技能通用加速效果（默认乘法 1.0，可在类默认值或实例上调节），可被技能覆盖使用
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_MoveSpeed.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_MoveSpeed : public UGameplayEffect
{
	GENERATED_BODY()

public:
	// 默认基于 Attributes.Combat.MoveSpeedMultiplier 做乘法，默认系数 1.2
	UShootEffect_MoveSpeed();
};
