// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "ShootGameStateBase.generated.h"

class UShootExperienceManagerComponent;
class UShootExpeditionLobbyComponent;

/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API AShootGameStateBase : public AGameState
{
	GENERATED_BODY()

public:
	AShootGameStateBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category="Experience")
	UShootExperienceManagerComponent* GetExperienceManagerComponent() const { return ExperienceManagerComponent; }

	UFUNCTION(BlueprintPure, Category="Expedition|Lobby")
	UShootExpeditionLobbyComponent* GetExpeditionLobbyComponent() const { return ExpeditionLobbyComponent; }

private:
	/** 世界级 Experience 状态放在 GameState，客户端和 Listen Server 使用同一份复制结果。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Experience", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootExperienceManagerComponent> ExperienceManagerComponent;

	/** 所有地图都有该轻量组件；只有带 Expedition URL 参数的 LobbyMap 会进入等待大厅状态。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Expedition|Lobby", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UShootExpeditionLobbyComponent> ExpeditionLobbyComponent;
};
