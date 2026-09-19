// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "CommonGameViewportClient.h"
//#include "CommonInputTypeEnum.h"
#include "CustomGameViewportClient.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FShootViewportInputKeyDelegate, const FInputKeyEventArgs&);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FShootInputDeviceConnectionDelegate,
	EInputDeviceConnectionState, FPlatformUserId, FInputDeviceId);
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
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;

	virtual void RemapControllerInput(FInputKeyEventArgs& InOutKeyEvent) override;

	void HandleInputDeviceConnectionChange(EInputDeviceConnectionState NewConnectionState,
	                                       FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);

	void SetDisableSplitScreen(bool bDisableSplitScreen);

	/**
	 * 同屏多设备配置页需要在第二位 LocalPlayer 创建前识别输入设备。
	 * 这里只广播原始设备身份；具体按键语义仍由页面配置的 InputAction/IMC 解析，
	 * 禁止在 ViewportClient 中写死键盘或手柄按键。
	 */
	FShootViewportInputKeyDelegate& OnViewportInputKey() { return ViewportInputKeyDelegate; }
	FShootInputDeviceConnectionDelegate& OnInputDeviceConnectionChanged() { return InputDeviceConnectionDelegate; }

	virtual void Init(struct FWorldContext& WorldContext, UGameInstance* OwningGameInstance, bool bCreateNewAudioDevice = true) override;

private:
	FShootViewportInputKeyDelegate ViewportInputKeyDelegate;
	FShootInputDeviceConnectionDelegate InputDeviceConnectionDelegate;
};
