// Copyright Epic Games, Inc. All Rights Reserved.

#include "Feedback/ContextEffects/LyraContextEffectsLibrary.h"

#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraContextEffectsLibrary)

void ULyraContextEffectsLibrary::GetEffects(
	const FGameplayTag Effect,
	const FGameplayTagContainer Context,
	TArray<USoundBase*>& Sounds,
	TArray<UNiagaraSystem*>& NiagaraSystems)
{
	if (!Effect.IsValid() || EffectsLoadState != EContextEffectsLibraryLoadState::Loaded)
	{
		return;
	}

	for (const ULyraActiveContextEffects* ActiveContextEffect : ActiveContextEffects)
	{
		if (ActiveContextEffect
			&& Effect.MatchesTagExact(ActiveContextEffect->EffectTag)
			&& Context.HasAllExact(ActiveContextEffect->Context)
			&& ActiveContextEffect->Context.IsEmpty() == Context.IsEmpty())
		{
			Sounds.Append(ActiveContextEffect->Sounds);
			NiagaraSystems.Append(ActiveContextEffect->NiagaraSystems);
		}
	}
}

void ULyraContextEffectsLibrary::LoadEffects()
{
	if (EffectsLoadState == EContextEffectsLibraryLoadState::Loading
		|| EffectsLoadState == EContextEffectsLibraryLoadState::Loaded)
	{
		return;
	}

	EffectsLoadState = EContextEffectsLibraryLoadState::Loading;
	ActiveContextEffects.Empty();
	LoadEffectsInternal();
}

EContextEffectsLibraryLoadState ULyraContextEffectsLibrary::GetContextEffectsLibraryLoadState() const
{
	return EffectsLoadState;
}

void ULyraContextEffectsLibrary::LoadEffectsInternal()
{
	TArray<ULyraActiveContextEffects*> LoadedContextEffects;
	for (const FLyraContextEffects& ContextEffect : ContextEffects)
	{
		if (!ContextEffect.EffectTag.IsValid())
		{
			continue;
		}

		ULyraActiveContextEffects* ActiveEffect = NewObject<ULyraActiveContextEffects>(this);
		ActiveEffect->EffectTag = ContextEffect.EffectTag;
		ActiveEffect->Context = ContextEffect.Context;

		for (const FSoftObjectPath& EffectPath : ContextEffect.Effects)
		{
			if (UObject* LoadedObject = EffectPath.TryLoad())
			{
				if (USoundBase* Sound = Cast<USoundBase>(LoadedObject))
				{
					ActiveEffect->Sounds.Add(Sound);
				}
				else if (UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(LoadedObject))
				{
					ActiveEffect->NiagaraSystems.Add(NiagaraSystem);
				}
			}
		}

		LoadedContextEffects.Add(ActiveEffect);
	}

	LyraContextEffectLibraryLoadingComplete(LoadedContextEffects);
}

void ULyraContextEffectsLibrary::LyraContextEffectLibraryLoadingComplete(
	TArray<ULyraActiveContextEffects*> LoadedContextEffects)
{
	EffectsLoadState = EContextEffectsLibraryLoadState::Loaded;
	ActiveContextEffects.Append(LoadedContextEffects);
}
