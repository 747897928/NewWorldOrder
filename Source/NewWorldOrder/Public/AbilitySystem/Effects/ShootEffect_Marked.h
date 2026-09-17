// 通用标记效果：授予 Status.Marked
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "ShootEffect_Marked.generated.h"

UCLASS()
class NEWWORLDORDER_API UShootEffect_Marked : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UShootEffect_Marked(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
