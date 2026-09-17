// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootExpeditionLobbyScreen.h"

#include "CommonButtonBase.h"
#include "GameModes/ShootExpeditionLobbyComponent.h"
#include "GameState/ShootGameStateBase.h"
#include "Messaging/CommonGameDialog.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootExpeditionLobbyScreen)

void UShootExpeditionLobbyScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	Coordinator = GetGameInstance() ? GetGameInstance()->GetSubsystem<UShootSessionCoordinatorSubsystem>() : nullptr;
	if (Coordinator)
	{
		Coordinator->RefreshPlatformSessionState();
		Coordinator->OnCoordinatorChanged.AddUObject(this, &ThisClass::RefreshCoordinatorState);
	}

	BindLobbyComponent();
	RefreshCoordinatorState();
	if (UWorld* World = GetWorld())
	{
		// PlayerArray 没有统一的客户端成员变化事件，只在页面激活时低频刷新，关闭后立即清理。
		World->GetTimerManager().SetTimer(
			LobbyRefreshTimerHandle, this, &ThisClass::RefreshLobbyData, 1.0f, true);
	}
}

void UShootExpeditionLobbyScreen::NativeOnDeactivated()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LobbyRefreshTimerHandle);
	}
	if (LobbyComponent)
	{
		LobbyComponent->OnLobbyChanged.RemoveDynamic(this, &ThisClass::RefreshLobbyData);
	}
	if (Coordinator)
	{
		Coordinator->OnCoordinatorChanged.RemoveAll(this);
	}
	LobbyComponent = nullptr;
	Coordinator = nullptr;
	Super::NativeOnDeactivated();
}

void UShootExpeditionLobbyScreen::BindLobbyComponent()
{
	if (LobbyComponent)
	{
		LobbyComponent->OnLobbyChanged.RemoveDynamic(this, &ThisClass::RefreshLobbyData);
	}

	AShootGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState<AShootGameStateBase>() : nullptr;
	LobbyComponent = GameState ? GameState->GetExpeditionLobbyComponent() : nullptr;
	if (LobbyComponent)
	{
		LobbyComponent->OnLobbyChanged.AddDynamic(this, &ThisClass::RefreshLobbyData);
		RefreshLobbyData();
	}
}

void UShootExpeditionLobbyScreen::RefreshLobbyData()
{
	if (!LobbyComponent)
	{
		BindLobbyComponent();
		if (!LobbyComponent)
		{
			BP_OnLobbyDataChanged({}, {}, true, false, {}, false);
			return;
		}
	}

	const TArray<FShootSessionPlayerInfo> Players = Coordinator
		? Coordinator->GetRuntimePlayerInfo()
		: TArray<FShootSessionPlayerInfo>();
	const bool bCanStart = CanLocalPlayerStartExpedition();
	const bool bCanInvite = CanInviteFriends();
	if (StartButton)
	{
		StartButton->SetIsEnabled(bCanStart);
	}
	if (InviteButton)
	{
		InviteButton->SetIsEnabled(bCanInvite);
	}
	BP_OnLobbyDataChanged(
		LobbyComponent->GetSelectedMapId(), LobbyComponent->GetSelectedExperienceId(),
		LobbyComponent->IsJoinInProgressAllowed(), false, Players, bCanStart);
}

void UShootExpeditionLobbyScreen::RefreshCoordinatorState()
{
	if (Coordinator)
	{
		BP_OnLobbySessionStateChanged(Coordinator->GetState(), Coordinator->GetStatusText());
	}
	RefreshLobbyData();
}

bool UShootExpeditionLobbyScreen::CanLocalPlayerStartExpedition() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	return LobbyComponent && LobbyComponent->IsWaitingLobby() && PlayerController &&
		PlayerController->IsLocalController() && PlayerController->HasAuthority();
}

bool UShootExpeditionLobbyScreen::StartExpedition()
{
	return LobbyComponent && LobbyComponent->StartSelectedExpedition(GetOwningPlayer());
}

bool UShootExpeditionLobbyScreen::CanInviteFriends() const
{
	return Coordinator && Coordinator->GetRole() == EShootSessionRole::Host &&
		Coordinator->GetOnlineMode() == ECommonSessionOnlineMode::Online &&
		Coordinator->GetRuntimeSessionInfo().bIsInSession;
}

bool UShootExpeditionLobbyScreen::InviteFriends()
{
	return CanInviteFriends() && Coordinator->ShowInviteUI(GetOwningPlayer());
}

void UShootExpeditionLobbyScreen::ConfirmLeaveLobby()
{
	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	UCommonMessagingSubsystem* Messaging = LocalPlayer
		? LocalPlayer->GetSubsystem<UCommonMessagingSubsystem>()
		: nullptr;
	if (!Messaging || !Coordinator)
	{
		return;
	}

	const FText Message = Coordinator->GetRole() == EShootSessionRole::Host
		? NSLOCTEXT("ShootExpeditionLobby", "HostLeaveLobbyMessage",
			"You are the host. Leaving the expedition lobby will destroy the session and return all members to the Home Base. Continue?")
		: NSLOCTEXT("ShootExpeditionLobby", "ClientLeaveLobbyMessage",
			"Leave the current expedition lobby and return to the Home Base? The host and other members can continue waiting.");
	Messaging->ShowConfirmation(
		UCommonGameDialogDescriptor::CreateConfirmationYesNo(
			NSLOCTEXT("ShootExpeditionLobby", "LeaveLobbyTitle", "Leave Expedition Lobby"), Message),
		FCommonMessagingResultDelegate::CreateUObject(
			this, &ThisClass::HandleLeaveLobbyConfirmation));
}

void UShootExpeditionLobbyScreen::HandleLeaveLobbyConfirmation(ECommonMessagingResult Result)
{
	if (Result == ECommonMessagingResult::Confirmed && Coordinator)
	{
		Coordinator->LeaveOrDestroySession(GetOwningPlayer());
	}
}
