// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/LyraGameSettingRegistry.h"

#include "GameSetting.h"
#include "GameSettingCollection.h"
#include "GameSettingValue.h"
#include "Settings/LyraSettingsLocal.h"
#include "Settings/LyraSettingsShared.h"
#include "Player/LyraLocalPlayer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameSettingRegistry)

DEFINE_LOG_CATEGORY(LogLyraGameSettingRegistry);

namespace
{
	void GatherResettableSettingValues(UGameSetting* Parent, TArray<UGameSettingValue*>& OutSettings)
	{
		if (!Parent)
		{
			return;
		}

		if (UGameSettingValue* Value = Cast<UGameSettingValue>(Parent))
		{
			if (Value->GetEditState().IsResetable())
			{
				OutSettings.Add(Value);
			}
			return;
		}

		for (UGameSetting* Child : Parent->GetChildSettings())
		{
			GatherResettableSettingValues(Child, OutSettings);
		}
	}
}

#define LOCTEXT_NAMESPACE "Lyra"

//--------------------------------------
// ULyraGameSettingRegistry
//--------------------------------------

ULyraGameSettingRegistry::ULyraGameSettingRegistry()
{
}

ULyraGameSettingRegistry* ULyraGameSettingRegistry::Get(ULyraLocalPlayer* InLocalPlayer)
{
	ULyraGameSettingRegistry* Registry = FindObject<ULyraGameSettingRegistry>(InLocalPlayer, TEXT("LyraGameSettingRegistry"), EFindObjectFlags::ExactClass);
	if (Registry == nullptr)
	{
		Registry = NewObject<ULyraGameSettingRegistry>(InLocalPlayer, TEXT("LyraGameSettingRegistry"));
		Registry->Initialize(InLocalPlayer);
	}

	return Registry;
}

bool ULyraGameSettingRegistry::IsFinishedInitializing() const
{
	if (Super::IsFinishedInitializing())
	{
		if (ULyraLocalPlayer* LocalPlayer = Cast<ULyraLocalPlayer>(OwningLocalPlayer))
		{
			if (LocalPlayer->GetSharedSettings() == nullptr)
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

void ULyraGameSettingRegistry::OnInitialize(ULocalPlayer* InLocalPlayer)
{
	ULyraLocalPlayer* LyraLocalPlayer = Cast<ULyraLocalPlayer>(InLocalPlayer);

	VideoSettings = InitializeVideoSettings(LyraLocalPlayer);
	InitializeVideoSettings_FrameRates(VideoSettings, LyraLocalPlayer);
	RegisterSetting(VideoSettings);

	AudioSettings = InitializeAudioSettings(LyraLocalPlayer);
	RegisterSetting(AudioSettings);

	GameplaySettings = InitializeGameplaySettings(LyraLocalPlayer);
	RegisterSetting(GameplaySettings);

	MouseAndKeyboardSettings = InitializeMouseAndKeyboardSettings(LyraLocalPlayer);
	RegisterSetting(MouseAndKeyboardSettings);

	GamepadSettings = InitializeGamepadSettings(LyraLocalPlayer);
	RegisterSetting(GamepadSettings);
}

void ULyraGameSettingRegistry::SaveChanges()
{
	Super::SaveChanges();
	
	if (ULyraLocalPlayer* LocalPlayer = Cast<ULyraLocalPlayer>(OwningLocalPlayer))
	{
		// Game user settings need to be applied to handle things like resolution, this saves indirectly
		LocalPlayer->GetLocalSettings()->ApplySettings(false);
		
		LocalPlayer->GetSharedSettings()->ApplySettings();
		LocalPlayer->GetSharedSettings()->SaveSettings();
	}
}

TArray<UGameSettingValue*> ULyraGameSettingRegistry::GetAllResettableSettings() const
{
	TArray<UGameSettingValue*> ResettableSettings;
	for (UGameSetting* TopLevelSetting : TopLevelSettings)
	{
		GatherResettableSettingValues(TopLevelSetting, ResettableSettings);
	}
	return ResettableSettings;
}

#undef LOCTEXT_NAMESPACE

