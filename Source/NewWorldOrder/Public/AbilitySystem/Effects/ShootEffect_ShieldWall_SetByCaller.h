// 护盾墙 Buff：通过 SetByCaller 注入护盾与减伤倍率，支持运行时动态数值
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ShootEffect_ShieldWall_SetByCaller.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_ShieldWall_SetByCaller : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_ShieldWall_SetByCaller();
};
