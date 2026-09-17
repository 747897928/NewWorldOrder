// Copyright ZhaoYiJie

#include "Weapons/Projectiles/ShootProjectileGrenade.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"

void AShootProjectileGrenade::HandlePreExplode(const FHitResult& ImpactHit)
{
	if (MovementComponent)
	{
		MovementComponent->StopMovementImmediately();
	}
}

bool AShootProjectileGrenade::ShouldExplodeOnHit(AActor* OtherActor, const FHitResult& Hit) const
{
	// 榴弹命中角色或明确可受伤的目标时立即爆炸；撞墙、地面等静态几何只弹跳，
	// 最终由 FuseTime 引爆。旧实现只判断“不是自己”，导致撞墙也直接爆炸。
	return OtherActor
		&& OtherActor != GetInstigator()
		&& (OtherActor->IsA<APawn>() || OtherActor->CanBeDamaged());
}
