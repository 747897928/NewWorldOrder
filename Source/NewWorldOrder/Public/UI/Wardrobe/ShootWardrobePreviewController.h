// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/Wardrobe/ShootWardrobeTypes.h"
#include "ShootWardrobePreviewController.generated.h"

class APlayerController;
class AShootWardrobePreviewActor;
class UViewport;
class UAnimSequenceBase;

/**
 * W_Cloth 蓝图使用的预览控制器。
 *
 * 它不继承 Widget，也不查找任何命名控件。W_Cloth 在激活时显式传入 Viewport 和预览 Actor 类，
 * Event Graph 再把按钮、鼠标拖拽和滚轮输入转成 Rotate/Zoom 调用。相机基准点全部来自
 * BP_ShootWardrobePreviewActor，不在 C++ 中保留视觉兜底坐标。
 */
UCLASS(BlueprintType)
class NEWWORLDORDER_API UShootWardrobePreviewController : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	bool Initialize(UViewport* InViewport, APlayerController* InOwningPlayer,
	                TSubclassOf<AShootWardrobePreviewActor> InPreviewActorClass);

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void Shutdown();

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void RefreshAppearance();

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void SetCameraMode(EShootWardrobePreviewCameraMode InCameraMode);

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void Rotate(float DeltaYawDegrees);

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void Zoom(float DeltaDistance);

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void ResetView();

	/** 装备请求发出前排队；Mutable 异步换装完成后由 PreviewActor 真正播放。 */
	void QueuePresentationAnimation(const TSoftObjectPtr<UAnimSequenceBase>& Animation);
	void ClearQueuedPresentationAnimation();

	UFUNCTION(BlueprintCallable, Category="Wardrobe|Preview")
	void Tick(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category="Wardrobe|Preview")
	AShootWardrobePreviewActor* GetPreviewActor() const { return PreviewActor.Get(); }

private:
	void ApplyCamera(bool bSnap = false);

	UPROPERTY(Transient)
	TWeakObjectPtr<UViewport> Viewport;

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> OwningPlayer;

	UPROPERTY(Transient)
	TWeakObjectPtr<AShootWardrobePreviewActor> PreviewActor;

	float PreviewYaw = 0.f;
	float ZoomOffset = 0.f;
	bool bCameraInitialized = false;
	EShootWardrobePreviewCameraMode CameraMode = EShootWardrobePreviewCameraMode::FullBody;
	FVector CurrentViewLocation = FVector::ZeroVector;
	FVector TargetViewLocation = FVector::ZeroVector;
	FRotator CurrentViewRotation = FRotator::ZeroRotator;
	FRotator TargetViewRotation = FRotator::ZeroRotator;

	/**
	 * 一级页、二级分类和条目刷新都会触发 ViewModel 状态广播，但它们不一定改变角色外观。
	 * 缓存上次已提交给 Mutable 的快照，避免切换 UI 时重复重建预览网格和 AnimBP。
	 */
	FGameplayTagContainer LastAppliedAppearanceTags;
	ECharacterGender LastAppliedGender = ECharacterGender::UNKNOWN;
};
