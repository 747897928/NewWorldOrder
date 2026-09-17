// 通用眩晕效果：授予 Status.Stunned，并阻止开火
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_Stun.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_Stun : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_Stun(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
