// 通用移速 Buff（SetByCaller）：通过 SetByCaller.MoveSpeedMultiplier 配置倍率
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_MoveSpeed_SetByCaller.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_MoveSpeed_SetByCaller : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_MoveSpeed_SetByCaller();
};
