// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/LyraActivatableWidget.h"

#include "ShootFrontEndScreen.generated.h"

class UCommonActivatableWidget;

/** W_FrontEnd 的最小 C++ 父类；固定 OptionsButton 仍由蓝图调用 OpenSettingsScreen。 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootFrontEndScreen : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="FrontEnd|Navigation")
	void OpenSettingsScreen();

protected:
	/** W_FrontEnd 蓝图配置迁移后的 W_LyraSettingScreen，不在 C++ 写死资产路径。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FrontEnd|Navigation")
	TSoftClassPtr<UCommonActivatableWidget> SettingsScreenClass;
};
