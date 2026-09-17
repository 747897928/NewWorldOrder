// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "GameplayEffect.h"

#include "ShootEffect_TacticalOverloadState.generated.h"

/**
 * 战术超载的权威持续状态。GA 会按技能等级覆盖 Duration；本 GE 只负责让 Status.Overload
 * 与 ActiveGameplayEffect 同生共灭，供弹药 Cost、HUD 和后续 Buff Modifier 使用。
 */
UCLASS(BlueprintType)
class NEWWORLDORDER_API UShootEffect_TacticalOverloadState final : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_TacticalOverloadState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
