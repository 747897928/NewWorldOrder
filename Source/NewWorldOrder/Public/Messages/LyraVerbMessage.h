// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "LyraVerbMessage.generated.h"

// 对齐 Lyra 的通用消息：Instigator Verb Target（可附带上下文标签和数值）。
// 本项目只迁移消息数据结构，不迁移依赖 LyraGameState/PlayerState 的整套复制组件；
// Respawn 通过项目自己的 PlayerController Client RPC 进入正确的本地 GameplayMessageSubsystem。
USTRUCT(BlueprintType)
struct FLyraVerbMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTag Verb;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<UObject> Instigator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	TObjectPtr<UObject> Target = nullptr;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer InstigatorTags;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer TargetTags;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	FGameplayTagContainer ContextTags;

	UPROPERTY(BlueprintReadWrite, Category=Gameplay)
	double Magnitude = 1.0;

	// 保留 Lyra 的调试接口，便于 GameplayMessage 日志直接输出完整消息。
	NEWWORLDORDER_API FString ToString() const;
};
