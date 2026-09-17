// 通用护盾容量加成（加法），可供钢铁壁垒/装甲强化等使用
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_ShieldBonus.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_ShieldBonus : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_ShieldBonus();
};
