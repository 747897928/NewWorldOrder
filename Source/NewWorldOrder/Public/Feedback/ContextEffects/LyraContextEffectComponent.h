// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "Feedback/ContextEffects/LyraContextEffectsInterface.h"

#include "LyraContextEffectComponent.generated.h"

#define UE_API NEWWORLDORDER_API

class UAudioComponent;
class ULyraContextEffectsLibrary;
class UNiagaraComponent;

/**
 * Actor 侧 ContextEffects 入口。
 * 默认数据资产由 BP_ShootCharacter 配置；C++ 只负责生命周期注册和运行时上下文聚合。
 */
UCLASS(
	MinimalAPI,
	ClassGroup = (Custom),
	hidecategories = (Variable, Tags, ComponentTick, ComponentReplication, Activation, Cooking, AssetUserData, Collision),
	CollapseCategories,
	meta = (BlueprintSpawnableComponent))
class ULyraContextEffectComponent : public UActorComponent, public ILyraContextEffectsInterface
{
	GENERATED_BODY()

public:
	UE_API ULyraContextEffectComponent();

protected:
	UE_API virtual void BeginPlay() override;
	UE_API virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

public:
	UFUNCTION(BlueprintCallable)
	UE_API virtual void AnimMotionEffect_Implementation(
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
		float AudioPitch = 1) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Context Effects")
	bool bConvertPhysicalSurfaceToContext = true;

	UPROPERTY(EditAnywhere, Category = "Context Effects")
	FGameplayTagContainer DefaultEffectContexts;

	UPROPERTY(EditAnywhere, Category = "Context Effects")
	TSet<TSoftObjectPtr<ULyraContextEffectsLibrary>> DefaultContextEffectsLibraries;

	UFUNCTION(BlueprintCallable)
	UE_API void UpdateEffectContexts(FGameplayTagContainer NewEffectContexts);

	UFUNCTION(BlueprintCallable)
	UE_API void UpdateLibraries(TSet<TSoftObjectPtr<ULyraContextEffectsLibrary>> NewContextEffectsLibraries);

private:
	UPROPERTY(Transient)
	FGameplayTagContainer CurrentContexts;

	UPROPERTY(Transient)
	TSet<TSoftObjectPtr<ULyraContextEffectsLibrary>> CurrentContextEffectsLibraries;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> ActiveAudioComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraComponent>> ActiveNiagaraComponents;
};

#undef UE_API
