// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootExpeditionLobbyScreen.h"

#include "CommonButtonBase.h"
#include "Components/DynamicEntryBox.h"
#include "GameModes/ShootExpeditionLobbyComponent.h"
#include "GameState/ShootGameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Messaging/CommonGameDialog.h"
#include "Messaging/CommonMessagingSubsystem.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "Player/ShootPlayerController.h"
#include "TimerManager.h"
#include "UI/Menu/ShootSquadPlayerSlot.h"

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

	CachedPlayers.Reset();
	const APlayerState* LocalPlayerState = GetOwningPlayerState();
	const APlayerState* HostPlayerState = LobbyComponent->GetHostPlayerState();
	if (const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr)
	{
		int32 DisplayIndex = 1;
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			if (!PlayerState)
			{
				continue;
			}
			FShootExpeditionLobbyPlayerInfo& PlayerInfo = CachedPlayers.AddDefaulted_GetRef();
			PlayerInfo.DisplayIndex = DisplayIndex++;
			PlayerInfo.PlayerName = PlayerState->GetPlayerName();
			PlayerInfo.PingMilliseconds = PlayerState->GetPingInMilliseconds();
			PlayerInfo.bIsHost = PlayerState == HostPlayerState;
			PlayerInfo.bIsLocalPlayer = PlayerState == LocalPlayerState;
			PlayerInfo.bIsReady = LobbyComponent->IsPlayerReady(PlayerState);
		}
	}
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
	if (ReadyButton)
	{
		ReadyButton->SetIsEnabled(LobbyComponent->IsWaitingLobby());
		if (IsLocalPlayerReady())
		{
			ReadyButton->SetIsSelected(true);
		}
		else
		{
			ReadyButton->ClearSelection();
		}
	}
	PopulatePlayerSlots(PlayerEntryBox, PlayerSlotClass);
	BP_OnLobbyDataChanged(
		LobbyComponent->GetSelectedMapId(), LobbyComponent->GetSelectedExperienceId(),
		LobbyComponent->IsJoinInProgressAllowed(), false, CachedPlayers, bCanStart);
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
		PlayerController->IsLocalController() && PlayerController->HasAuthority() &&
		LobbyComponent->AreAllPlayersReady();
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

void UShootExpeditionLobbyScreen::ToggleLocalPlayerReady()
{
	if (AShootPlayerController* PlayerController = Cast<AShootPlayerController>(GetOwningPlayer()))
	{
		// 只有项目 PlayerController 提供拥有连接的 Server RPC，因此这里的子类 Cast 是必要的。
		PlayerController->SetExpeditionLobbyReady(!IsLocalPlayerReady());
	}
}

bool UShootExpeditionLobbyScreen::IsLocalPlayerReady() const
{
	return LobbyComponent && LobbyComponent->IsPlayerReady(GetOwningPlayerState());
}

void UShootExpeditionLobbyScreen::PopulatePlayerSlots(
	UDynamicEntryBox* EntryBox,
	TSubclassOf<UShootSquadPlayerSlot> SlotWidgetClass)
{
	if (EntryBox)
	{
		PlayerEntryBox = EntryBox;
	}
	if (SlotWidgetClass)
	{
		PlayerSlotClass = SlotWidgetClass;
	}
	if (!PlayerEntryBox)
	{
		return;
	}
	if (!PlayerSlotClass)
	{
		UClass* ConfiguredClass = PlayerEntryBox->GetEntryWidgetClass();
		if (ConfiguredClass && ConfiguredClass->IsChildOf(UShootSquadPlayerSlot::StaticClass()))
		{
			PlayerSlotClass = ConfiguredClass;
		}
	}
	if (!PlayerSlotClass)
	{
		return;
	}

	PlayerEntryBox->Reset(false);
	constexpr int32 MaxVisibleSlots = 4;
	for (int32 Index = 0; Index < MaxVisibleSlots; ++Index)
	{
		if (UShootSquadPlayerSlot* PlayerSlotWidget =
			PlayerEntryBox->CreateEntry<UShootSquadPlayerSlot>(PlayerSlotClass))
		{
			if (CachedPlayers.IsValidIndex(Index))
			{
				PlayerSlotWidget->InitializeOccupiedSlot(CachedPlayers[Index]);
			}
			else
			{
				PlayerSlotWidget->InitializeEmptySlot(Index + 1);
			}
		}
	}
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
			"You are the host. Leaving the squad will return all members to the Home Base. Continue?")
		: NSLOCTEXT("ShootExpeditionLobby", "ClientLeaveLobbyMessage",
			"Leave the current squad and return to the Home Base? The other members can continue waiting.");
	Messaging->ShowConfirmation(
		UCommonGameDialogDescriptor::CreateConfirmationYesNo(
			NSLOCTEXT("ShootExpeditionLobby", "LeaveLobbyTitle", "Leave Squad"), Message),
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
