// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "ShootExpeditionLobbyTypes.generated.h"

/** 大厅成员的只读 UI 快照；权威 Ready 状态仍保存在 GameState 的 LobbyComponent。 */
USTRUCT(BlueprintType)
struct FShootExpeditionLobbyPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Expedition|Lobby")
	int32 DisplayIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category="Expedition|Lobby")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category="Expedition|Lobby")
	float PingMilliseconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category="Expedition|Lobby")
	bool bIsHost = false;

	UPROPERTY(BlueprintReadOnly, Category="Expedition|Lobby")
	bool bIsLocalPlayer = false;

	UPROPERTY(BlueprintReadOnly, Category="Expedition|Lobby")
	bool bIsReady = false;
};
