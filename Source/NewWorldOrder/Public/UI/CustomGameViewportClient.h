// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "CommonGameViewportClient.h"
//#include "CommonInputTypeEnum.h"
#include "CustomGameViewportClient.generated.h"

//DECLARE_MULTICAST_DELEGATE_ThreeParams(FRemapInputKeyDelegate, ECommonInputType CommonInputType, int32 DeviceId,int32 ControllerId);
/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API UCustomGameViewportClient : public UCommonGameViewportClient
{
	GENERATED_BODY()

public:
	UCustomGameViewportClient();

	virtual void UpdateActiveSplitscreenType() override;

	virtual void RemapControllerInput(FInputKeyEventArgs& InOutKeyEvent) override;

	void HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState,
	                                       FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);

	void SetDisableSplitScreen(bool bDisableSplitScreen);

	virtual void Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice = true) override;
	
	//ECommonInputType GetInputTypeFromKey(const FKey& Key) const;

	//FRemapInputKeyDelegate RemapInputKeyDelegate;
};
