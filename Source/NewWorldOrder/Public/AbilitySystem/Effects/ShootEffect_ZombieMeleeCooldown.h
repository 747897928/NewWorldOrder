// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "AbilitySystem/Effects/ShootEffect_MatchSkillCooldowns.h"

#include "ShootEffect_ZombieMeleeCooldown.generated.h"

/**
 * Zombie 近战冷却 GE。默认时长只是构造期兜底，Zombie GA::ApplyCooldown 会按 archetype
 * 的 Attack Interval 覆盖 Spec 时长；拥有标签由共同冷却 GE 基类配置。
 */
UCLASS()
class NEWWORLDORDER_API UShootEffect_ZombieMeleeCooldown final : public UShootEffect_MatchSkillCooldownBase
{
	GENERATED_BODY()

public:
	UShootEffect_ZombieMeleeCooldown(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
