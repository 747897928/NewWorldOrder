// 角色快照：通过 SetByCaller 注入属性 delta（Health/Shield/UltimateCharge）
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_SnapshotRestore.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_SnapshotRestore : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_SnapshotRestore();
};
