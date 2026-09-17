// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Settings/LyraSettingScreen.h"

#include "Input/CommonUIInputTypes.h"
#include "GameSettingValue.h"
#include "Messaging/CommonGameDialog.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "Player/LyraLocalPlayer.h"
#include "Settings/LyraGameSettingRegistry.h"
#include "UI/Common/LyraTabListWidgetBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraSettingScreen)

class UGameSettingRegistry;

void ULyraSettingScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	BackHandle = RegisterUIActionBinding(FBindUIActionArgs(BackInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleBackAction)));
	ApplyHandle = RegisterUIActionBinding(FBindUIActionArgs(ApplyInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleApplyAction)));
	CancelChangesHandle = RegisterUIActionBinding(FBindUIActionArgs(CancelChangesInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleCancelChangesAction)));
	ResetToDefaultsHandle = RegisterUIActionBinding(FBindUIActionArgs(ResetToDefaultsInputActionData, true, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleResetToDefaultsAction)));
}

UGameSettingRegistry* ULyraSettingScreen::CreateRegistry()
{
	ULyraGameSettingRegistry* NewRegistry = NewObject<ULyraGameSettingRegistry>();

	if (ULyraLocalPlayer* LocalPlayer = CastChecked<ULyraLocalPlayer>(GetOwningLocalPlayer()))
	{
		NewRegistry->Initialize(LocalPlayer);
	}

	return NewRegistry;
}

void ULyraSettingScreen::HandleBackAction()
{
	if (AttemptToPopNavigation())
	{
		return;
	}

	ApplyChanges();

	DeactivateWidget();
}

void ULyraSettingScreen::HandleApplyAction()
{
	ApplyChanges();
}

void ULyraSettingScreen::HandleCancelChangesAction()
{
	CancelChanges();
}

void ULyraSettingScreen::HandleResetToDefaultsAction()
{
	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UCommonMessagingSubsystem* Messaging = LocalPlayer
		? LocalPlayer->GetSubsystem<UCommonMessagingSubsystem>()
		: nullptr;
	if (!Messaging)
	{
		return;
	}

	// 全局恢复会同时影响画面、音频和输入，必须先确认，避免误触导致玩家失去熟悉的设置。
	Messaging->ShowConfirmation(
		UCommonGameDialogDescriptor::CreateConfirmationYesNo(
			NSLOCTEXT("LyraSettings", "ResetAllSettingsTitle", "Reset All Settings"),
			NSLOCTEXT("LyraSettings", "ResetAllSettingsMessage",
				"Reset all resettable options in Gameplay, Video, Audio, Mouse & Keyboard, and Gamepad to their defaults. Select Apply Changes to save, or Cancel to undo. Continue?")),
		FCommonMessagingResultDelegate::CreateUObject(
			this, &ThisClass::HandleResetToDefaultsConfirmation));
}

void ULyraSettingScreen::HandleResetToDefaultsConfirmation(ECommonMessagingResult Result)
{
	if (Result == ECommonMessagingResult::Confirmed)
	{
		ResetAllSettingsToDefaults();
	}
}

void ULyraSettingScreen::ResetAllSettingsToDefaults()
{
	ULyraGameSettingRegistry* SettingRegistry = GetRegistry<ULyraGameSettingRegistry>();
	if (!SettingRegistry)
	{
		return;
	}

	// ResetToDefault 会沿用 GameSettings 的 ChangeTracker，因而不会绕过 Apply/Cancel 和存储链。
	for (UGameSettingValue* Setting : SettingRegistry->GetAllResettableSettings())
	{
		if (Setting)
		{
			Setting->ResetToDefault();
		}
	}
}

void ULyraSettingScreen::OnSettingsDirtyStateChanged_Implementation(bool bSettingsDirty)
{
	if (bSettingsDirty)
	{
		if (!GetActionBindings().Contains(ApplyHandle))
		{
			AddActionBinding(ApplyHandle);
		}
		if (!GetActionBindings().Contains(CancelChangesHandle))
		{
			AddActionBinding(CancelChangesHandle);
		}
	}
	else
	{
		RemoveActionBinding(ApplyHandle);
		RemoveActionBinding(CancelChangesHandle);
	}
}
