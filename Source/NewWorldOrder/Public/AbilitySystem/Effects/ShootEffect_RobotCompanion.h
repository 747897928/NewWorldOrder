// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "GameplayEffect.h"

#include "ShootEffect_RobotCompanion.generated.h"

/**
 * 机器人自有 ASC 的出生/升级生命初始化。
 * 数值由 Character 蓝图可调参数计算，再通过两个 FName SetByCaller 通道写入；不污染玩家属性集。
 */
UCLASS()
class NEWWORLDORDER_API UShootEffect_RobotCompanionVitals final : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_RobotCompanionVitals();
};
