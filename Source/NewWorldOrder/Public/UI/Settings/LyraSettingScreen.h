// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataTable.h"
#include "Widgets/GameSettingScreen.h"

#include "LyraSettingScreen.generated.h"

class UGameSettingRegistry;
class ULyraTabListWidgetBase;
class UObject;
enum class ECommonMessagingResult : uint8;

UCLASS(Abstract, meta = (Category = "Settings", DisableNativeTick))
class NEWWORLDORDER_API ULyraSettingScreen : public UGameSettingScreen
{
	GENERATED_BODY()

public:

protected:
	virtual void NativeOnInitialized() override;
	virtual UGameSettingRegistry* CreateRegistry() override;

	void HandleBackAction();
	void HandleApplyAction();
	void HandleCancelChangesAction();
	void HandleResetToDefaultsAction();
	void HandleResetToDefaultsConfirmation(ECommonMessagingResult Result);
	void ResetAllSettingsToDefaults();

	virtual void OnSettingsDirtyStateChanged_Implementation(bool bSettingsDirty) override;
	
protected:
	UPROPERTY(BlueprintReadOnly, Category = Input, meta = (BindWidget, OptionalWidget = true, AllowPrivateAccess = true))
	TObjectPtr<ULyraTabListWidgetBase> TopSettingsTabs;
	
	UPROPERTY(EditDefaultsOnly)
	FDataTableRowHandle BackInputActionData;

	UPROPERTY(EditDefaultsOnly)
	FDataTableRowHandle ApplyInputActionData;

	UPROPERTY(EditDefaultsOnly)
	FDataTableRowHandle CancelChangesInputActionData;

	/**
	 * 使用 /Game/UI/DT_UniversalActions.Input_ResetDefaults，由 CommonBoundActionBar 自动生成跨设备按钮。
	 * 确认后只修改当前设置事务；玩家仍需 Apply 保存，也可以用 Cancel 撤销。
	 */
	UPROPERTY(EditDefaultsOnly)
	FDataTableRowHandle ResetToDefaultsInputActionData;

	FUIActionBindingHandle BackHandle;
	FUIActionBindingHandle ApplyHandle;
	FUIActionBindingHandle CancelChangesHandle;
	FUIActionBindingHandle ResetToDefaultsHandle;
};
