// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/HitResult.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "LyraContextEffectsInterface.generated.h"

#define UE_API NEWWORLDORDER_API

class UAnimSequenceBase;
class USceneComponent;

UENUM()
enum EEffectsContextMatchType : int
{
	ExactMatch,
	BestMatch
};

/**
 * 动画通知和具体效果实现之间的 Lyra 官方接口。
 * Character 可以通过 ActorComponent 实现它，AnimNotify 不需要 Cast 到项目角色类。
 */
UINTERFACE(MinimalAPI, Blueprintable)
class ULyraContextEffectsInterface : public UInterface
{
	GENERATED_BODY()
};

class ILyraContextEffectsInterface : public IInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	UE_API void AnimMotionEffect(
		FName Bone,
		FGameplayTag MotionEffect,
		USceneComponent* StaticMeshComponent,
		FVector LocationOffset,
		FRotator RotationOffset,
		const UAnimSequenceBase* AnimationSequence,
		bool bHitSuccess,
		FHitResult HitResult,
		FGameplayTagContainer Contexts,
		FVector VFXScale = FVector(1),
		float AudioVolume = 1,
		float AudioPitch = 1);
};

#undef UE_API
