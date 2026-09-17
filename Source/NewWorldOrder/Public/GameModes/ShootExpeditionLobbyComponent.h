// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "Engine/AssetManagerTypes.h"

#include "ShootExpeditionLobbyComponent.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FShootExpeditionLobbyChanged);

/**
 * 等待大厅中的副本选择快照。
 *
 * HomeMap 只负责让玩家选择并创建房间；Online Host 先由 CommonSession 进入 LobbyMap，
 * GameMode 再把 URL 中的目标副本写入本组件并复制给所有客户端。房主点击开始后只执行一次
 * ServerTravel，现有连接随服务器进入副本，途中加入由平台 Session 的 bAllowJoinInProgress 控制。
 */
UCLASS(BlueprintType, meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootExpeditionLobbyComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UShootExpeditionLobbyComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ConfigureLobby(const FPrimaryAssetId& InMapId, const FPrimaryAssetId& InExperienceId,
		bool bInAllowJoinInProgress, bool bInFillEmptySlotsWithBots);

	UFUNCTION(BlueprintPure, Category="Expedition|Lobby")
	bool IsWaitingLobby() const { return bWaitingLobby; }

	UFUNCTION(BlueprintPure, Category="Expedition|Lobby")
	FPrimaryAssetId GetSelectedMapId() const { return SelectedMapId; }

	UFUNCTION(BlueprintPure, Category="Expedition|Lobby")
	FPrimaryAssetId GetSelectedExperienceId() const { return SelectedExperienceId; }

	UFUNCTION(BlueprintPure, Category="Expedition|Lobby")
	bool IsJoinInProgressAllowed() const { return bAllowJoinInProgress; }

	UFUNCTION(BlueprintPure, Category="Expedition|Lobby")
	bool ShouldFillEmptySlotsWithBots() const { return bFillEmptySlotsWithBots; }

	/** 仅 Listen Server 房主的本地 Lobby UI 调用；远端客户端调用会被权威检查拒绝。 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Lobby")
	bool StartSelectedExpedition(APlayerController* RequestingPlayer);

	UPROPERTY(BlueprintAssignable, Category="Expedition|Lobby")
	FShootExpeditionLobbyChanged OnLobbyChanged;

private:
	UPROPERTY(ReplicatedUsing=OnRep_LobbyState)
	FPrimaryAssetId SelectedMapId;

	UPROPERTY(ReplicatedUsing=OnRep_LobbyState)
	FPrimaryAssetId SelectedExperienceId;

	UPROPERTY(ReplicatedUsing=OnRep_LobbyState)
	bool bWaitingLobby = false;

	UPROPERTY(ReplicatedUsing=OnRep_LobbyState)
	bool bAllowJoinInProgress = true;

	UPROPERTY(ReplicatedUsing=OnRep_LobbyState)
	bool bFillEmptySlotsWithBots = false;

	UFUNCTION()
	void OnRep_LobbyState();
};
