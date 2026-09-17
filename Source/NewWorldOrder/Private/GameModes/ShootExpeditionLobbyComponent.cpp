// Copyright NewWorldOrder Game. All Rights Reserved.

#include "GameModes/ShootExpeditionLobbyComponent.h"

#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootExpeditionLobbyComponent)

DEFINE_LOG_CATEGORY_STATIC(LogShootExpeditionLobby, Log, All);

UShootExpeditionLobbyComponent::UShootExpeditionLobbyComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UShootExpeditionLobbyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, SelectedMapId);
	DOREPLIFETIME(ThisClass, SelectedExperienceId);
	DOREPLIFETIME(ThisClass, bWaitingLobby);
	DOREPLIFETIME(ThisClass, bAllowJoinInProgress);
	DOREPLIFETIME(ThisClass, bFillEmptySlotsWithBots);
	DOREPLIFETIME(ThisClass, HostPlayerState);
	DOREPLIFETIME(ThisClass, ReadyPlayers);
}

void UShootExpeditionLobbyComponent::ConfigureLobby(const FPrimaryAssetId& InMapId,
	const FPrimaryAssetId& InExperienceId, bool bInAllowJoinInProgress,
	bool bInFillEmptySlotsWithBots)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	SelectedMapId = InMapId;
	SelectedExperienceId = InExperienceId;
	bWaitingLobby = SelectedMapId.IsValid() && SelectedExperienceId.IsValid();
	bAllowJoinInProgress = bInAllowJoinInProgress;
	bFillEmptySlotsWithBots = bInFillEmptySlotsWithBots;
	HostPlayerState = nullptr;
	ReadyPlayers.Reset();
	OnLobbyChanged.Broadcast();
}

void UShootExpeditionLobbyComponent::RegisterLobbyPlayer(APlayerState* PlayerState)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bWaitingLobby || !PlayerState)
	{
		return;
	}

	if (!HostPlayerState)
	{
		HostPlayerState = PlayerState;
	}
	ReadyPlayers.Remove(PlayerState);
	OnLobbyChanged.Broadcast();
}

void UShootExpeditionLobbyComponent::UnregisterLobbyPlayer(APlayerState* PlayerState)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !PlayerState)
	{
		return;
	}

	ReadyPlayers.Remove(PlayerState);
	OnLobbyChanged.Broadcast();
}

void UShootExpeditionLobbyComponent::SetPlayerReady(APlayerState* PlayerState, bool bReady)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bWaitingLobby || !PlayerState)
	{
		return;
	}

	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState || !GameState->PlayerArray.Contains(PlayerState))
	{
		return;
	}

	if (bReady)
	{
		ReadyPlayers.AddUnique(PlayerState);
	}
	else
	{
		ReadyPlayers.Remove(PlayerState);
	}
	OnLobbyChanged.Broadcast();
}

bool UShootExpeditionLobbyComponent::IsPlayerReady(const APlayerState* PlayerState) const
{
	return PlayerState && ReadyPlayers.Contains(PlayerState);
}

bool UShootExpeditionLobbyComponent::AreAllPlayersReady() const
{
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!bWaitingLobby || !GameState || GameState->PlayerArray.IsEmpty())
	{
		return false;
	}

	for (const APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (PlayerState && !ReadyPlayers.Contains(PlayerState))
		{
			return false;
		}
	}
	return true;
}

bool UShootExpeditionLobbyComponent::StartSelectedExpedition(APlayerController* RequestingPlayer)
{
	UWorld* World = GetWorld();
	if (!World || !GetOwner() || !GetOwner()->HasAuthority() || !RequestingPlayer ||
		!RequestingPlayer->IsLocalController() || !bWaitingLobby || !AreAllPlayersReady())
	{
		return false;
	}

	FAssetData MapAssetData;
	if (!UAssetManager::Get().GetPrimaryAssetData(SelectedMapId, MapAssetData))
	{
		UE_LOG(LogShootExpeditionLobby, Error, TEXT("无法解析等待大厅选择的地图 %s"),
			*SelectedMapId.ToString());
		return false;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UShootSessionCoordinatorSubsystem* Coordinator =
			GameInstance->GetSubsystem<UShootSessionCoordinatorSubsystem>())
		{
			Coordinator->SetJoinInProgressAllowed(bAllowJoinInProgress);
		}
	}

	// 当前在线产品边界是一台机器一个 LocalPlayer；本地分屏只在 Standalone 入口创建。
	FString TravelURL = MapAssetData.PackageName.ToString();
	TravelURL += FString::Printf(TEXT("?Experience=%s?LocalPlayers=1?AllowJoinInProgress=%d?FillBots=%d"),
		*SelectedExperienceId.PrimaryAssetName.ToString(),
		bAllowJoinInProgress ? 1 : 0,
		bFillEmptySlotsWithBots ? 1 : 0);

	UE_LOG(LogShootExpeditionLobby, Log, TEXT("房主开始副本：%s"), *TravelURL);
	bWaitingLobby = false;
	OnLobbyChanged.Broadcast();
	World->ServerTravel(TravelURL, false);
	return true;
}

void UShootExpeditionLobbyComponent::OnRep_LobbyState()
{
	OnLobbyChanged.Broadcast();
}
