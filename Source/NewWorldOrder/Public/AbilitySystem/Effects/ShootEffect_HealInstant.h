// 通用即时治疗 GE：通过 SetByCaller.Heal 以加法修改 Health
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_HealInstant.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_HealInstant : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_HealInstant();
};

