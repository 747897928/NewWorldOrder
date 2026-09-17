// 通用射速 Buff（默认乘法 1.3），可被技能使用或子类覆盖
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_FireRate.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_FireRate : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_FireRate();
};
