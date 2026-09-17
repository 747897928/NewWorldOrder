// 通用治疗输出加成（默认乘法 1.2），可供技能/被动使用
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_HealingDone.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_HealingDone : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_HealingDone();
};
