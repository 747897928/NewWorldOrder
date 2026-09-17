// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Weapons/Projectiles/ShootProjectileBase.h"
#include "ShootProjectileGrenade.generated.h"

/**
 * 榴弹投射物：允许弹跳，引信到期或命中目标后爆炸
 */
UCLASS()
class AShootProjectileGrenade : public AShootProjectileBase
{
	GENERATED_BODY()

protected:
	virtual void HandlePreExplode(const FHitResult& ImpactHit) override;
	virtual bool ShouldExplodeOnHit(AActor* OtherActor, const FHitResult& Hit) const override;
};
