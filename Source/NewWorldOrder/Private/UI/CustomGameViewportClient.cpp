// Copyright ZhaoYiJie


#include "UI/CustomGameViewportClient.h"

#include "CommonUISettings.h"
#include "ICommonUIModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CustomGameViewportClient)

namespace GameViewportTags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Platform_Trait_Input_HardwareCursor, "Platform.Trait.Input.HardwareCursor");
}

UCustomGameViewportClient::UCustomGameViewportClient()
{
	const IPlatformInputDeviceMapper& PlatformInputMapper = IPlatformInputDeviceMapper::Get();
	PlatformInputMapper.GetOnInputDeviceConnectionChange().AddUObject(
		this, &UCustomGameViewportClient::HandleInputDeviceConnectionChange);
}

void UCustomGameViewportClient::UpdateActiveSplitscreenType()
{
	Super::UpdateActiveSplitscreenType();
}

void UCustomGameViewportClient::RemapControllerInput(FInputKeyEventArgs& InOutKeyEvent)
{
	Super::RemapControllerInput(InOutKeyEvent);

	/*ECommonInputType CommonInputType = GetInputTypeFromKey(InOutKeyEvent.Key);
	int32 DeviceId = InOutKeyEvent.InputDevice.GetId();
	int32 ControllerId = InOutKeyEvent.ControllerId;
	RemapInputKeyDelegate.Broadcast(CommonInputType, DeviceId, ControllerId);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red,
		                                 FString::Printf(
			                                 TEXT("CommonInputType = %d, DeviceId = %d, ControllerId = %d"),
			                                 CommonInputType, DeviceId, ControllerId));
	}*/

	/*const int32 NumLocalPlayers = World ? World->GetGameInstance()->GetNumLocalPlayers() : 0;

	if (NumLocalPlayers > 1 && InOutKeyEvent.Key.IsGamepadKey())
	{
		if (InOutKeyEvent.InputDevice.GetId() == 0)
		{
		}
		if (InOutKeyEvent.ControllerId == 1)
		{
		}
		else if (InOutKeyEvent.ControllerId == 2)
		{
			InOutKeyEvent.InputDevice = FInputDeviceId::CreateFromInternalId(0);

			IPlatformInputDeviceMapper& PlatformInputDeviceMapper = IPlatformInputDeviceMapper::Get();
			// Determine the owning user for this input device
			FPlatformUserId OwningUser = PlatformInputDeviceMapper.GetUserForInputDevice(InOutKeyEvent.InputDevice);
			
			if (!OwningUser.IsValid())
			{
				UE_LOG(LogTemp, Warning,
				       TEXT(
					       "No second gamepad input device detected, attempting to generate a placeholder for the second device for switching."
				       ))
				OwningUser = FPlatformUserId::CreateFromInternalId(0);
				//2号控制器控制第0个玩家
				PlatformInputDeviceMapper.Internal_MapInputDeviceToUser(InOutKeyEvent.InputDevice, OwningUser,
				  
				                                                        EInputDeviceConnectionState::Connected);
			}
		}
	}*/
}

void UCustomGameViewportClient::HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState,
                                                                  FPlatformUserId PlatformUserId,
                                                                  FInputDeviceId InputDeviceId)
{

}

void UCustomGameViewportClient::SetDisableSplitScreen(bool bDisableSplitScreen)
{
	SetForceDisableSplitscreen(bDisableSplitScreen);
}

void UCustomGameViewportClient::Init(FWorldContext& WorldContext, UGameInstance* OwningGameInstance,
	bool bCreateNewAudioDevice)
{
	Super::Init(WorldContext, OwningGameInstance, bCreateNewAudioDevice);
	
	// We have software cursors set up in our project settings for console/mobile use, but on desktop we're fine with
	// the standard hardware cursors
	const bool UseHardwareCursor = ICommonUIModule::GetSettings().GetPlatformTraits().HasTag(GameViewportTags::TAG_Platform_Trait_Input_HardwareCursor);
	SetUseSoftwareCursorWidgets(!UseHardwareCursor);
}

/*ECommonInputType UCustomGameViewportClient::GetInputTypeFromKey(const FKey& Key) const
{
	if (!Key.IsValid())
	{
		return ECommonInputType::Count;
	}

	if (Key.IsTouch())
	{
		return ECommonInputType::Touch;
	}
	else if (Key.IsGamepadKey())
	{
		return ECommonInputType::Gamepad;
	}
	else if (!Key.IsTouch() && !Key.IsGamepadKey())
	{
		return ECommonInputType::MouseAndKeyboard;
	}
	else
	{
		return ECommonInputType::Count;
	}
}*/
