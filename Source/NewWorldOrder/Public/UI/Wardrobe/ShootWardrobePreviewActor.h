// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/CharacterGender.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "UI/Wardrobe/ShootWardrobeTypes.h"
#include "ShootWardrobePreviewActor.generated.h"

class UCustomizableSkeletalComponent;
class UMutableAppearanceComponent;
class USkeletalMeshComponent;
class USkyLightComponent;
class USpotLightComponent;
class UPostProcessComponent;
class UStaticMeshComponent;
class UAnimSequenceBase;
class UAnimInstance;

/**
 * 衣柜 UI 专用预览 Actor。
 *
 * 它只在 UMG Viewport 的独立预览世界中生成，不参与真实关卡、输入、库存或存档。
 * W_Cloth 打开后把 PlayerState 当前性别与外观标签同步进来，试穿/装备时刷新这个 Actor，
 * 玩家就能在界面的影棚预览区域直接看换装效果。
 */
UCLASS()
class NEWWORLDORDER_API AShootWardrobePreviewActor : public AActor
{
	GENERATED_BODY()

public:
	AShootWardrobePreviewActor();

	void ApplyAppearance(ECharacterGender InGender, const FGameplayTagContainer& InAppearanceTags);
	void SetPreviewYaw(float InYawDegrees);
	void QueuePresentationAnimation(const TSoftObjectPtr<UAnimSequenceBase>& Animation);
	void ClearQueuedPresentationAnimation();
	bool GetCameraView(EShootWardrobePreviewCameraMode CameraMode, FVector& OutViewLocation, FRotator& OutViewRotation, FVector& OutFocusTarget) const;

protected:
	virtual void BeginPlay() override;

private:
	void InitializePreviewIfNeeded(ECharacterGender InGender, const FGameplayTagContainer& InAppearanceTags);
	void HandleMutableSkeletalMeshUpdated();
	void PlayQueuedPresentationIfReady();
	USceneComponent* ResolveCameraAnchor(EShootWardrobePreviewCameraMode CameraMode) const;
	FVector ResolveCameraFocusTarget(EShootWardrobePreviewCameraMode CameraMode) const;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview")
	TObjectPtr<USceneComponent> PreviewMeshRoot;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Studio")
	TObjectPtr<UStaticMeshComponent> StudioFloor;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Studio")
	TObjectPtr<UStaticMeshComponent> StudioBackdrop;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Camera")
	TObjectPtr<USceneComponent> FullBodyCameraAnchor;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Camera")
	TObjectPtr<USceneComponent> UpperBodyCameraAnchor;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Camera")
	TObjectPtr<USceneComponent> HeadCameraAnchor;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Camera")
	TObjectPtr<USceneComponent> FootwearCameraAnchor;

	/**
	 * CameraAnchor 负责默认机位，FocusTarget 负责默认画面中心。
	 * Widget 使用角色位置和 FocusTarget 高度生成缩放轴心，避免横向构图偏移在 dolly 时产生视差。
	 */
	UPROPERTY(EditDefaultsOnly, Category="WardrobePreview|Camera")
	FVector FullBodyCameraFocusTarget;

	UPROPERTY(EditDefaultsOnly, Category="WardrobePreview|Camera")
	FVector UpperBodyCameraFocusTarget;

	UPROPERTY(EditDefaultsOnly, Category="WardrobePreview|Camera")
	FVector HeadCameraFocusTarget;

	UPROPERTY(EditDefaultsOnly, Category="WardrobePreview|Camera")
	FVector FootwearCameraFocusTarget;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview")
	TObjectPtr<USkeletalMeshComponent> PreviewHeadMesh;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview")
	TObjectPtr<USkeletalMeshComponent> PreviewBodyMesh;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview")
	TObjectPtr<UCustomizableSkeletalComponent> PreviewHeadCSkeletalComponent;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview")
	TObjectPtr<UCustomizableSkeletalComponent> PreviewBodyCSkeletalComponent;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview")
	TObjectPtr<UMutableAppearanceComponent> PreviewAppearanceComponent;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Lighting")
	TObjectPtr<USpotLightComponent> KeyLight;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Lighting")
	TObjectPtr<USpotLightComponent> FillLight;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Lighting")
	TObjectPtr<USpotLightComponent> RimLight;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Lighting")
	TObjectPtr<USpotLightComponent> BackLight;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|Lighting")
	TObjectPtr<USkyLightComponent> StudioSkyLight;

	UPROPERTY(VisibleAnywhere, Category="WardrobePreview|PostProcess")
	TObjectPtr<UPostProcessComponent> StudioPostProcess;

	/**
	 * 角色网格正面对准衣柜相机所需的基础偏移。
	 * W_Cloth 运行时只叠加玩家拖拽旋转，不再把朝向写死；如果某个 Mutable 角色资源正面轴不同，
	 * 请在 BP_ShootWardrobePreviewActor 里调这个值，而不是改 Widget 或相机。
	 */
	UPROPERTY(EditDefaultsOnly, Category="WardrobePreview|Camera")
	float PreviewFacingYawOffset = 0.f;

	/**
	 * 只替换衣柜预览 Actor 的 AnimInstance。请在 BP_ShootWardrobePreviewActor 中配置专用 AnimBP；
	 * Idle、随机条目和男女动画替换全部由 AnimGraph / Asset Override Editor 维护，C++ 不参与选择。
	 * 留空时沿用 MutableAppearanceComponent 为该性别选择的游戏角色 AnimBP。
	 */
	UPROPERTY(EditDefaultsOnly, Category="WardrobePreview|Animation")
	TSubclassOf<UAnimInstance> FemalePreviewAnimClass;

	UPROPERTY(EditDefaultsOnly, Category="WardrobePreview|Animation")
	TSubclassOf<UAnimInstance> MalePreviewAnimClass;

	/** 试穿表演覆盖 DefaultSlot 时使用的过渡时间；在 BP_ShootWardrobePreviewActor 中按画面效果调整。 */
	UPROPERTY(EditDefaultsOnly, Category="WardrobePreview|Animation", meta=(ClampMin="0.0"))
	float PresentationBlendInTime = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category="WardrobePreview|Animation", meta=(ClampMin="0.0"))
	float PresentationBlendOutTime = 0.35f;

	TSoftObjectPtr<UAnimSequenceBase> QueuedPresentationAnimation;

	bool bPreviewInitialized = false;
};
