// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "InteractionStatics.generated.h"

template <typename InterfaceType> class TScriptInterface;

class AActor;
class IInteractableTarget;
class UObject;
struct FFrame;
struct FHitResult;
struct FOverlapResult;

/** 
 * 交互系统的静态函数库
 * 提供与交互目标相关的工具函数
 */
UCLASS()
class UInteractionStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UInteractionStatics();

public:
	// 从交互目标接口获取实际的Actor
	UFUNCTION(BlueprintCallable)
	static AActor* GetActorFromInteractableTarget(TScriptInterface<IInteractableTarget> InteractableTarget);

	// 从Actor获取所有可交互目标（包括Actor本身和其组件）
	UFUNCTION(BlueprintCallable)
	static void GetInteractableTargetsFromActor(AActor* Actor, TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets);

	// 从重叠检测结果中提取可交互目标
	static void AppendInteractableTargetsFromOverlapResults(const TArray<FOverlapResult>& OverlapResults, TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets);
    
	// 从命中结果中提取可交互目标
	static void AppendInteractableTargetsFromHitResult(const FHitResult& HitResult, TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets);
};
