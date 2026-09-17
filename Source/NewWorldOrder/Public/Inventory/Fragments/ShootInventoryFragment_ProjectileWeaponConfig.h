// Projectile weapon config fragment: 投射物相关配置
#pragma once

#include "CoreMinimal.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Weapons/Projectiles/ShootProjectileBase.h"
#include "ShootInventoryFragment_ProjectileWeaponConfig.generated.h"

class AShootProjectileBase;

UCLASS(BlueprintType)
class UShootInventoryFragment_ProjectileWeaponConfig : public UShootInventoryItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Projectile")
	TSubclassOf<AShootProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Projectile")
	FProjectileWeaponConfig ProjectileConfig;
};
