// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModes/ShootGameMode.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "Player/ShootPlayerState.h"
#include "System/SaveGameSubsystem.h"
#include "System/ShootGameInstance.h"

namespace
{
	int32 ResolveLocalPlayerIndex(const AShootPlayerState* PlayerState, const UGameInstance* GameInstance)
	{
		const APlayerController* PlayerController =
			PlayerState ? Cast<APlayerController>(PlayerState->GetOwner()) : nullptr;
		const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
		return LocalPlayer && GameInstance
			? GameInstance->GetLocalPlayers().IndexOfByKey(LocalPlayer)
			: INDEX_NONE;
	}
}

void AShootGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	if (GetLocalPlayerMapPolicy() == EShootLocalPlayerMapPolicy::SplitProtagonists && NewPlayer)
	{
		AShootPlayerState* ShootPlayerState = NewPlayer->GetPlayerState<AShootPlayerState>();
		const int32 LocalPlayerIndex = ResolveLocalPlayerIndex(ShootPlayerState, GetGameInstance());
		if (ShootPlayerState && LocalPlayerIndex != INDEX_NONE)
		{
			// PlayerState::BeginPlay 可能发生在本地 PlayerController 完成 ULocalPlayer 归属之前，
			// 第一次恢复无法区分 Player01/02。此处已是 UE 生成 Pawn 前最后一步，重新执行幂等恢复：
			// Persistent Inventory/Resource 会先清旧再应用，外观与性别能力套件也按最终 MM/MF 规则收敛。
			if (USaveGameSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<USaveGameSubsystem>())
			{
				SaveSubsystem->RestorePlayerInventoryState(ShootPlayerState);
			}
		}
	}

	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

ECharacterGender AShootGameMode::ResolveInitialCharacterGender_Implementation(
	const AShootPlayerState* PlayerState, ECharacterGender SavedGender) const
{
	if (GetLocalPlayerMapPolicy() != EShootLocalPlayerMapPolicy::SplitProtagonists)
	{
		return Super::ResolveInitialCharacterGender_Implementation(PlayerState, SavedGender);
	}

	const int32 LocalPlayerIndex = ResolveLocalPlayerIndex(PlayerState, GetGameInstance());
	if (LocalPlayerIndex == INDEX_NONE)
	{
		// Listen Server 的远端 Controller 没有 ULocalPlayer，必须保留远端账号自己的选择。
		return SavedGender;
	}

	if (const UShootGameInstance* ShootGameInstance = GetGameInstance<UShootGameInstance>())
	{
		ECharacterGender SelectedGender = ECharacterGender::UNKNOWN;
		if (ShootGameInstance->GetPendingLocalCoopProtagonist(LocalPlayerIndex, SelectedGender))
		{
			return SelectedGender;
		}
	}

	// 直接打开测试图或自动化没有经过选择页时，才使用 GameMode 的可配置默认值。
	// UNKNOWN 不适合作为可玩的主角，资产误配时只在这里回退为男主，不污染 PlayerState。
	const ECharacterGender Player01Gender =
		SplitPlayer01DefaultGender == ECharacterGender::FEMALE
			? ECharacterGender::FEMALE
			: ECharacterGender::MALE;
	return LocalPlayerIndex == 0
		? Player01Gender
		: (Player01Gender == ECharacterGender::MALE
			? ECharacterGender::FEMALE
			: ECharacterGender::MALE);
}

bool AShootGameMode::ShouldPersistLastActiveGender_Implementation(
	const AShootPlayerState* PlayerState) const
{
	if (GetLocalPlayerMapPolicy() != EShootLocalPlayerMapPolicy::SplitProtagonists)
	{
		return Super::ShouldPersistLastActiveGender_Implementation(PlayerState);
	}

	const int32 LocalPlayerIndex = ResolveLocalPlayerIndex(PlayerState, GetGameInstance());
	// 远端玩家使用各自进程/存档。本机 Player01/02 的互斥选择属于双人配置，
	// 二者都不能覆盖单人模式用于“最后切换主角”的 LastActiveGender。
	return LocalPlayerIndex == INDEX_NONE;
}
