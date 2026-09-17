// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"

#include "LyraInteractionDurationMessage.generated.h"

// 声明交互持续时间消息的GameplayTag
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_INTERACTION_DURATION_MESSAGE);

/**
 * 交互持续时间消息结构体
 * 用于在长时间交互过程中传递信息
 */
USTRUCT(BlueprintType)
struct FLyraInteractionDurationMessage
{
	GENERATED_BODY()

public:
	// 交互的发起者
	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Instigator = nullptr;

	// 交互持续时间
	UPROPERTY(BlueprintReadWrite)
	float Duration = 0;
};
