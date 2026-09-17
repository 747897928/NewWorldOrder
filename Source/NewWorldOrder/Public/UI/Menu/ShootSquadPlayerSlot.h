// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "UI/Menu/ShootExpeditionLobbyTypes.h"

#include "ShootSquadPlayerSlot.generated.h"

/** 可复用的四人小队成员槽；蓝图完全拥有槽位布局和状态材质。 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootSquadPlayerSlot : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void InitializeOccupiedSlot(const FShootExpeditionLobbyPlayerInfo& InPlayerInfo);
	void InitializeEmptySlot(int32 InDisplayIndex);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="Expedition|Lobby", meta=(DisplayName="On Player Slot Changed"))
	void BP_OnPlayerSlotChanged(bool bOccupied, const FShootExpeditionLobbyPlayerInfo& PlayerInfo);
};
