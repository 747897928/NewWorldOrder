// 通用减速效果：授予 Status.Slowed，并降低移速倍率
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_Slow.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_Slow : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_Slow(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
