// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "ShootSessionPlayerEntry.generated.h"

/**
 * 玩家行的数据父类。它不声明 BindWidget，也不设置任何文本或样式；
 * WBP_SessionPlayerEntry 决定头像、编号、名称、Ping 和响应式排版。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootSessionPlayerEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Session|Data")
	void InitializePlayerData(int32 DisplayIndex, const FString& PlayerName, float PingMilliseconds);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="Session|Data", meta=(DisplayName="On Player Data Changed"))
	void BP_OnPlayerDataChanged(int32 DisplayIndex, const FString& PlayerName, float PingMilliseconds);
};
