// 通用治疗受益加成（默认乘法 1.2），可供技能/被动使用
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_HealingReceived.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_HealingReceived : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_HealingReceived();
};
