// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Wardrobe/ShootWardrobePreviewController.h"

#include "Components/Viewport.h"
#include "GameFramework/PlayerController.h"
#include "Player/ShootPlayerState.h"
#include "ShootLogChannels.h"
#include "UI/Wardrobe/ShootWardrobePreviewActor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootWardrobePreviewController)

bool UShootWardrobePreviewController::Initialize(UViewport* InViewport,
	APlayerController* InOwningPlayer,
	TSubclassOf<AShootWardrobePreviewActor> InPreviewActorClass)
{
	Shutdown();
	Viewport = InViewport;
	OwningPlayer = InOwningPlayer;
	if (!Viewport.IsValid() || !InPreviewActorClass)
	{
		return false;
	}

	Viewport->SetEnableAdvancedFeatures(true);
	PreviewActor = Cast<AShootWardrobePreviewActor>(Viewport->Spawn(InPreviewActorClass));
	if (!PreviewActor.IsValid())
	{
		return false;
	}

	RefreshAppearance();
	ResetView();
	return true;
}

void UShootWardrobePreviewController::Shutdown()
{
	if (PreviewActor.IsValid())
	{
		PreviewActor->Destroy();
	}
	PreviewActor = nullptr;
	Viewport = nullptr;
	OwningPlayer = nullptr;
	bCameraInitialized = false;
	LastAppliedAppearanceTags.Reset();
	LastAppliedGender = ECharacterGender::UNKNOWN;
}

void UShootWardrobePreviewController::RefreshAppearance()
{
	const AShootPlayerState* PlayerState = OwningPlayer.IsValid()
		? OwningPlayer->GetPlayerState<AShootPlayerState>()
		: nullptr;
	if (PreviewActor.IsValid() && PlayerState)
	{
		const ECharacterGender Gender = PlayerState->GetCharacterGender();
		const FGameplayTagContainer& AppearanceTags = PlayerState->GetAppearanceTags(Gender);
		if (Gender == LastAppliedGender && AppearanceTags == LastAppliedAppearanceTags)
		{
			return;
		}

		// 只有装备、卸下或角色切换实际改变外观时才允许 Mutable 重新生成网格。
		// Tab/分类切换仍会刷新列表，但不能打断衣柜预览正在播放的 Idle 或试穿表演。
		PreviewActor->ApplyAppearance(Gender, AppearanceTags);
		LastAppliedGender = Gender;
		LastAppliedAppearanceTags = AppearanceTags;
	}
}

void UShootWardrobePreviewController::SetCameraMode(EShootWardrobePreviewCameraMode InCameraMode)
{
	CameraMode = InCameraMode;
	ZoomOffset = 0.f;
	ApplyCamera();
}

void UShootWardrobePreviewController::Rotate(float DeltaYawDegrees)
{
	PreviewYaw = FMath::UnwindDegrees(PreviewYaw + DeltaYawDegrees);
	ApplyCamera();
}

void UShootWardrobePreviewController::Zoom(float DeltaDistance)
{
	// 缩放范围属于交互安全边界，避免穿过角色或退到预览世界之外；按钮步长由 W_Cloth 蓝图传入。
	ZoomOffset = FMath::Clamp(ZoomOffset + DeltaDistance, -140.f, 220.f);
	ApplyCamera();
}

void UShootWardrobePreviewController::ResetView()
{
	PreviewYaw = 0.f;
	ZoomOffset = 0.f;
	ApplyCamera(!bCameraInitialized);
}

void UShootWardrobePreviewController::QueuePresentationAnimation(
	const TSoftObjectPtr<UAnimSequenceBase>& Animation)
{
	if (PreviewActor.IsValid())
	{
		PreviewActor->QueuePresentationAnimation(Animation);
	}
}

void UShootWardrobePreviewController::ClearQueuedPresentationAnimation()
{
	if (PreviewActor.IsValid())
	{
		PreviewActor->ClearQueuedPresentationAnimation();
	}
}

void UShootWardrobePreviewController::Tick(float DeltaSeconds)
{
	if (!Viewport.IsValid() || !bCameraInitialized)
	{
		return;
	}

	constexpr float CameraBlendSpeed = 8.5f;
	CurrentViewLocation = FMath::VInterpTo(CurrentViewLocation, TargetViewLocation, DeltaSeconds, CameraBlendSpeed);
	CurrentViewRotation = FMath::RInterpTo(CurrentViewRotation, TargetViewRotation, DeltaSeconds, CameraBlendSpeed);
	Viewport->SetViewLocation(CurrentViewLocation);
	Viewport->SetViewRotation(CurrentViewRotation);
}

void UShootWardrobePreviewController::ApplyCamera(bool bSnap)
{
	if (!Viewport.IsValid() || !PreviewActor.IsValid())
	{
		return;
	}

	PreviewActor->SetPreviewYaw(PreviewYaw);

	FVector BaseViewLocation;
	FRotator BaseViewRotation;
	FVector FocusTarget;
	if (!PreviewActor->GetCameraView(CameraMode, BaseViewLocation, BaseViewRotation, FocusTarget))
	{
		// 没有蓝图 CameraAnchor 时不偷偷使用 C++ 视觉坐标，配置错误应直接在预览 Actor 蓝图中修正。
		UE_LOG(LogShoot, Warning, TEXT("Wardrobe preview actor %s has no camera view for mode %d"),
			*GetNameSafe(PreviewActor.Get()), static_cast<int32>(CameraMode));
		return;
	}

	const FVector ActorLocation = PreviewActor->GetActorLocation();
	const FVector SubjectPivot(ActorLocation.X, ActorLocation.Y, FocusTarget.Z);
	FVector SubjectToCamera = BaseViewLocation - SubjectPivot;
	const float BaseDistance = SubjectToCamera.Size();
	if (!SubjectToCamera.Normalize())
	{
		return;
	}

	TargetViewLocation = SubjectPivot + SubjectToCamera * FMath::Max(50.f, BaseDistance + ZoomOffset);
	TargetViewRotation = BaseViewRotation;
	if (bSnap || !bCameraInitialized)
	{
		CurrentViewLocation = TargetViewLocation;
		CurrentViewRotation = TargetViewRotation;
		bCameraInitialized = true;
	}

	Viewport->SetViewLocation(CurrentViewLocation);
	Viewport->SetViewRotation(CurrentViewRotation);
}
