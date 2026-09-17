// 手雷冷却效果：仅负责阻止重复激活并提供 HUD 冷却时长，不依赖 Lyra 的 GE 资产。
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_GrenadeCooldown.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_GrenadeCooldown : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_GrenadeCooldown(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
