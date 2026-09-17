// Copyright Epic Games, Inc. All Rights Reserved.

#include "Feedback/ContextEffects/AnimNotify_LyraContextEffects.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Feedback/ContextEffects/LyraContextEffectsInterface.h"
#include "Feedback/ContextEffects/LyraContextEffectsLibrary.h"
#include "Feedback/ContextEffects/LyraContextEffectsSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_LyraContextEffects)

UAnimNotify_LyraContextEffects::UAnimNotify_LyraContextEffects()
{
}

void UAnimNotify_LyraContextEffects::PostLoad()
{
	Super::PostLoad();
}

#if WITH_EDITOR
void UAnimNotify_LyraContextEffects::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

FString UAnimNotify_LyraContextEffects::GetNotifyName_Implementation() const
{
	return Effect.IsValid() ? Effect.ToString() : Super::GetNotifyName_Implementation();
}

void UAnimNotify_LyraContextEffects::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwningActor = MeshComp->GetOwner();
	if (!OwningActor)
	{
		return;
	}

	bool bHitSuccess = false;
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	if (TraceProperties.bIgnoreActor)
	{
		QueryParams.AddIgnoredActor(OwningActor);
	}
	QueryParams.bReturnPhysicalMaterial = true;

	if (bPerformTrace)
	{
		// Trace 起点必须跟随具体发出通知的 MeshComp，而不是假设 CharacterMesh0；
		// 这样隐藏 Manny 源网格和可见 CC 网格共存时，Linked Layer 的通知仍使用正确 Owner。
		const FVector TraceStart =
			bAttached ? MeshComp->GetSocketLocation(SocketName) : MeshComp->GetComponentLocation();
		if (UWorld* World = OwningActor->GetWorld())
		{
			bHitSuccess = World->LineTraceSingleByChannel(
				HitResult,
				TraceStart,
				TraceStart + TraceProperties.EndTraceLocationOffset,
				TraceProperties.TraceChannel,
				QueryParams,
				FCollisionResponseParams::DefaultResponseParam);
		}
	}

	FGameplayTagContainer Contexts;
	TArray<UObject*> ImplementingObjects;
	if (OwningActor->Implements<ULyraContextEffectsInterface>())
	{
		ImplementingObjects.Add(OwningActor);
	}

	for (UActorComponent* Component : OwningActor->GetComponents())
	{
		if (Component && Component->Implements<ULyraContextEffectsInterface>())
		{
			ImplementingObjects.Add(Component);
		}
	}

	for (UObject* Object : ImplementingObjects)
	{
		ILyraContextEffectsInterface::Execute_AnimMotionEffect(
			Object,
			bAttached ? SocketName : NAME_None,
			Effect,
			MeshComp,
			LocationOffset,
			RotationOffset,
			Animation,
			bHitSuccess,
			HitResult,
			Contexts,
			VFXProperties.Scale,
			AudioProperties.VolumeMultiplier,
			AudioProperties.PitchMultiplier);
	}

#if WITH_EDITORONLY_DATA
	if (!bPreviewInEditor)
	{
		return;
	}

	UWorld* World = OwningActor->GetWorld();
	if (!World || World->WorldType != EWorldType::EditorPreview)
	{
		return;
	}

	Contexts.AppendTags(PreviewProperties.PreviewContexts);
	if (PreviewProperties.bPreviewPhysicalSurfaceAsContext)
	{
		const ULyraContextEffectsSettings* Settings = GetDefault<ULyraContextEffectsSettings>();
		if (Settings)
		{
			const FGameplayTag* SurfaceContext =
				Settings->SurfaceTypeToContextMap.Find(PreviewProperties.PreviewPhysicalSurface);
			if (SurfaceContext)
			{
				Contexts.AddTag(*SurfaceContext);
			}
		}
	}

	UObject* LibraryObject = PreviewProperties.PreviewContextEffectsLibrary.TryLoad();
	ULyraContextEffectsLibrary* EffectLibrary = Cast<ULyraContextEffectsLibrary>(LibraryObject);
	if (!EffectLibrary)
	{
		return;
	}

	EffectLibrary->LoadEffects();
	if (EffectLibrary->GetContextEffectsLibraryLoadState() != EContextEffectsLibraryLoadState::Loaded)
	{
		return;
	}

	TArray<USoundBase*> Sounds;
	TArray<UNiagaraSystem*> NiagaraSystems;
	EffectLibrary->GetEffects(Effect, Contexts, Sounds, NiagaraSystems);

	for (USoundBase* Sound : Sounds)
	{
		UGameplayStatics::SpawnSoundAttached(
			Sound,
			MeshComp,
			bAttached ? SocketName : NAME_None,
			LocationOffset,
			RotationOffset,
			EAttachLocation::KeepRelativeOffset,
			false,
			AudioProperties.VolumeMultiplier,
			AudioProperties.PitchMultiplier,
			0.0f,
			nullptr,
			nullptr,
			true);
	}

	for (UNiagaraSystem* NiagaraSystem : NiagaraSystems)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			NiagaraSystem,
			MeshComp,
			bAttached ? SocketName : NAME_None,
			LocationOffset,
			RotationOffset,
			VFXProperties.Scale,
			EAttachLocation::KeepRelativeOffset,
			true,
			ENCPoolMethod::None,
			true,
			true);
	}
#endif
}

#if WITH_EDITOR
void UAnimNotify_LyraContextEffects::ValidateAssociatedAssets()
{
	Super::ValidateAssociatedAssets();
}

void UAnimNotify_LyraContextEffects::SetParameters(
	FGameplayTag EffectIn,
	FVector LocationOffsetIn,
	FRotator RotationOffsetIn,
	FLyraContextEffectAnimNotifyVFXSettings VFXPropertiesIn,
	FLyraContextEffectAnimNotifyAudioSettings AudioPropertiesIn,
	bool bAttachedIn,
	FName SocketNameIn,
	bool bPerformTraceIn,
	FLyraContextEffectAnimNotifyTraceSettings TracePropertiesIn)
{
	Effect = EffectIn;
	LocationOffset = LocationOffsetIn;
	RotationOffset = RotationOffsetIn;
	VFXProperties = VFXPropertiesIn;
	AudioProperties = AudioPropertiesIn;
	bAttached = bAttachedIn;
	SocketName = SocketNameIn;
	bPerformTrace = bPerformTraceIn;
	TraceProperties = TracePropertiesIn;
}
#endif
