// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Wardrobe/ShootWardrobePreviewActor.h"

#include "Character/MutableAppearanceComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "MuCO/CustomizableSkeletalComponent.h"
#include "Animation/AnimInstance.h"
#include "ShootLogChannels.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootWardrobePreviewActor)

AShootWardrobePreviewActor::AShootWardrobePreviewActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	PreviewMeshRoot = CreateDefaultSubobject<USceneComponent>(TEXT("PreviewMeshRoot"));
	PreviewMeshRoot->SetupAttachment(SceneRoot);

	StudioFloor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StudioFloor"));
	StudioFloor->SetupAttachment(SceneRoot);
	StudioFloor->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StudioFloor->SetGenerateOverlapEvents(false);

	StudioBackdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StudioBackdrop"));
	StudioBackdrop->SetupAttachment(SceneRoot);
	StudioBackdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StudioBackdrop->SetGenerateOverlapEvents(false);

	FullBodyCameraAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("FullBodyCameraAnchor"));
	FullBodyCameraAnchor->SetupAttachment(SceneRoot);

	UpperBodyCameraAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("UpperBodyCameraAnchor"));
	UpperBodyCameraAnchor->SetupAttachment(SceneRoot);

	HeadCameraAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("HeadCameraAnchor"));
	HeadCameraAnchor->SetupAttachment(SceneRoot);

	FootwearCameraAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("FootwearCameraAnchor"));
	FootwearCameraAnchor->SetupAttachment(SceneRoot);

	PreviewHeadMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewHeadMesh"));
	PreviewHeadMesh->SetupAttachment(PreviewMeshRoot);
	PreviewHeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewHeadMesh->SetGenerateOverlapEvents(false);
	PreviewHeadMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	PreviewHeadMesh->bCastDynamicShadow = true;

	PreviewBodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PreviewBodyMesh"));
	PreviewBodyMesh->SetupAttachment(PreviewHeadMesh);
	PreviewBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewBodyMesh->SetGenerateOverlapEvents(false);
	PreviewBodyMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	PreviewBodyMesh->bCastDynamicShadow = true;
	// 预览 Actor 与正式角色共用 Mutable 异步生命周期；最终 LeaderPose 建立前不显示重复的完整兜底人物。
	PreviewBodyMesh->SetVisibility(false, true);

	PreviewBodyCSkeletalComponent = CreateDefaultSubobject<UCustomizableSkeletalComponent>(TEXT("PreviewBodyCSkeletalComponent"));
	PreviewBodyCSkeletalComponent->SetupAttachment(PreviewBodyMesh);
	PreviewBodyCSkeletalComponent->SetIsReplicated(false);

	PreviewHeadCSkeletalComponent = CreateDefaultSubobject<UCustomizableSkeletalComponent>(TEXT("PreviewHeadCSkeletalComponent"));
	PreviewHeadCSkeletalComponent->SetupAttachment(PreviewHeadMesh);
	PreviewHeadCSkeletalComponent->SetIsReplicated(false);

	PreviewAppearanceComponent = CreateDefaultSubobject<UMutableAppearanceComponent>(TEXT("PreviewAppearanceComponent"));
	PreviewAppearanceComponent->SetIsReplicated(false);

	KeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("KeyLight"));
	KeyLight->SetupAttachment(SceneRoot);

	FillLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(SceneRoot);

	RimLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("RimLight"));
	RimLight->SetupAttachment(SceneRoot);

	BackLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("BackLight"));
	BackLight->SetupAttachment(SceneRoot);

	StudioSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("StudioSkyLight"));
	StudioSkyLight->SetupAttachment(SceneRoot);

	StudioPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("StudioPostProcess"));
	StudioPostProcess->SetupAttachment(SceneRoot);
}

void AShootWardrobePreviewActor::BeginPlay()
{
	Super::BeginPlay();

	// Mutable 完成异步换网格后会先同步 Hair/Shoe 动画层，再通知本 Actor 播放待处理的试穿表演。
	PreviewAppearanceComponent->OnMutableSkeletalMeshUpdated.AddUObject(
		this, &ThisClass::HandleMutableSkeletalMeshUpdated);
}

void AShootWardrobePreviewActor::ApplyAppearance(ECharacterGender InGender,
                                                 const FGameplayTagContainer& InAppearanceTags)
{
	if (!bPreviewInitialized)
	{
		// InitializePreviewComponents 已经提交本次标签更新，首次进入不能紧接着再提交第二次异步更新。
		// 重复更新会在试穿蒙太奇开始后再次替换 Mutable 网格，表现为配置了动画却看不到或被立刻打断。
		InitializePreviewIfNeeded(InGender, InAppearanceTags);
		return;
	}

	if (PreviewAppearanceComponent)
	{
		PreviewAppearanceComponent->ApplyPreviewAppearanceTags(InGender, InAppearanceTags);
	}
}

void AShootWardrobePreviewActor::QueuePresentationAnimation(
	const TSoftObjectPtr<UAnimSequenceBase>& Animation)
{
	QueuedPresentationAnimation = Animation;
}

void AShootWardrobePreviewActor::ClearQueuedPresentationAnimation()
{
	QueuedPresentationAnimation.Reset();
}

void AShootWardrobePreviewActor::SetPreviewYaw(float InYawDegrees)
{
	if (PreviewMeshRoot)
	{
		// 资源轴向交给 BP_ShootWardrobePreviewActor 的 PreviewFacingYawOffset 调。
		// Widget 只叠加玩家拖拽旋转角，避免衣柜打开时被 C++ 固定成侧脸。
		PreviewMeshRoot->SetRelativeRotation(FRotator(0.f, InYawDegrees + PreviewFacingYawOffset, 0.f));
	}
}

bool AShootWardrobePreviewActor::GetCameraView(
	EShootWardrobePreviewCameraMode CameraMode,
	FVector& OutViewLocation,
	FRotator& OutViewRotation,
	FVector& OutFocusTarget) const
{
	const USceneComponent* CameraAnchor = ResolveCameraAnchor(CameraMode);
	if (!CameraAnchor)
	{
		return false;
	}

	// UViewport 使用预览世界坐标设置相机。Anchor 和 FocusTarget 都在 BP 子类里调，
	// Widget 根据它们恢复默认构图，并以人物轴心执行稳定 dolly。
	OutViewLocation = CameraAnchor->GetComponentLocation();
	OutViewRotation = CameraAnchor->GetComponentRotation();
	OutFocusTarget = GetActorTransform().TransformPosition(ResolveCameraFocusTarget(CameraMode));
	return true;
}

void AShootWardrobePreviewActor::InitializePreviewIfNeeded(ECharacterGender InGender,
                                                           const FGameplayTagContainer& InAppearanceTags)
{
	if (bPreviewInitialized || !PreviewAppearanceComponent)
	{
		return;
	}

	// 必须在 Mutable 初始化前交给组件预览专用基础 AnimBP。
	// 后续 Hair/Shoe AnimLayer 会链接到这个实例；不能等更新完成后再由 Actor 替换实例。
	PreviewAppearanceComponent->SetPreviewBaseAnimClasses(FemalePreviewAnimClass, MalePreviewAnimClass);
	PreviewAppearanceComponent->InitializePreviewComponents(
		PreviewHeadCSkeletalComponent,
		PreviewBodyCSkeletalComponent,
		PreviewHeadMesh,
		PreviewBodyMesh,
		InGender,
		InAppearanceTags);
	bPreviewInitialized = true;
}

void AShootWardrobePreviewActor::HandleMutableSkeletalMeshUpdated()
{
	PlayQueuedPresentationIfReady();
}

void AShootWardrobePreviewActor::PlayQueuedPresentationIfReady()
{
	if (QueuedPresentationAnimation.IsNull() || !PreviewHeadMesh)
	{
		return;
	}

	UAnimSequenceBase* Animation = QueuedPresentationAnimation.LoadSynchronous();
	if (!Animation)
	{
		UE_LOG(LogShoot, Warning, TEXT("Wardrobe preview animation failed to load: %s"),
			*QueuedPresentationAnimation.ToSoftObjectPath().ToString());
		QueuedPresentationAnimation.Reset();
		return;
	}

	UAnimInstance* PreviewAnimInstance = PreviewHeadMesh->GetAnimInstance();
	if (!PreviewAnimInstance)
	{
		// Mutable 更新回调理论上发生在基础 AnimInstance 建立后。若资产初始化异常，保留请求供下一次更新重试，
		// 不能静默丢掉这次试穿表演。
		UE_LOG(LogShoot, Warning, TEXT("Wardrobe preview animation is waiting for an AnimInstance: %s"),
			*Animation->GetPathName());
		return;
	}

	// AnimGraph 的 DefaultSlot 专门用于一次性试穿表演；蒙太奇结束后会自动回到状态机 Idle。
	PreviewAnimInstance->Montage_Stop(PresentationBlendOutTime);
	if (PreviewAnimInstance->PlaySlotAnimationAsDynamicMontage(
		Animation, TEXT("DefaultSlot"), PresentationBlendInTime, PresentationBlendOutTime, 1.f, 1))
	{
		QueuedPresentationAnimation.Reset();
	}
	else
	{
		UE_LOG(LogShoot, Warning, TEXT("Wardrobe preview failed to play animation in DefaultSlot: %s"),
			*Animation->GetPathName());
	}
}

USceneComponent* AShootWardrobePreviewActor::ResolveCameraAnchor(EShootWardrobePreviewCameraMode CameraMode) const
{
	switch (CameraMode)
	{
	case EShootWardrobePreviewCameraMode::Head:
		return HeadCameraAnchor;
	case EShootWardrobePreviewCameraMode::UpperBody:
		return UpperBodyCameraAnchor;
	case EShootWardrobePreviewCameraMode::Footwear:
		return FootwearCameraAnchor;
	case EShootWardrobePreviewCameraMode::FullBody:
	default:
		return FullBodyCameraAnchor;
	}
}

FVector AShootWardrobePreviewActor::ResolveCameraFocusTarget(EShootWardrobePreviewCameraMode CameraMode) const
{
	switch (CameraMode)
	{
	case EShootWardrobePreviewCameraMode::Head:
		return HeadCameraFocusTarget;
	case EShootWardrobePreviewCameraMode::UpperBody:
		return UpperBodyCameraFocusTarget;
	case EShootWardrobePreviewCameraMode::Footwear:
		return FootwearCameraFocusTarget;
	case EShootWardrobePreviewCameraMode::FullBody:
	default:
		return FullBodyCameraFocusTarget;
	}
}
