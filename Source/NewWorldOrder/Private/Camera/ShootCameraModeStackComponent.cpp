#include "Camera/ShootCameraModeStackComponent.h"

#include "Camera/CameraComponent.h"
#include "Character/ShootCharacter.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameModes/ShootExperienceDefinition.h"
#include "GameModes/ShootExperienceManagerComponent.h"
#include "Player/ShootPlayerController.h"
#include "Weapons/ShootRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootCameraModeStackComponent)

UShootCameraModeStackComponent::UShootCameraModeStackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UShootCameraModeStackComponent::BeginPlay()
{
	Super::BeginPlay();

	CharacterOwner = Cast<AShootCharacter>(GetOwner());
	if (const AShootCharacter* Character = CharacterOwner.Get())
	{
		if (const UCameraComponent* Camera = Character->GetFollowCamera())
		{
			ThirdPersonFieldOfView = Camera->FieldOfView;
		}
		if (const UCameraComponent* Camera = Character->GetFirstPersonCamera())
		{
			FirstPersonFieldOfView = Camera->FieldOfView;
		}
		if (const USpringArmComponent* CameraBoom = Character->GetCameraBoom())
		{
			ThirdPersonArmLength = CameraBoom->TargetArmLength;
			ThirdPersonTargetOffset = CameraBoom->TargetOffset;
			ThirdPersonSocketOffset = CameraBoom->SocketOffset;
		}
	}

	BindToExperienceManager();
	ApplyCameraView();
}

void UShootCameraModeStackComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UShootExperienceManagerComponent* Manager = BoundExperienceManager.Get())
	{
		Manager->OnExperienceLoaded().RemoveAll(this);
	}
	UpdateFirstPersonPresentation(false, true);
	Super::EndPlay(EndPlayReason);
}

void UShootCameraModeStackComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bExperienceBound)
	{
		BindToExperienceManager();
	}

	AShootCharacter* Character = CharacterOwner.Get();
	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}

	// Experience 可能早于客户端 Controller/Pawn 归属就绪。等本 Pawn 成为本地控制后再原子激活目标相机，
	// 避免重生首帧把远端模拟代理或错误 LocalPlayer 当作视角拥有者。
	if (!bDefaultPerspectiveApplied && bExperienceBound)
	{
		// HandleExperienceLoaded 可能发生在 Controller 尚未复制到 Pawn 时。此处在本地归属已经成立后
		// 再读一次 Controller 偏好，保证死亡重生不会被早到的 Experience 默认值覆盖。
		CurrentPerspective = ExperienceDefaultPerspective;
		if (bAllowPerspectiveSwitch)
		{
			if (const AShootPlayerController* PlayerController =
				Cast<AShootPlayerController>(Character->GetController()))
			{
				PlayerController->GetPreferredCameraPerspective(CurrentPerspective);
			}
		}
		bDefaultPerspectiveApplied = true;
		ActivatePerspective(CurrentPerspective);
	}
}

EShootCameraPerspective UShootCameraModeStackComponent::GetPerspective() const
{
	return CurrentPerspective;
}

bool UShootCameraModeStackComponent::CanTogglePerspective() const
{
	const AShootCharacter* Character = CharacterOwner.Get();
	return bAllowPerspectiveSwitch && bDefaultPerspectiveApplied && Character && Character->IsLocallyControlled();
}

void UShootCameraModeStackComponent::TogglePerspective()
{
	if (!CanTogglePerspective())
	{
		return;
	}

	const EShootCameraPerspective NewPerspective =
		GetPerspective() == EShootCameraPerspective::ThirdPerson
			? EShootCameraPerspective::FirstPerson
			: EShootCameraPerspective::ThirdPerson;

	// Pawn 会在死亡时销毁；本地偏好放在同一 LocalPlayer 的 PlayerController 上，重生后由新 Pawn 复用。
	if (const AShootCharacter* Character = CharacterOwner.Get())
	{
		if (AShootPlayerController* PlayerController = Cast<AShootPlayerController>(Character->GetController()))
		{
			PlayerController->SetPreferredCameraPerspective(NewPerspective);
		}
	}
	SetPerspective(NewPerspective);
}

void UShootCameraModeStackComponent::SetPerspective(EShootCameraPerspective NewPerspective)
{
	CurrentPerspective = NewPerspective;
	ActivatePerspective(NewPerspective);
}

void UShootCameraModeStackComponent::SetAimModeWeight(UShootRangedWeaponInstance* Weapon, float NewWeight)
{
	AimModeWeight = FMath::Clamp(NewWeight, 0.0f, 1.0f);
	AimingWeapon = AimModeWeight > KINDA_SMALL_NUMBER ? Weapon : nullptr;
	ApplyCameraView();
}

void UShootCameraModeStackComponent::BindToExperienceManager()
{
	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	UShootExperienceManagerComponent* Manager =
		GameState ? GameState->FindComponentByClass<UShootExperienceManagerComponent>() : nullptr;
	if (!Manager)
	{
		return;
	}

	BoundExperienceManager = Manager;
	bExperienceBound = true;
	Manager->CallOrRegister_OnExperienceLoaded(
		FShootExperienceLoadedDelegate::FDelegate::CreateUObject(
			this, &ThisClass::HandleExperienceLoaded));
}

void UShootCameraModeStackComponent::HandleExperienceLoaded(const UShootExperienceDefinition* Experience)
{
	if (!Experience)
	{
		return;
	}

	bAllowPerspectiveSwitch = Experience->bAllowCameraPerspectiveSwitch;
	bDefaultPerspectiveApplied = false;
	ExperienceDefaultPerspective = Experience->DefaultCameraPerspective;

	EShootCameraPerspective DesiredPerspective = ExperienceDefaultPerspective;
	if (bAllowPerspectiveSwitch)
	{
		if (const AShootCharacter* Character = CharacterOwner.Get())
		{
			if (const AShootPlayerController* PlayerController =
				Cast<AShootPlayerController>(Character->GetController()))
			{
				PlayerController->GetPreferredCameraPerspective(DesiredPerspective);
			}
		}
	}

	CurrentPerspective = DesiredPerspective;
	if (AShootCharacter* Character = CharacterOwner.Get(); Character && Character->IsLocallyControlled())
	{
		bDefaultPerspectiveApplied = true;
		ActivatePerspective(DesiredPerspective);
	}
}

void UShootCameraModeStackComponent::ApplyCameraView()
{
	AShootCharacter* Character = CharacterOwner.Get();
	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}

	float ThirdPersonFinalFieldOfView = ThirdPersonFieldOfView;
	float ThirdPersonFinalArmLength = ThirdPersonArmLength;
	FVector ThirdPersonFinalTargetOffset = ThirdPersonTargetOffset;
	FVector ThirdPersonFinalSocketOffset = ThirdPersonSocketOffset;
	float FirstPersonFinalFieldOfView = FirstPersonFieldOfView;
	if (const UShootRangedWeaponInstance* Weapon = AimingWeapon.Get())
	{
		// 第三人称 ADS 只写 FollowCamera/CameraBoom；第一人称只写独立 FirstPersonCamera 的 FOV。
		// 当前只有 Sniper 分类允许第一人称 ADS，并继续由已有 Scope UI 覆盖视图。
		ThirdPersonFinalFieldOfView = FMath::Lerp(
			ThirdPersonFieldOfView, Weapon->GetADSCameraFieldOfView(), AimModeWeight);
		ThirdPersonFinalArmLength = FMath::Lerp(
			ThirdPersonArmLength, Weapon->GetADSCameraArmLength(), AimModeWeight);
		// CameraBoom 挂在胶囊中心，第三人称 ADS 以 Pawn 眼高作为 Pivot；这是既有已验收肩射构图，
		// 与独立 FirstPersonCamera 的 Head 骨骼相对位置互不覆盖。
		const FVector ThirdPersonAimTargetOffset =
			Character->GetPawnViewLocation() - Character->GetActorLocation();
		ThirdPersonFinalTargetOffset = FMath::Lerp(
			ThirdPersonTargetOffset, ThirdPersonAimTargetOffset, AimModeWeight);
		ThirdPersonFinalSocketOffset = FMath::Lerp(
			ThirdPersonSocketOffset, Weapon->GetADSCameraSocketOffset(), AimModeWeight);

		const float FirstPersonAimWeight = Weapon->SupportsFirstPersonADS() ? AimModeWeight : 0.0f;
		FirstPersonFinalFieldOfView = FMath::Lerp(
			FirstPersonFieldOfView, Weapon->GetADSCameraFieldOfView(), FirstPersonAimWeight);
	}

	if (UCameraComponent* Camera = Character->GetFollowCamera())
	{
		Camera->SetFieldOfView(ThirdPersonFinalFieldOfView);
	}
	if (UCameraComponent* Camera = Character->GetFirstPersonCamera())
	{
		Camera->SetFieldOfView(FirstPersonFinalFieldOfView);
	}
	if (USpringArmComponent* CameraBoom = Character->GetCameraBoom())
	{
		CameraBoom->TargetArmLength = ThirdPersonFinalArmLength;
		CameraBoom->TargetOffset = ThirdPersonFinalTargetOffset;
		CameraBoom->SocketOffset = ThirdPersonFinalSocketOffset;
	}
}

void UShootCameraModeStackComponent::ActivatePerspective(EShootCameraPerspective NewPerspective)
{
	AShootCharacter* Character = CharacterOwner.Get();
	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}

	UCameraComponent* ThirdPersonCamera = Character->GetFollowCamera();
	UCameraComponent* FirstPersonCamera = Character->GetFirstPersonCamera();
	if (!ThirdPersonCamera || !FirstPersonCamera)
	{
		return;
	}

	const bool bFirstPerson = NewPerspective == EShootCameraPerspective::FirstPerson;
	if (bFirstPerson)
	{
		// 同一游戏帧内先应用 Owner 可见性和俯仰限制，再切活动相机；没有“肩后无头”过渡帧。
		UpdateFirstPersonPresentation(true);
		ThirdPersonCamera->SetActive(false);
		FirstPersonCamera->SetActive(true);
	}
	else
	{
		// 退出时先恢复第三人称输出，再显示 Head，避免第一人称相机看到头部模型内部。
		FirstPersonCamera->SetActive(false);
		ThirdPersonCamera->SetActive(true);
		UpdateFirstPersonPresentation(false);
	}
	ApplyCameraView();
}

void UShootCameraModeStackComponent::UpdateFirstPersonPresentation(bool bFirstPersonActive, bool bForceCleanup)
{
	AShootCharacter* Character = CharacterOwner.Get();
	if (!Character || (!Character->IsLocallyControlled() && !bForceCleanup))
	{
		return;
	}
	if (bFirstPersonPresentationActive != bFirstPersonActive)
	{
		bFirstPersonPresentationActive = bFirstPersonActive;
		// Mutable 主角把 Head 和 Body 分为两个渲染 Mesh。只对本 Owner 视图隐藏 Head，
		// 不使用 HideBoneByName 这种全视图状态，因此同一进程的其他分屏玩家仍能看到完整角色。
		Character->SetFirstPersonCameraActive(bFirstPersonActive);
	}

	AShootPlayerController* PlayerController = Cast<AShootPlayerController>(Character->GetController());
	if (!PlayerController)
	{
		return;
	}

	if (bFirstPersonActive)
	{
		PlayerController->ApplyFirstPersonPitchLimits(FirstPersonViewPitchMin, FirstPersonViewPitchMax);
	}
	else
	{
		PlayerController->RestoreCameraPitchLimits();
	}
}
