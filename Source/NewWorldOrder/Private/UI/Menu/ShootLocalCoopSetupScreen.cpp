// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootLocalCoopSetupScreen.h"

#include "CommonUserSubsystem.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameModes/LyraUserFacingExperienceDefinition.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "System/ShootGameInstance.h"
#include "TimerManager.h"
#include "UI/CustomGameViewportClient.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootLocalCoopSetupScreen)

#define LOCTEXT_NAMESPACE "ShootLocalCoopSetupScreen"

void UShootLocalCoopSetupScreen::InitializeForExperience(
	ULyraUserFacingExperienceDefinition* Experience)
{
	SelectedExperience = Experience;
}

void UShootLocalCoopSetupScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	CommonUserSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UCommonUserSubsystem>()
		: nullptr;
	Coordinator = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UShootSessionCoordinatorSubsystem>()
		: nullptr;

	if (!SelectedExperience || !CommonUserSubsystem || !Coordinator || !SetupInputMapping ||
		!SelectLeftAction || !SelectRightAction || !ConfirmAction || !CancelAction)
	{
		BP_OnSetupError(LOCTEXT("SetupUnavailable", "Local co-op setup is unavailable."));
		return;
	}

	CommonUserSubsystem->SetMaxLocalPlayers(2);
	CommonUserSubsystem->OnUserInitializeComplete.AddDynamic(
		this, &ThisClass::HandleUserInitializeComplete);

	if (UCustomGameViewportClient* ViewportClient = Cast<UCustomGameViewportClient>(
		GetGameInstance()->GetGameViewportClient()))
	{
		ViewportClient->OnViewportInputKey().AddUObject(this, &ThisClass::HandleViewportInputKey);
		ViewportClient->OnInputDeviceConnectionChanged().AddUObject(
			this, &ThisClass::HandleInputDeviceConnectionChanged);
	}

	SetupStage = EShootLocalCoopSetupStage::DeviceJoin;
	RefreshInitialPlayers();
	ApplyInputMappingToLocalPlayers(true);
	BeginListeningForSecondPlayer();
	BroadcastSetupState();
}

void UShootLocalCoopSetupScreen::NativeOnDeactivated()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LaunchTimerHandle);
	}
	StopListeningForSecondPlayer();
	ApplyInputMappingToLocalPlayers(false);

	if (CommonUserSubsystem)
	{
		CommonUserSubsystem->OnUserInitializeComplete.RemoveDynamic(
			this, &ThisClass::HandleUserInitializeComplete);
	}
	if (UCustomGameViewportClient* ViewportClient = GetGameInstance()
		? Cast<UCustomGameViewportClient>(GetGameInstance()->GetGameViewportClient())
		: nullptr)
	{
		ViewportClient->OnViewportInputKey().RemoveAll(this);
		ViewportClient->OnInputDeviceConnectionChanged().RemoveAll(this);
	}

	if (!bTravelStarted)
	{
		CleanupSecondLocalPlayer();
		if (UShootGameInstance* ShootGameInstance = GetGameInstance<UShootGameInstance>())
		{
			ShootGameInstance->ClearPendingLocalCoopProtagonists();
		}
	}

	CommonUserSubsystem = nullptr;
	Coordinator = nullptr;
	Super::NativeOnDeactivated();
}

bool UShootLocalCoopSetupScreen::NativeOnHandleBackAction()
{
	if (SetupStage == EShootLocalCoopSetupStage::CharacterAssignment)
	{
		for (FShootLocalCoopPlayerSetupState& State : PlayerStates)
		{
			State.bConfirmed = false;
		}
		SetupStage = EShootLocalCoopSetupStage::DeviceJoin;
		BeginListeningForSecondPlayer();
		BroadcastSetupState();
		return true;
	}

	CancelSetup();
	return true;
}

void UShootLocalCoopSetupScreen::RefreshInitialPlayers()
{
	for (int32 Index = 0; Index < 2; ++Index)
	{
		PlayerStates[Index] = {};
		PlayerStates[Index].LocalPlayerIndex = Index;
		PlayerStates[Index].SelectedProtagonist = Index == 0
			? ECharacterGender::MALE
			: ECharacterGender::FEMALE;

		const UCommonUserInfo* UserInfo = CommonUserSubsystem
			? CommonUserSubsystem->GetUserInfoForLocalPlayerIndex(Index)
			: nullptr;
		if (UserInfo && UserInfo->IsLoggedIn())
		{
			PlayerStates[Index].bJoined = true;
			PlayerStates[Index].InputDeviceId = UserInfo->PrimaryInputDevice.GetId();
		}
	}
}

void UShootLocalCoopSetupScreen::BeginListeningForSecondPlayer()
{
	if (!CommonUserSubsystem || PlayerStates[1].bJoined || !SetupInputMapping || !ConfirmAction)
	{
		return;
	}

	TArray<FKey> JoinKeys;
	SetupInputMapping->ForEachKeyMapping([this, &JoinKeys](const FEnhancedActionKeyMapping& Mapping)
	{
		if (Mapping.Action == ConfirmAction && Mapping.Key.IsValid())
		{
			JoinKeys.AddUnique(Mapping.Key);
		}
	});

	FCommonUserInitializeParams Params;
	Params.LocalPlayerIndex = 1;
	Params.bCanCreateNewLocalPlayer = true;
	Params.bCanUseGuestLogin = true;
	Params.RequestedPrivilege = ECommonUserPrivilege::CanPlay;
	CommonUserSubsystem->ListenForLoginKeyInput({}, JoinKeys, Params);
}

void UShootLocalCoopSetupScreen::StopListeningForSecondPlayer()
{
	if (CommonUserSubsystem)
	{
		CommonUserSubsystem->ListenForLoginKeyInput({}, {}, {});
	}
}

void UShootLocalCoopSetupScreen::HandleUserInitializeComplete(
	const UCommonUserInfo* UserInfo, bool bSuccess, FText Error,
	ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext)
{
	if (RequestedPrivilege != ECommonUserPrivilege::CanPlay || !UserInfo ||
		UserInfo->LocalPlayerIndex != 1)
	{
		return;
	}

	if (!bSuccess)
	{
		BP_OnSetupError(Error.IsEmpty()
			? LOCTEXT("SecondPlayerJoinFailed", "The second player could not join.")
			: Error);
		BeginListeningForSecondPlayer();
		return;
	}

	bCreatedSecondLocalPlayer = true;
	PlayerStates[1].bJoined = true;
	PlayerStates[1].InputDeviceId = UserInfo->PrimaryInputDevice.GetId();
	StopListeningForSecondPlayer();
	ApplyInputMappingToLocalPlayers(true);
	BroadcastSetupState();
}

bool UShootLocalCoopSetupScreen::CanContinueToCharacterAssignment() const
{
	return SetupStage == EShootLocalCoopSetupStage::DeviceJoin &&
		PlayerStates[0].bJoined && PlayerStates[1].bJoined;
}

bool UShootLocalCoopSetupScreen::ContinueToCharacterAssignment()
{
	if (!CanContinueToCharacterAssignment())
	{
		return false;
	}

	StopListeningForSecondPlayer();
	SetupStage = EShootLocalCoopSetupStage::CharacterAssignment;
	for (FShootLocalCoopPlayerSetupState& State : PlayerStates)
	{
		State.bConfirmed = false;
	}
	BroadcastSetupState();
	return true;
}

void UShootLocalCoopSetupScreen::CancelSetup()
{
	DeactivateWidget();
}

void UShootLocalCoopSetupScreen::ApplyInputMappingToLocalPlayers(bool bAdd)
{
	if (!SetupInputMapping || !GetGameInstance())
	{
		return;
	}

	for (ULocalPlayer* LocalPlayer : GetGameInstance()->GetLocalPlayers())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
		{
			if (bAdd)
			{
				InputSubsystem->AddMappingContext(SetupInputMapping, 100);
			}
			else
			{
				InputSubsystem->RemoveMappingContext(SetupInputMapping);
			}
		}
	}
}

void UShootLocalCoopSetupScreen::HandleViewportInputKey(const FInputKeyEventArgs& EventArgs)
{
	if (SetupStage != EShootLocalCoopSetupStage::CharacterAssignment || EventArgs.Event != IE_Pressed)
	{
		return;
	}

	const int32 LocalPlayerIndex = ResolveLocalPlayerIndex(EventArgs.InputDevice);
	if (!FMath::IsWithinInclusive(LocalPlayerIndex, 0, 1))
	{
		return;
	}

	const UInputAction* ResolvedAction = nullptr;
	for (const UInputAction* Action : {SelectLeftAction.Get(), SelectRightAction.Get(), ConfirmAction.Get(), CancelAction.Get()})
	{
		if (DoesKeyTriggerAction(EventArgs.Key, Action))
		{
			ResolvedAction = Action;
			break;
		}
	}
	if (ResolvedAction)
	{
		HandleCharacterAction(LocalPlayerIndex, ResolvedAction);
	}
}

void UShootLocalCoopSetupScreen::HandleCharacterAction(
	int32 LocalPlayerIndex, const UInputAction* Action)
{
	FShootLocalCoopPlayerSetupState& State = PlayerStates[LocalPlayerIndex];
	if (Action == CancelAction)
	{
		if (State.bConfirmed)
		{
			State.bConfirmed = false;
			BroadcastSetupState();
		}
		else
		{
			NativeOnHandleBackAction();
		}
		return;
	}

	if (State.bConfirmed)
	{
		return;
	}

	if (Action == SelectLeftAction)
	{
		State.SelectedProtagonist = ECharacterGender::MALE;
	}
	else if (Action == SelectRightAction)
	{
		State.SelectedProtagonist = ECharacterGender::FEMALE;
	}
	else if (Action == ConfirmAction)
	{
		const FShootLocalCoopPlayerSetupState& OtherState = PlayerStates[1 - LocalPlayerIndex];
		if (OtherState.bConfirmed && OtherState.SelectedProtagonist == State.SelectedProtagonist)
		{
			BP_OnCharacterSelectionConflict(LocalPlayerIndex, State.SelectedProtagonist);
			return;
		}
		State.bConfirmed = true;
	}

	BroadcastSetupState();
	TryLaunchWhenReady();
}

bool UShootLocalCoopSetupScreen::DoesKeyTriggerAction(FKey Key, const UInputAction* Action) const
{
	if (!SetupInputMapping || !Action)
	{
		return false;
	}

	bool bMatches = false;
	SetupInputMapping->ForEachKeyMapping([&bMatches, Key, Action](const FEnhancedActionKeyMapping& Mapping)
	{
		bMatches |= Mapping.Action == Action && Mapping.Key == Key;
	});
	return bMatches;
}

int32 UShootLocalCoopSetupScreen::ResolveLocalPlayerIndex(FInputDeviceId InputDeviceId) const
{
	const UCommonUserInfo* UserInfo = CommonUserSubsystem
		? CommonUserSubsystem->GetUserInfoForInputDevice(InputDeviceId)
		: nullptr;
	return UserInfo ? UserInfo->LocalPlayerIndex : INDEX_NONE;
}

void UShootLocalCoopSetupScreen::HandleInputDeviceConnectionChanged(
	EInputDeviceConnectionState ConnectionState, FPlatformUserId, FInputDeviceId InputDeviceId)
{
	if (ConnectionState != EInputDeviceConnectionState::Disconnected)
	{
		return;
	}

	const int32 LocalPlayerIndex = ResolveLocalPlayerIndex(InputDeviceId);
	if (LocalPlayerIndex == 1)
	{
		CleanupSecondLocalPlayer();
		PlayerStates[1].bJoined = false;
		PlayerStates[1].bConfirmed = false;
		PlayerStates[1].InputDeviceId = INDEX_NONE;
		SetupStage = EShootLocalCoopSetupStage::DeviceJoin;
		BeginListeningForSecondPlayer();
		BroadcastSetupState();
	}
}

void UShootLocalCoopSetupScreen::TryLaunchWhenReady()
{
	if (!PlayerStates[0].bConfirmed || !PlayerStates[1].bConfirmed)
	{
		return;
	}
	if (PlayerStates[0].SelectedProtagonist == PlayerStates[1].SelectedProtagonist)
	{
		return;
	}

	SetupStage = EShootLocalCoopSetupStage::Launching;
	BroadcastSetupState();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			LaunchTimerHandle, this, &ThisClass::LaunchSelectedExperience, 0.75f, false);
	}
}

void UShootLocalCoopSetupScreen::LaunchSelectedExperience()
{
	if (!SelectedExperience || !Coordinator)
	{
		BP_OnSetupError(LOCTEXT("LaunchUnavailable", "The selected experience could not be started."));
		SetupStage = EShootLocalCoopSetupStage::CharacterAssignment;
		BroadcastSetupState();
		return;
	}

	if (UShootGameInstance* ShootGameInstance = GetGameInstance<UShootGameInstance>())
	{
		ShootGameInstance->SetPendingLocalCoopProtagonists(
			PlayerStates[0].SelectedProtagonist, PlayerStates[1].SelectedProtagonist);
	}

	UCommonSession_HostSessionRequest* Request = SelectedExperience->CreateHostingRequestWithOptions(
		this, ECommonSessionOnlineMode::Offline, 2, 2, false, false);
	APlayerController* PlayerController = GetOwningPlayer();
	if (!Request || !PlayerController || !Coordinator->HostSessionRequest(PlayerController, Request))
	{
		if (UShootGameInstance* ShootGameInstance = GetGameInstance<UShootGameInstance>())
		{
			ShootGameInstance->ClearPendingLocalCoopProtagonists();
		}
		BP_OnSetupError(LOCTEXT("LaunchFailed", "The local co-op session could not be started."));
		SetupStage = EShootLocalCoopSetupStage::CharacterAssignment;
		for (FShootLocalCoopPlayerSetupState& State : PlayerStates)
		{
			State.bConfirmed = false;
		}
		BroadcastSetupState();
		return;
	}

	bTravelStarted = true;
}

void UShootLocalCoopSetupScreen::CleanupSecondLocalPlayer()
{
	if (bCreatedSecondLocalPlayer && CommonUserSubsystem)
	{
		CommonUserSubsystem->TryToLogOutUser(1, true);
		bCreatedSecondLocalPlayer = false;
	}
}

void UShootLocalCoopSetupScreen::BroadcastSetupState()
{
	BP_OnSetupChanged(SetupStage, PlayerStates[0], PlayerStates[1],
		CanContinueToCharacterAssignment());
}

#undef LOCTEXT_NAMESPACE
