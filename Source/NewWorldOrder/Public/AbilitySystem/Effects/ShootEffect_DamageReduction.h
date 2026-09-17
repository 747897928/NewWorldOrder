// 通用减伤加成（加法到 DamageReductionBonus），可叠加临时减伤
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_DamageReduction.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_DamageReduction : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_DamageReduction();
};
