// 通用易伤效果：授予 Status.Vulnerable，并降低减伤（默认 -30%）
#pragma once

#include "GameplayEffect.h"
#include "ShootEffect_Vulnerable.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_Vulnerable : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_Vulnerable(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
