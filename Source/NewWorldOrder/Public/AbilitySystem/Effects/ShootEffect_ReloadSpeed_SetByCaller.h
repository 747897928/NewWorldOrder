// 通用换弹 Buff（SetByCaller）：通过 SetByCaller.ReloadSpeedMultiplier 配置倍率
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_ReloadSpeed_SetByCaller.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_ReloadSpeed_SetByCaller : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_ReloadSpeed_SetByCaller();
};
