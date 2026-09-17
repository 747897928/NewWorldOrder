// Copyright Epic Games, Inc. All Rights Reserved.

#include "Feedback/ContextEffects/LyraContextEffectComponent.h"

#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Feedback/ContextEffects/LyraContextEffectsLibrary.h"
#include "Feedback/ContextEffects/LyraContextEffectsSubsystem.h"
#include "NiagaraComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraContextEffectComponent)

ULyraContextEffectComponent::ULyraContextEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
}

void ULyraContextEffectComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentContexts.AppendTags(DefaultEffectContexts);
	CurrentContextEffectsLibraries = DefaultContextEffectsLibraries;

	if (const UWorld* World = GetWorld())
	{
		if (ULyraContextEffectsSubsystem* Subsystem = World->GetSubsystem<ULyraContextEffectsSubsystem>())
		{
			Subsystem->LoadAndAddContextEffectsLibraries(GetOwner(), CurrentContextEffectsLibraries);
		}
	}
}

void ULyraContextEffectComponent::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	if (const UWorld* World = GetWorld())
	{
		if (ULyraContextEffectsSubsystem* Subsystem = World->GetSubsystem<ULyraContextEffectsSubsystem>())
		{
			Subsystem->UnloadAndRemoveContextEffectsLibraries(GetOwner());
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ULyraContextEffectComponent::AnimMotionEffect_Implementation(
	FName Bone,
	FGameplayTag MotionEffect,
	USceneComponent* StaticMeshComponent,
	FVector LocationOffset,
	FRotator RotationOffset,
	const UAnimSequenceBase* AnimationSequence,
	bool bHitSuccess,
	FHitResult HitResult,
	FGameplayTagContainer Contexts,
	FVector VFXScale,
	float AudioVolume,
	float AudioPitch)
{
	FGameplayTagContainer TotalContexts = Contexts;
	TotalContexts.AppendTags(CurrentContexts);

	if (bConvertPhysicalSurfaceToContext && bHitSuccess && HitResult.PhysMaterial.IsValid())
	{
		const ULyraContextEffectsSettings* Settings = GetDefault<ULyraContextEffectsSettings>();
		if (Settings)
		{
			const EPhysicalSurface SurfaceType = HitResult.PhysMaterial->SurfaceType;
			if (const FGameplayTag* SurfaceContext = Settings->SurfaceTypeToContextMap.Find(SurfaceType))
			{
				TotalContexts.AddTag(*SurfaceContext);
			}
		}
	}

	ActiveAudioComponents.RemoveAll([](const TObjectPtr<UAudioComponent>& Component)
	{
		return !IsValid(Component);
	});
	ActiveNiagaraComponents.RemoveAll([](const TObjectPtr<UNiagaraComponent>& Component)
	{
		return !IsValid(Component);
	});

	if (const UWorld* World = GetWorld())
	{
		if (ULyraContextEffectsSubsystem* Subsystem = World->GetSubsystem<ULyraContextEffectsSubsystem>())
		{
			TArray<UAudioComponent*> SpawnedAudio;
			TArray<UNiagaraComponent*> SpawnedNiagara;
			Subsystem->SpawnContextEffects(
				GetOwner(),
				StaticMeshComponent,
				Bone,
				LocationOffset,
				RotationOffset,
				MotionEffect,
				TotalContexts,
				SpawnedAudio,
				SpawnedNiagara,
				VFXScale,
				AudioVolume,
				AudioPitch);
			ActiveAudioComponents.Append(SpawnedAudio);
			ActiveNiagaraComponents.Append(SpawnedNiagara);
		}
	}
}

void ULyraContextEffectComponent::UpdateEffectContexts(FGameplayTagContainer NewEffectContexts)
{
	CurrentContexts = MoveTemp(NewEffectContexts);
}

void ULyraContextEffectComponent::UpdateLibraries(
	TSet<TSoftObjectPtr<ULyraContextEffectsLibrary>> NewContextEffectsLibraries)
{
	CurrentContextEffectsLibraries = MoveTemp(NewContextEffectsLibraries);

	if (const UWorld* World = GetWorld())
	{
		if (ULyraContextEffectsSubsystem* Subsystem = World->GetSubsystem<ULyraContextEffectsSubsystem>())
		{
			Subsystem->UnloadAndRemoveContextEffectsLibraries(GetOwner());
			Subsystem->LoadAndAddContextEffectsLibraries(GetOwner(), CurrentContextEffectsLibraries);
		}
	}
}
