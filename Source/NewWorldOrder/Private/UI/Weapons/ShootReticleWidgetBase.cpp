#include "UI/Weapons/ShootReticleWidgetBase.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "ShootGameplayTags.h"
#include "Weapons/ShootRangedWeaponInstance.h"

UShootReticleWidgetBase::UShootReticleWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootReticleWidgetBase::InitializeFromWeapon(UShootRangedWeaponInstance* InWeapon)
{
	WeaponInstance = InWeapon;
	InventoryInstance = nullptr;
	if (WeaponInstance)
	{
		InventoryInstance = Cast<UShootInventoryItemInstance>(WeaponInstance->GetInstigator());
	}
	OnWeaponInitialized();
}

float UShootReticleWidgetBase::ComputeSpreadAngle() const
{
	if (WeaponInstance)
	{
		const float BaseSpreadAngle = WeaponInstance->GetCalculatedSpreadAngle();
		const float SpreadAngleMultiplier = WeaponInstance->GetCalculatedSpreadAngleMultiplier();
		return BaseSpreadAngle * SpreadAngleMultiplier;
	}
	return 0.0f;
}

float UShootReticleWidgetBase::ComputeMaxScreenspaceSpreadRadius() const
{
	const float LongShotDistance = 10000.0f;

	APlayerController* OwningController = GetOwningPlayer();
	if (OwningController && OwningController->PlayerCameraManager)
	{
		// 沿用 Lyra 的散布锥投影公式：取 100 米处锥体上边缘并投影回本地玩家屏幕。
		const float SpreadRadiusRadians = FMath::DegreesToRadians(ComputeSpreadAngle() * 0.5f);
		const float SpreadRadiusAtDistance = FMath::Tan(SpreadRadiusRadians) * LongShotDistance;

		FVector CameraLocation;
		FRotator CameraRotation;
		OwningController->PlayerCameraManager->GetCameraViewPoint(CameraLocation, CameraRotation);
		const FVector CameraForward = CameraRotation.RotateVector(FVector::ForwardVector);
		const FVector CameraUp = CameraRotation.RotateVector(FVector::UpVector);

		const FVector OffsetTargetAtDistance =
			CameraLocation + (CameraForward * LongShotDistance) + (CameraUp * SpreadRadiusAtDistance);

		FVector2D OffsetTargetScreenspace;
		if (OwningController->ProjectWorldLocationToScreen(OffsetTargetAtDistance, OffsetTargetScreenspace, true))
		{
			int32 ViewportSizeX = 0;
			int32 ViewportSizeY = 0;
			OwningController->GetViewportSize(ViewportSizeX, ViewportSizeY);

			// UE 的 GetViewportSize 返回整窗像素尺寸，而上面的 true 让投影结果处于当前玩家子视口的局部坐标。
			// Lyra 只需处理单一全屏玩家，两者天然一致；NWO 分屏时必须先乘 LocalPlayer 的归一化子视口尺寸，
			// 否则会拿整窗中心减子视口坐标，CrossHairs 半径被放大到屏幕外，只剩被蓝图钳制过的 Outer。
			FVector2D PlayerViewportSize(
				static_cast<FVector::FReal>(ViewportSizeX),
				static_cast<FVector::FReal>(ViewportSizeY));
			if (const ULocalPlayer* LocalPlayer = OwningController->GetLocalPlayer())
			{
				PlayerViewportSize.X *= LocalPlayer->Size.X;
				PlayerViewportSize.Y *= LocalPlayer->Size.Y;
			}

			const FVector2D ScreenCenter = PlayerViewportSize * 0.5f;
			return (OffsetTargetScreenspace - ScreenCenter).Length();
		}
	}
	return 0.0f;
}

bool UShootReticleWidgetBase::HasFirstShotAccuracy() const
{
	return WeaponInstance && WeaponInstance->HasFirstShotAccuracy();
}

void UShootReticleWidgetBase::OnReticleADSVisualChanged_Implementation(bool bIsADS)
{
	// 通用准星没有强制 ADS 视觉；具体蓝图或 C++ 子类按武器表现覆盖。
}

void UShootReticleWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
	HitNotifyHandle = MessageSubsystem.RegisterListener<FShootReticleHitNotifyMessage>(
		GameplayTags.Msg_UI_Reticle_HitNotify, this, &ThisClass::HandleHitNotificationMessage);
	AdsHandle = MessageSubsystem.RegisterListener<FShootReticleADSMessage>(
		GameplayTags.Msg_UI_Reticle_ADS, this, &ThisClass::HandleAdsMessage);
	EliminationHandle = MessageSubsystem.RegisterListener<FShootReticleEliminationMessage>(
		GameplayTags.Msg_UI_Reticle_Elimination, this, &ThisClass::HandleEliminationMessage);
}

void UShootReticleWidgetBase::NativeDestruct()
{
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.UnregisterListener(HitNotifyHandle);
	MessageSubsystem.UnregisterListener(AdsHandle);
	MessageSubsystem.UnregisterListener(EliminationHandle);

	Super::NativeDestruct();
}

void UShootReticleWidgetBase::HandleHitNotificationMessage(FGameplayTag, const FShootReticleHitNotifyMessage& Message)
{
	if (Message.SourceActor.IsValid() && Message.SourceActor.Get() != GetOwningPlayerPawn())
	{
		return;
	}

	FShootReticleHitNotifyMessage LocalMessage = Message;
	if (APlayerController* OwningController = GetOwningPlayer())
	{
		for (FShootReticleHitLocation& HitLocation : LocalMessage.HitMarkers)
		{
			if (HitLocation.bHasWorldPosition)
			{
				OwningController->ProjectWorldLocationToScreen(HitLocation.WorldPosition, HitLocation.ScreenPosition, true);
			}
		}
	}

	OnReticleHitNotification(LocalMessage);
}

void UShootReticleWidgetBase::HandleAdsMessage(FGameplayTag, const FShootReticleADSMessage& Message)
{
	if (!Message.SourceActor.IsValid() || Message.SourceActor.Get() == GetOwningPlayerPawn())
	{
		OnReticleADSVisualChanged(Message.bIsAds);
	}
}

void UShootReticleWidgetBase::HandleEliminationMessage(FGameplayTag, const FShootReticleEliminationMessage& Message)
{
	if (!Message.Instigator.IsValid() || Message.Instigator.Get() == GetOwningPlayerPawn())
	{
		OnReticleEliminationVisual();
	}
}
