// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ShootGameModeBase.h"

#include "ShootGameMode.generated.h"

/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API AShootGameMode : public AShootGameModeBase
{
	GENERATED_BODY()

protected:
	/**
	 * UE 在此入口之后才生成 Pawn。PlayerState::BeginPlay 可能早于 ULocalPlayer 归属完成，
	 * 因此本地双人性别必须在这里按可靠索引完成最终恢复，避免两人都沿用 HomeMap 的 LastActiveGender。
	 */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	virtual ECharacterGender ResolveInitialCharacterGender_Implementation(
		const AShootPlayerState* PlayerState, ECharacterGender SavedGender) const override;
	virtual bool ShouldPersistLastActiveGender_Implementation(
		const AShootPlayerState* PlayerState) const override;

	/**
	 * 双人选择页尚未接通时，Player01 使用的可配置默认主角。
	 * Player02 始终取相反性别，保证两位本地玩家互斥；以后选择页只需覆盖规则或写入配置，
	 * 不需要修改 PlayerState、SaveGame 恢复或 Mutable 生命周期。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Local Multiplayer")
	ECharacterGender SplitPlayer01DefaultGender = ECharacterGender::MALE;
};
