// Copyright NewWorldOrder Game. All Rights Reserved.

#include "UI/Menu/ShootSessionPlayerEntry.h"

void UShootSessionPlayerEntry::InitializePlayerData(int32 DisplayIndex, const FString& PlayerName,
	float PingMilliseconds)
{
	BP_OnPlayerDataChanged(DisplayIndex, PlayerName, PingMilliseconds);
}
