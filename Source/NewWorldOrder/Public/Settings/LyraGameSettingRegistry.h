// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Ticker.h"
#include "DataSource/GameSettingDataSourceDynamic.h" // IWYU pragma: keep
#include "GameSettingRegistry.h"
#include "Settings/LyraSettingsLocal.h" // IWYU pragma: keep

#include "LyraGameSettingRegistry.generated.h"

class ULocalPlayer;
class UObject;

//--------------------------------------
// ULyraGameSettingRegistry
//--------------------------------------

class UGameSettingCollection;
class UGameSettingValue;
class ULyraLocalPlayer;

DECLARE_LOG_CATEGORY_EXTERN(LogLyraGameSettingRegistry, Log, Log);

#define GET_SHARED_SETTINGS_FUNCTION_PATH(FunctionOrPropertyName)							\
	MakeShared<FGameSettingDataSourceDynamic>(TArray<FString>({								\
		GET_FUNCTION_NAME_STRING_CHECKED(ULyraLocalPlayer, GetSharedSettings),				\
		GET_FUNCTION_NAME_STRING_CHECKED(ULyraSettingsShared, FunctionOrPropertyName)		\
	}))

#define GET_LOCAL_SETTINGS_FUNCTION_PATH(FunctionOrPropertyName)							\
	MakeShared<FGameSettingDataSourceDynamic>(TArray<FString>({								\
		GET_FUNCTION_NAME_STRING_CHECKED(ULyraLocalPlayer, GetLocalSettings),				\
		GET_FUNCTION_NAME_STRING_CHECKED(ULyraSettingsLocal, FunctionOrPropertyName)		\
	}))

/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API ULyraGameSettingRegistry : public UGameSettingRegistry
{
	GENERATED_BODY()

public:
	ULyraGameSettingRegistry();

	static ULyraGameSettingRegistry* Get(ULyraLocalPlayer* InLocalPlayer);
	
	virtual void SaveChanges() override;

	/**
	 * 收集五个设置分类下全部允许恢复默认值的叶子设置。
	 * 全局 Reset Defaults 必须从 Registry 的真实树递归收集，不能在页面里硬编码分类或条目名单；
	 * 这样后续 Data Driven Registry 新增条目时会自动进入同一事务。
	 */
	TArray<UGameSettingValue*> GetAllResettableSettings() const;

protected:
	virtual void OnInitialize(ULocalPlayer* InLocalPlayer) override;
	virtual bool IsFinishedInitializing() const override;

	UGameSettingCollection* InitializeVideoSettings(ULyraLocalPlayer* InLocalPlayer);
	void InitializeVideoSettings_FrameRates(UGameSettingCollection* Screen, ULyraLocalPlayer* InLocalPlayer);
	UGameSettingCollection* InitializeAudioSettings(ULyraLocalPlayer* InLocalPlayer);
	UGameSettingCollection* InitializeGameplaySettings(ULyraLocalPlayer* InLocalPlayer);

	UGameSettingCollection* InitializeMouseAndKeyboardSettings(ULyraLocalPlayer* InLocalPlayer);
	UGameSettingCollection* InitializeGamepadSettings(ULyraLocalPlayer* InLocalPlayer);

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> VideoSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> AudioSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> GameplaySettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> MouseAndKeyboardSettings;

	UPROPERTY()
	TObjectPtr<UGameSettingCollection> GamepadSettings;

};
