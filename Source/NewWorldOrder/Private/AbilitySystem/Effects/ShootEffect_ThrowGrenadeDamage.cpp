// Copyright ZhaoYiJie

#include "AbilitySystem/Effects/ShootEffect_ThrowGrenadeDamage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootEffect_ThrowGrenadeDamage)

UShootEffect_ThrowGrenadeDamage::UShootEffect_ThrowGrenadeDamage()
{
	// 这是手持手雷的 GE 默认值，不是武器 Item Fragment，也不进入榴弹发射器配置。
	BaseDamage = 100.0f;
}
