// Copyright ZhaoYiJie

#include "Weapons/Projectiles/ShootProjectileRocket.h"

#include "GameFramework/ProjectileMovementComponent.h"

void AShootProjectileRocket::ConfigureMovement()
{
	if (MovementComponent)
	{
		// 火箭的速度和重力仍由 ProjectileConfig 数据驱动；只有“不能弹跳”是火箭类不可违背的规则。
		MovementComponent->bShouldBounce = false;
	}
}
