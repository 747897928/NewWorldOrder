// Copyright Epic Games, Inc. All Rights Reserved.

#include "Settings/LyraSettingsShared.h"

#include "Framework/Application/SlateApplication.h"
#include "Internationalization/Culture.h"
#include "Internationalization/TextLocalizationManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystemSteam.h"
#include "Player/LyraLocalPlayer.h"
#include "Rendering/SlateRenderer.h"
#include "SubtitleDisplaySubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#if NEWWORLDORDER_WITH_STEAM
#include "steam/steam_api.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraSettingsShared)

static FString SHARED_SETTINGS_SLOT_NAME = TEXT("SharedGameSettings");

DEFINE_LOG_CATEGORY_STATIC(LogLyraSettingsShared, Log, All);

namespace
{
	FString GetCultureFromSteamLanguage()
	{
#if NEWWORLDORDER_WITH_STEAM
		if (IsRunningDedicatedServer() || !SteamAPI_IsSteamRunning())
		{
			return FString();
		}

		// OnlineSubsystemSteam 负责 Steam API 生命周期；这里只在客户端接口已完成初始化后读取应用语言。
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(STEAM_SUBSYSTEM);
		if (!OnlineSubsystem || OnlineSubsystem->GetSubsystemName() != STEAM_SUBSYSTEM)
		{
			return FString();
		}

		FOnlineSubsystemSteam* SteamSubsystem = static_cast<FOnlineSubsystemSteam*>(OnlineSubsystem);
		if (!SteamSubsystem->IsSteamClientAvailable() || !SteamApps())
		{
			return FString();
		}

		const char* CurrentGameLanguage = SteamApps()->GetCurrentGameLanguage();
		if (!CurrentGameLanguage)
		{
			return FString();
		}

		const FString SteamLanguage = UTF8_TO_TCHAR(CurrentGameLanguage);
		if (SteamLanguage.Equals(TEXT("english"), ESearchCase::IgnoreCase))
		{
			return TEXT("en");
		}
		if (SteamLanguage.Equals(TEXT("schinese"), ESearchCase::IgnoreCase))
		{
			return TEXT("zh-Hans");
		}
		if (SteamLanguage.Equals(TEXT("tchinese"), ESearchCase::IgnoreCase))
		{
			return TEXT("zh-Hant");
		}
		if (SteamLanguage.Equals(TEXT("russian"), ESearchCase::IgnoreCase))
		{
			return TEXT("ru");
		}
		if (SteamLanguage.Equals(TEXT("spanish"), ESearchCase::IgnoreCase))
		{
			return TEXT("es");
		}
		if (SteamLanguage.Equals(TEXT("brazilian"), ESearchCase::IgnoreCase))
		{
			return TEXT("pt-BR");
		}
		if (SteamLanguage.Equals(TEXT("german"), ESearchCase::IgnoreCase))
		{
			return TEXT("de");
		}
		if (SteamLanguage.Equals(TEXT("japanese"), ESearchCase::IgnoreCase))
		{
			return TEXT("ja");
		}
		if (SteamLanguage.Equals(TEXT("french"), ESearchCase::IgnoreCase))
		{
			return TEXT("fr");
		}
		if (SteamLanguage.Equals(TEXT("polish"), ESearchCase::IgnoreCase))
		{
			return TEXT("pl");
		}
		if (SteamLanguage.Equals(TEXT("koreana"), ESearchCase::IgnoreCase))
		{
			return TEXT("ko");
		}
#endif

		return FString();
	}

	FString FindAvailableCulture(const FString& RequestedCulture, const TArray<FString>& AvailableCultures)
	{
		if (RequestedCulture.IsEmpty())
		{
			return FString();
		}

		for (const FString& CultureName : FInternationalization::Get().GetPrioritizedCultureNames(RequestedCulture))
		{
			if (AvailableCultures.Contains(CultureName))
			{
				return CultureName;
			}
		}

		return FString();
	}
}

namespace LyraSettingsSharedCVars
{
	static float DefaultGamepadLeftStickInnerDeadZone = 0.25f;
	static FAutoConsoleVariableRef CVarGamepadLeftStickInnerDeadZone(
		TEXT("gpad.DefaultLeftStickInnerDeadZone"),
		DefaultGamepadLeftStickInnerDeadZone,
		TEXT("Gamepad left stick inner deadzone")
	);

	static float DefaultGamepadRightStickInnerDeadZone = 0.25f;
	static FAutoConsoleVariableRef CVarGamepadRightStickInnerDeadZone(
		TEXT("gpad.DefaultRightStickInnerDeadZone"),
		DefaultGamepadRightStickInnerDeadZone,
		TEXT("Gamepad right stick inner deadzone")
	);	
}

ULyraSettingsShared::ULyraSettingsShared()
{
	FInternationalization::Get().OnCultureChanged().AddUObject(this, &ThisClass::OnCultureChanged);

	GamepadMoveStickDeadZone = LyraSettingsSharedCVars::DefaultGamepadLeftStickInnerDeadZone;
	GamepadLookStickDeadZone = LyraSettingsSharedCVars::DefaultGamepadRightStickInnerDeadZone;
}

int32 ULyraSettingsShared::GetLatestDataVersion() const
{
	// 0 = before subclassing ULocalPlayerSaveGame
	// 1 = first proper version
	return 1;
}

ULyraSettingsShared* ULyraSettingsShared::CreateTemporarySettings(const ULyraLocalPlayer* LocalPlayer)
{
	// This is not loaded from disk but should be set up to save
	ULyraSettingsShared* SharedSettings = Cast<ULyraSettingsShared>(CreateNewSaveGameForLocalPlayer(ULyraSettingsShared::StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME));

	SharedSettings->ApplySettings();

	return SharedSettings;
}

ULyraSettingsShared* ULyraSettingsShared::LoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer)
{
	// This will stall the main thread while it loads
	ULyraSettingsShared* SharedSettings = Cast<ULyraSettingsShared>(LoadOrCreateSaveGameForLocalPlayer(ULyraSettingsShared::StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME));

	SharedSettings->ApplySettings();

	return SharedSettings;
}

bool ULyraSettingsShared::AsyncLoadOrCreateSettings(const ULyraLocalPlayer* LocalPlayer, FOnSettingsLoadedEvent Delegate)
{
	FOnLocalPlayerSaveGameLoadedNative Lambda = FOnLocalPlayerSaveGameLoadedNative::CreateLambda([Delegate]
		(ULocalPlayerSaveGame* LoadedSave)
		{
			ULyraSettingsShared* LoadedSettings = CastChecked<ULyraSettingsShared>(LoadedSave);
			
			LoadedSettings->ApplySettings();

			Delegate.ExecuteIfBound(LoadedSettings);
		});

	return ULocalPlayerSaveGame::AsyncLoadOrCreateSaveGameForLocalPlayer(ULyraSettingsShared::StaticClass(), LocalPlayer, SHARED_SETTINGS_SLOT_NAME, Lambda);
}

FString ULyraSettingsShared::ResolveDefaultCulture()
{
	const TArray<FString> AvailableCultures = FTextLocalizationManager::Get().GetLocalizedCultureNames(ELocalizationLoadFlags::Game);

	const FString SteamCulture = FindAvailableCulture(GetCultureFromSteamLanguage(), AvailableCultures);
	if (!SteamCulture.IsEmpty())
	{
		UE_LOG(LogLyraSettingsShared, Log, TEXT("首启语言使用 Steam 应用语言映射：%s"), *SteamCulture);
		return SteamCulture;
	}

	const FString SystemCulture = FInternationalization::Get().GetDefaultCulture()->GetName();
	const FString SystemCultureMatch = FindAvailableCulture(SystemCulture, AvailableCultures);
	if (!SystemCultureMatch.IsEmpty())
	{
		UE_LOG(LogLyraSettingsShared, Log, TEXT("首启语言使用系统语言：%s -> %s"), *SystemCulture, *SystemCultureMatch);
		return SystemCultureMatch;
	}

	const FString SimplifiedChineseCulture = FindAvailableCulture(TEXT("zh-Hans"), AvailableCultures);
	if (!SimplifiedChineseCulture.IsEmpty())
	{
		UE_LOG(LogLyraSettingsShared, Log, TEXT("Steam 和系统语言未匹配首发语言，回退简体中文"));
		return SimplifiedChineseCulture;
	}

	const FString EnglishCulture = FindAvailableCulture(TEXT("en"), AvailableCultures);
	if (!EnglishCulture.IsEmpty())
	{
		UE_LOG(LogLyraSettingsShared, Warning, TEXT("缺少简体中文本地化资源，回退英语：%s"), *EnglishCulture);
		return EnglishCulture;
	}

	if (AvailableCultures.Num() > 0)
	{
		UE_LOG(LogLyraSettingsShared, Warning, TEXT("缺少默认本地化资源，使用首个可用 Culture：%s"), *AvailableCultures[0]);
		return AvailableCultures[0];
	}

	// 没有 Game locres 时仍返回 NativeCulture，保证引擎没有可用资源时可以正常启动。
	UE_LOG(LogLyraSettingsShared, Error, TEXT("没有可用的 Game 本地化资源，使用 NativeCulture en"));
	return TEXT("en");
}

void ULyraSettingsShared::SaveSettings()
{
	// Schedule an async save because it's okay if it fails
	AsyncSaveGameToSlotForLocalPlayer();

	// TODO_BH: Move this to the serialize function instead with a bumped version number
	if (UEnhancedInputLocalPlayerSubsystem* System = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwningPlayer))
	{
		if (UEnhancedInputUserSettings* InputSettings = System->GetUserSettings())
		{
			InputSettings->AsyncSaveSettings();
		}
	}
}

void ULyraSettingsShared::ApplySettings()
{
	ApplySubtitleOptions();
	ApplyBackgroundAudioSettings();
	ApplyCultureSettings();

	if (UEnhancedInputLocalPlayerSubsystem* System = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwningPlayer))
	{
		if (UEnhancedInputUserSettings* InputSettings = System->GetUserSettings())
		{
			InputSettings->ApplySettings();
		}
	}
}

void ULyraSettingsShared::SetColorBlindStrength(int32 InColorBlindStrength)
{
	InColorBlindStrength = FMath::Clamp(InColorBlindStrength, 0, 10);
	if (ColorBlindStrength != InColorBlindStrength)
	{
		ColorBlindStrength = InColorBlindStrength;
		FSlateApplication::Get().GetRenderer()->SetColorVisionDeficiencyType(
			(EColorVisionDeficiency)(int32)ColorBlindMode, (int32)ColorBlindStrength, true, false);
	}
}

void ULyraSettingsShared::SetGamepadInputAPIOption(const ELyraGamepadInputAPIOption NewValue)
{
	const bool bWasValueChanged = ChangeValueAndDirty(GamepadInputAPIOptions, NewValue);

	// We dont have any other additional work to do if the value wasn't changed.
	if (!bWasValueChanged)
	{
		return;
	}

	// A comma-separated list of preferred gamepad APIs
	FString GamepadAPIOptions = TEXT("");

	switch (NewValue)
	{
	case ELyraGamepadInputAPIOption::Legacy:
		GamepadAPIOptions = TEXT("XInput,WinDualShock");
		break;
	case ELyraGamepadInputAPIOption::Modern:
		GamepadAPIOptions = TEXT("GameInput");
		break;
	default:
		checkNoEntry();
		break;
	}

	FGenericPlatformMisc::SetPreferredInputDevices(*GamepadAPIOptions);
}

int32 ULyraSettingsShared::GetColorBlindStrength() const
{
	return ColorBlindStrength;
}

void ULyraSettingsShared::SetColorBlindMode(EColorBlindMode InMode)
{
	if (ColorBlindMode != InMode)
	{
		ColorBlindMode = InMode;
		FSlateApplication::Get().GetRenderer()->SetColorVisionDeficiencyType(
			(EColorVisionDeficiency)(int32)ColorBlindMode, (int32)ColorBlindStrength, true, false);
	}
}

EColorBlindMode ULyraSettingsShared::GetColorBlindMode() const
{
	return ColorBlindMode;
}

void ULyraSettingsShared::ApplySubtitleOptions()
{
	if (USubtitleDisplaySubsystem* SubtitleSystem = USubtitleDisplaySubsystem::Get(OwningPlayer))
	{
		FSubtitleFormat SubtitleFormat;
		SubtitleFormat.SubtitleTextSize = SubtitleTextSize;
		SubtitleFormat.SubtitleTextColor = SubtitleTextColor;
		SubtitleFormat.SubtitleTextBorder = SubtitleTextBorder;
		SubtitleFormat.SubtitleBackgroundOpacity = SubtitleBackgroundOpacity;

		SubtitleSystem->SetSubtitleDisplayOptions(SubtitleFormat);
	}
}

//////////////////////////////////////////////////////////////////////

void ULyraSettingsShared::SetAllowAudioInBackgroundSetting(ELyraAllowBackgroundAudioSetting NewValue)
{
	if (ChangeValueAndDirty(AllowAudioInBackground, NewValue))
	{
		ApplyBackgroundAudioSettings();
	}
}

void ULyraSettingsShared::ApplyBackgroundAudioSettings()
{
	if (OwningPlayer && OwningPlayer->IsPrimaryPlayer())
	{
		FApp::SetUnfocusedVolumeMultiplier((AllowAudioInBackground != ELyraAllowBackgroundAudioSetting::Off) ? 1.0f : 0.0f);
	}
}

//////////////////////////////////////////////////////////////////////

void ULyraSettingsShared::ApplyCultureSettings()
{
	if (bResetToDefaultCulture)
	{
		// 编辑器内保留 Lyra 的系统默认语义，正式游戏则复用项目的 Steam -> 系统 -> 简中策略。
		const FString CultureToApply = GIsEditor
			? FInternationalization::Get().GetDefaultCulture()->GetName()
			: ResolveDefaultCulture();
		if (FInternationalization::Get().SetCurrentCulture(CultureToApply))
		{
			// Clear this string
			GConfig->RemoveKey(TEXT("Internationalization"), TEXT("Culture"), GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		bResetToDefaultCulture = false;
	}
	else if (!PendingCulture.IsEmpty())
	{
		// SetCurrentCulture may trigger PendingCulture to be cleared (if a culture change is broadcast) so we take a copy of it to work with
		const FString CultureToApply = PendingCulture;
		if (FInternationalization::Get().SetCurrentCulture(CultureToApply))
		{
			// Note: This is intentionally saved to the users config
			// We need to localize text before the player logs in and very early in the loading screen
			GConfig->SetString(TEXT("Internationalization"), TEXT("Culture"), *CultureToApply, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
		ClearPendingCulture();
	}
}

void ULyraSettingsShared::ResetCultureToCurrentSettings()
{
	ClearPendingCulture();
	bResetToDefaultCulture = false;
}

const FString& ULyraSettingsShared::GetPendingCulture() const
{
	return PendingCulture;
}

void ULyraSettingsShared::SetPendingCulture(const FString& NewCulture)
{
	PendingCulture = NewCulture;
	bResetToDefaultCulture = false;
	bIsDirty = true;
}

void ULyraSettingsShared::OnCultureChanged()
{
	ClearPendingCulture();
	bResetToDefaultCulture = false;
}

void ULyraSettingsShared::ClearPendingCulture()
{
	PendingCulture.Reset();
}

bool ULyraSettingsShared::IsUsingDefaultCulture() const
{
	FString Culture;
	GConfig->GetString(TEXT("Internationalization"), TEXT("Culture"), Culture, GGameUserSettingsIni);

	return Culture.IsEmpty();
}

void ULyraSettingsShared::ResetToDefaultCulture()
{
	ClearPendingCulture();
	bResetToDefaultCulture = true;
	bIsDirty = true;
}

//////////////////////////////////////////////////////////////////////

void ULyraSettingsShared::ApplyInputSensitivity()
{
	// IMC_Default 上的 SettingBasedScalar、SettingsDrivenDeadZone、GamepadSensitivity 与
	// AimInversion modifier 每帧读取本对象，因此设置变更立即生效，不需要重建或重加 IMC。
}

