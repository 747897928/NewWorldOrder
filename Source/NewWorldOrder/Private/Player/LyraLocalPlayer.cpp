// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/LyraLocalPlayer.h"

#include "AudioMixerBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "CommonUserSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Settings/LyraSettingsLocal.h"
#include "Settings/LyraSettingsShared.h"
#include "UI/ViewModel/AttributeViewModel.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraLocalPlayer)

class UObject;

ULyraLocalPlayer::ULyraLocalPlayer()
{
}

void ULyraLocalPlayer::PostInitProperties()
{
	Super::PostInitProperties();

	if (ULyraSettingsLocal* LocalSettings = GetLocalSettings())
	{
		LocalSettings->OnAudioOutputDeviceChanged.AddUObject(this, &ThisClass::OnAudioOutputDeviceChanged);
	}
}

void ULyraLocalPlayer::SwitchController(class APlayerController* PC)
{
	Super::SwitchController(PC);

	OnPlayerControllerChanged(PlayerController);
}

bool ULyraLocalPlayer::SpawnPlayActor(const FString& URL, FString& OutError, UWorld* InWorld)
{
	const bool bResult = Super::SpawnPlayActor(URL, OutError, InWorld);

	OnPlayerControllerChanged(PlayerController);

	return bResult;
}

void ULyraLocalPlayer::InitOnlineSession()
{
	OnPlayerControllerChanged(PlayerController);

	Super::InitOnlineSession();
}

void ULyraLocalPlayer::OnPlayerControllerChanged(APlayerController* NewController)
{
	if (NewController && !UserMappableInputMappingContext.IsNull())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (UEnhancedInputUserSettings* InputSettings = InputSubsystem->GetUserSettings())
			{
				// Register 只建立设置目录，不会把 IMC 叠加到 FrontEnd；实际激活仍由角色/Widget 生命周期负责。
				if (const UInputMappingContext* MappingContext = UserMappableInputMappingContext.LoadSynchronous())
				{
					InputSettings->RegisterInputMappingContext(MappingContext);
				}
			}
		}
	}

	// Stop listening for changes from the old controller
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (ILyraTeamAgentInterface* ControllerAsTeamProvider = Cast<ILyraTeamAgentInterface>(LastBoundPC.Get()))
	{
		OldTeamID = ControllerAsTeamProvider->GetGenericTeamId();
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().RemoveAll(this);
	}

	// Grab the current team ID and listen for future changes
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (ILyraTeamAgentInterface* ControllerAsTeamProvider = Cast<ILyraTeamAgentInterface>(NewController))
	{
		NewTeamID = ControllerAsTeamProvider->GetGenericTeamId();
		ControllerAsTeamProvider->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnControllerChangedTeam);
		LastBoundPC = NewController;
	}

	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
}

void ULyraLocalPlayer::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// Do nothing, we merely observe the team of our associated player controller
}

FGenericTeamId ULyraLocalPlayer::GetGenericTeamId() const
{
	if (ILyraTeamAgentInterface* ControllerAsTeamProvider = Cast<ILyraTeamAgentInterface>(PlayerController))
	{
		return ControllerAsTeamProvider->GetGenericTeamId();
	}
	else
	{
		return FGenericTeamId::NoTeam;
	}
}

FOnLyraTeamIndexChangedDelegate* ULyraLocalPlayer::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

ULyraSettingsLocal* ULyraLocalPlayer::GetLocalSettings() const
{
	return ULyraSettingsLocal::Get();
}

ULyraSettingsShared* ULyraLocalPlayer::GetSharedSettings() const
{
	if (!SharedSettings)
	{
		// 桌面平台可在账号登录前直接检查本地磁盘；其他平台先使用临时对象，登录后再异步替换。
		if (PLATFORM_DESKTOP)
		{
			SharedSettings = ULyraSettingsShared::LoadOrCreateSettings(this);
		}
		else
		{
			SharedSettings = ULyraSettingsShared::CreateTemporarySettings(this);
		}
	}

	return SharedSettings;
}

void ULyraLocalPlayer::LoadSharedSettingsFromDisk(bool bForceLoad)
{
	FUniqueNetIdRepl CurrentNetId = GetCachedUniqueNetId();
	if (!bForceLoad && SharedSettings && CurrentNetId == NetIdForSharedSettings)
	{
		return;
	}

	ensure(ULyraSettingsShared::AsyncLoadOrCreateSettings(
		this, ULyraSettingsShared::FOnSettingsLoadedEvent::CreateUObject(this, &ThisClass::OnSharedSettingsLoaded)));
}

UAttributeViewModel* ULyraLocalPlayer::GetAttributeViewModel_Implementation()
{
	if (!AttributeViewModel)
	{
		AttributeViewModel = NewObject<UAttributeViewModel>(this);
	}
	return AttributeViewModel;
}

void ULyraLocalPlayer::OnSharedSettingsLoaded(ULyraSettingsShared* LoadedOrCreatedSettings)
{
	// The settings are applied before it gets here
	if (ensure(LoadedOrCreatedSettings))
	{
		SharedSettings = LoadedOrCreatedSettings;
		NetIdForSharedSettings = GetCachedUniqueNetId();
	}
}

void ULyraLocalPlayer::OnAudioOutputDeviceChanged(const FString& InAudioOutputDeviceId)
{
	FOnCompletedDeviceSwap DevicesSwappedCallback;
	DevicesSwappedCallback.BindUFunction(this, FName("OnCompletedAudioDeviceSwap"));
	UAudioMixerBlueprintLibrary::SwapAudioOutputDevice(GetWorld(), InAudioOutputDeviceId, DevicesSwappedCallback);
}

void ULyraLocalPlayer::OnCompletedAudioDeviceSwap(const FSwapAudioOutputResult& SwapResult)
{
	if (SwapResult.Result == ESwapAudioOutputDeviceResultState::Failure)
	{
	}
}

void ULyraLocalPlayer::OnControllerChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}
