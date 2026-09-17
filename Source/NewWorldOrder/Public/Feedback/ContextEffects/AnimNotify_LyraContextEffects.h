// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "Chaos/ChaosEngineInterface.h"
#include "Engine/EngineTypes.h"
#include "GameplayTagContainer.h"

#include "AnimNotify_LyraContextEffects.generated.h"

#define UE_API NEWWORLDORDER_API

USTRUCT(BlueprintType)
struct FLyraContextEffectAnimNotifyVFXSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FX)
	FVector Scale = FVector(1.0f, 1.0f, 1.0f);
};

USTRUCT(BlueprintType)
struct FLyraContextEffectAnimNotifyAudioSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	float VolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Sound)
	float PitchMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct FLyraContextEffectAnimNotifyTraceSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	TEnumAsByte<ECollisionChannel> TraceChannel = ECollisionChannel::ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	FVector EndTraceLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Trace)
	bool bIgnoreActor = true;
};

USTRUCT(BlueprintType)
struct FLyraContextEffectAnimNotifyPreviewSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview)
	bool bPreviewPhysicalSurfaceAsContext = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview, meta = (EditCondition = "bPreviewPhysicalSurfaceAsContext"))
	TEnumAsByte<EPhysicalSurface> PreviewPhysicalSurface = EPhysicalSurface::SurfaceType_Default;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview, meta = (AllowedClasses = "/Script/NewWorldOrder.LyraContextEffectsLibrary"))
	FSoftObjectPath PreviewContextEffectsLibrary;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Preview)
	FGameplayTagContainer PreviewContexts;
};

/**
 * Lyra 官方 Context Effects AnimNotify。
 *
 * 调用链：动画通知 -> Owner/Component 上的 ILyraContextEffectsInterface
 * -> ULyraContextEffectComponent -> WorldSubsystem -> 数据资产中的声音和 Niagara。
 * 保留 Lyra 反射类型名是为了让已迁入的 FootstepEffectTagModifier 原始节点和 pin 数据可恢复，
 * 模块路径差异由 DefaultEngine.ini 的 CoreRedirect 负责。
 */
UCLASS(MinimalAPI, const, hidecategories = Object, CollapseCategories, Config = Game, meta = (DisplayName = "Play Context Effects"))
class UAnimNotify_LyraContextEffects : public UAnimNotify
{
	GENERATED_BODY()

public:
	UE_API UAnimNotify_LyraContextEffects();

	UE_API virtual void PostLoad() override;
#if WITH_EDITOR
	UE_API virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	UE_API virtual FString GetNotifyName_Implementation() const override;
	UE_API virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
#if WITH_EDITOR
	UE_API virtual void ValidateAssociatedAssets() override;

	UFUNCTION(BlueprintCallable)
	UE_API void SetParameters(
		FGameplayTag EffectIn,
		FVector LocationOffsetIn,
		FRotator RotationOffsetIn,
		FLyraContextEffectAnimNotifyVFXSettings VFXPropertiesIn,
		FLyraContextEffectAnimNotifyAudioSettings AudioPropertiesIn,
		bool bAttachedIn,
		FName SocketNameIn,
		bool bPerformTraceIn,
		FLyraContextEffectAnimNotifyTraceSettings TracePropertiesIn);
#endif

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (DisplayName = "Effect", ExposeOnSpawn = true))
	FGameplayTag Effect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (ExposeOnSpawn = true))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (ExposeOnSpawn = true))
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (ExposeOnSpawn = true))
	FLyraContextEffectAnimNotifyVFXSettings VFXProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (ExposeOnSpawn = true))
	FLyraContextEffectAnimNotifyAudioSettings AudioProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttachmentProperties", meta = (ExposeOnSpawn = true))
	uint32 bAttached : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AttachmentProperties", meta = (ExposeOnSpawn = true, EditCondition = "bAttached"))
	FName SocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (ExposeOnSpawn = true))
	uint32 bPerformTrace : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify", meta = (ExposeOnSpawn = true, EditCondition = "bPerformTrace"))
	FLyraContextEffectAnimNotifyTraceSettings TraceProperties;

#if WITH_EDITORONLY_DATA
	UPROPERTY(Config, EditAnywhere, Category = "PreviewProperties")
	uint32 bPreviewInEditor : 1;

	UPROPERTY(EditAnywhere, Category = "PreviewProperties", meta = (EditCondition = "bPreviewInEditor"))
	FLyraContextEffectAnimNotifyPreviewSettings PreviewProperties;
#endif
};

#undef UE_API
