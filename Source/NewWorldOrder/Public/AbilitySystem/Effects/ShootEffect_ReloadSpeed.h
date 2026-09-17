// 通用换弹速度 Buff（默认乘法 1.3），可被技能使用或子类覆盖
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_ReloadSpeed.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_ReloadSpeed : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_ReloadSpeed();
};
