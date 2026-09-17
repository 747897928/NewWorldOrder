// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"

#include "LyraContextEffectsSubsystem.generated.h"

#define UE_API NEWWORLDORDER_API

enum EPhysicalSurface : int;

class AActor;
class UAudioComponent;
class ULyraContextEffectsLibrary;
class UNiagaraComponent;
class USceneComponent;

UCLASS(MinimalAPI, config = Game, defaultconfig, meta = (DisplayName = "LyraContextEffects"))
class ULyraContextEffectsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(config, EditAnywhere)
	TMap<TEnumAsByte<EPhysicalSurface>, FGameplayTag> SurfaceTypeToContextMap;
};

UCLASS(MinimalAPI)
class ULyraContextEffectsSet : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TSet<TObjectPtr<ULyraContextEffectsLibrary>> LyraContextEffectsLibraries;
};

/**
 * World 级 ContextEffects 调度器。
 * 组件注册 Actor 对应的数据资产集合，AnimNotify 只传 Effect/Context/命中表面，
 * Subsystem 再生成声音和 Niagara，保持角色、动画与表现资源解耦。
 */
UCLASS(MinimalAPI)
class ULyraContextEffectsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "ContextEffects")
	UE_API void SpawnContextEffects(
		const AActor* SpawningActor,
		USceneComponent* AttachToComponent,
		FName AttachPoint,
		FVector LocationOffset,
		FRotator RotationOffset,
		FGameplayTag Effect,
		FGameplayTagContainer Contexts,
		TArray<UAudioComponent*>& AudioOut,
		TArray<UNiagaraComponent*>& NiagaraOut,
		FVector VFXScale = FVector(1),
		float AudioVolume = 1,
		float AudioPitch = 1);

	UFUNCTION(BlueprintCallable, Category = "ContextEffects")
	UE_API bool GetContextFromSurfaceType(TEnumAsByte<EPhysicalSurface> PhysicalSurface, FGameplayTag& Context) const;

	UFUNCTION(BlueprintCallable, Category = "ContextEffects")
	UE_API void LoadAndAddContextEffectsLibraries(
		AActor* OwningActor,
		TSet<TSoftObjectPtr<ULyraContextEffectsLibrary>> ContextEffectsLibraries);

	UFUNCTION(BlueprintCallable, Category = "ContextEffects")
	UE_API void UnloadAndRemoveContextEffectsLibraries(AActor* OwningActor);

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, TObjectPtr<ULyraContextEffectsSet>> ActiveActorEffectsMap;
};

#undef UE_API
