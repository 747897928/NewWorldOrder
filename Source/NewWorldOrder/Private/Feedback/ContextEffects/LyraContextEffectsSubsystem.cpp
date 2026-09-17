// Copyright Epic Games, Inc. All Rights Reserved.

#include "Feedback/ContextEffects/LyraContextEffectsSubsystem.h"

#include "Feedback/ContextEffects/LyraContextEffectsLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraContextEffectsSubsystem)

void ULyraContextEffectsSubsystem::SpawnContextEffects(
	const AActor* SpawningActor,
	USceneComponent* AttachToComponent,
	FName AttachPoint,
	FVector LocationOffset,
	FRotator RotationOffset,
	FGameplayTag Effect,
	FGameplayTagContainer Contexts,
	TArray<UAudioComponent*>& AudioOut,
	TArray<UNiagaraComponent*>& NiagaraOut,
	FVector VFXScale,
	float AudioVolume,
	float AudioPitch)
{
	if (!SpawningActor || !AttachToComponent)
	{
		return;
	}

	const TObjectPtr<ULyraContextEffectsSet>* EffectsSetPtr = ActiveActorEffectsMap.Find(SpawningActor);
	if (!EffectsSetPtr || !*EffectsSetPtr)
	{
		return;
	}

	TArray<USoundBase*> Sounds;
	TArray<UNiagaraSystem*> NiagaraSystems;
	for (ULyraContextEffectsLibrary* EffectLibrary : (*EffectsSetPtr)->LyraContextEffectsLibraries)
	{
		if (!EffectLibrary)
		{
			continue;
		}

		if (EffectLibrary->GetContextEffectsLibraryLoadState() == EContextEffectsLibraryLoadState::Unloaded)
		{
			EffectLibrary->LoadEffects();
		}

		if (EffectLibrary->GetContextEffectsLibraryLoadState() == EContextEffectsLibraryLoadState::Loaded)
		{
			EffectLibrary->GetEffects(Effect, Contexts, Sounds, NiagaraSystems);
		}
	}

	for (USoundBase* Sound : Sounds)
	{
		if (UAudioComponent* AudioComponent = UGameplayStatics::SpawnSoundAttached(
			Sound,
			AttachToComponent,
			AttachPoint,
			LocationOffset,
			RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			false,
			AudioVolume,
			AudioPitch,
			0.0f,
			nullptr,
			nullptr,
			true))
		{
			AudioOut.Add(AudioComponent);
		}
	}

	for (UNiagaraSystem* NiagaraSystem : NiagaraSystems)
	{
		if (UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			AttachToComponent,
			AttachPoint,
			LocationOffset,
			RotationOffset,
			VFXScale,
			EAttachLocation::KeepRelativeOffset,
			true,
			ENCPoolMethod::None,
			true,
			true))
		{
			NiagaraOut.Add(NiagaraComponent);
		}
	}
}

bool ULyraContextEffectsSubsystem::GetContextFromSurfaceType(
	TEnumAsByte<EPhysicalSurface> PhysicalSurface,
	FGameplayTag& Context) const
{
	const ULyraContextEffectsSettings* Settings = GetDefault<ULyraContextEffectsSettings>();
	if (Settings)
	{
		if (const FGameplayTag* FoundContext = Settings->SurfaceTypeToContextMap.Find(PhysicalSurface))
		{
			Context = *FoundContext;
		}
	}

	return Context.IsValid();
}

void ULyraContextEffectsSubsystem::LoadAndAddContextEffectsLibraries(
	AActor* OwningActor,
	TSet<TSoftObjectPtr<ULyraContextEffectsLibrary>> ContextEffectsLibraries)
{
	if (!OwningActor || ContextEffectsLibraries.IsEmpty())
	{
		return;
	}

	ULyraContextEffectsSet* EffectsSet = NewObject<ULyraContextEffectsSet>(this);
	for (const TSoftObjectPtr<ULyraContextEffectsLibrary>& SoftLibrary : ContextEffectsLibraries)
	{
		if (ULyraContextEffectsLibrary* Library = SoftLibrary.LoadSynchronous())
		{
			Library->LoadEffects();
			EffectsSet->LyraContextEffectsLibraries.Add(Library);
		}
	}

	ActiveActorEffectsMap.Emplace(OwningActor, EffectsSet);
}

void ULyraContextEffectsSubsystem::UnloadAndRemoveContextEffectsLibraries(AActor* OwningActor)
{
	if (OwningActor)
	{
		ActiveActorEffectsMap.Remove(OwningActor);
	}
}
