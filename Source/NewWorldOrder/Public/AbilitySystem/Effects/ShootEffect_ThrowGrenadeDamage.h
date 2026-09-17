// Copyright ZhaoYiJie

#pragma once

#include "AbilitySystem/Effects/ShootEffect_DamageBase.h"
#include "ShootEffect_ThrowGrenadeDamage.generated.h"

/** 手持手雷专用伤害 GE；与榴弹发射器的武器/弹药/表现闭环保持独立。 */
UCLASS()
class NEWWORLDORDER_API UShootEffect_ThrowGrenadeDamage : public UShootEffect_DamageBase
{
	GENERATED_BODY()

public:
	UShootEffect_ThrowGrenadeDamage();
};
