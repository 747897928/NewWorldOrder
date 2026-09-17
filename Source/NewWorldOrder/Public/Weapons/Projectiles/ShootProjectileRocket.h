// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Weapons/Projectiles/ShootProjectileBase.h"
#include "ShootProjectileRocket.generated.h"

/**
 * 火箭投射物：直线飞行，命中立即爆炸
 */
UCLASS()
class AShootProjectileRocket : public AShootProjectileBase
{
	GENERATED_BODY()

protected:
	virtual void ConfigureMovement() override;
};
