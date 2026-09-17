// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootSquadPlayerSlot.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootSquadPlayerSlot)

void UShootSquadPlayerSlot::InitializeOccupiedSlot(const FShootExpeditionLobbyPlayerInfo& InPlayerInfo)
{
	BP_OnPlayerSlotChanged(true, InPlayerInfo);
}

void UShootSquadPlayerSlot::InitializeEmptySlot(int32 InDisplayIndex)
{
	FShootExpeditionLobbyPlayerInfo EmptyInfo;
	EmptyInfo.DisplayIndex = InDisplayIndex;
	BP_OnPlayerSlotChanged(false, EmptyInfo);
}
