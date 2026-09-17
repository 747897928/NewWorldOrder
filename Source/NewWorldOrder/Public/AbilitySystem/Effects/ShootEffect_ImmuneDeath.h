// 临时免疫致死：给予 Status.ImmuneDeath 标签，阻止生命值降到 0 以下
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_ImmuneDeath.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_ImmuneDeath : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_ImmuneDeath(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
