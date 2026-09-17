// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/LyraActivatableWidget.h"
#include "ShootMainMenuScreen.generated.h"

class UCommonButtonBase;
enum class ECommonMessagingResult : uint8;

/**
 * 玩家私有游戏菜单。HomeMap 显示联机、衣柜与返回 FrontEnd；副本/Lobby 显示设置与退出副本。
 * 固定按钮的 OnClicked 由 WBP_GameMenu EventGraph 连接到公开动作；C++ 负责上下文状态和权威退出链。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootMainMenuScreen : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Game Menu|Navigation")
	void OpenSessionScreen();

	UFUNCTION(BlueprintCallable, Category="Game Menu|Navigation")
	void OpenWardrobeScreen();

	UFUNCTION(BlueprintCallable, Category="Game Menu|Navigation")
	void OpenSettingsScreen();

	/** 显示角色对应的确认弹框；确认后 Local 返回 HomeMap，Host 销毁房间，Client 只离开自己。 */
	UFUNCTION(BlueprintCallable, Category="Game Menu|Expedition")
	void RequestExitExpedition();

	UFUNCTION(BlueprintPure, Category="Game Menu|Expedition")
	bool IsInExpedition() const;

	UFUNCTION(BlueprintCallable, Category="Game Menu|Navigation")
	void CloseScreen();

protected:
	virtual void NativeOnActivated() override;
	virtual bool NativeOnHandleBackAction() override;

	UPROPERTY(EditDefaultsOnly, Category="Menu|Navigation")
	TSoftClassPtr<UCommonActivatableWidget> SessionScreenClass;

	UPROPERTY(EditDefaultsOnly, Category="Menu|Navigation")
	TSoftClassPtr<UCommonActivatableWidget> WardrobeScreenClass;

	UPROPERTY(EditDefaultsOnly, Category="Menu|Navigation")
	TSoftClassPtr<UCommonActivatableWidget> SettingsScreenClass;

	/** 以下控件只由 C++ 切换业务上下文；文案、顺序和样式仍由 WBP_GameMenu 维护。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> OnlineButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> WardrobeButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> ReturnMainMenuButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> ExitExpeditionButton;

private:
	void PushChildScreen(const TSoftClassPtr<UCommonActivatableWidget>& ScreenClass);
	void HandleExitExpeditionConfirmation(ECommonMessagingResult Result);
	void ExitExpeditionAfterConfirmation();
	void UpdateMenuContext();
	void ClearRuntimeSessionForExit() const;
};
