// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "UI/LyraActivatableWidget.h"

#include "ShootExpeditionLobbyScreen.generated.h"

class UShootExpeditionLobbyComponent;
class UShootSessionCoordinatorSubsystem;
class UCommonButtonBase;
enum class ECommonMessagingResult : uint8;

/** LobbyMap 的等待大厅页面后端；玩家列表来自 GameState，副本选择来自复制的 LobbyComponent。 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootExpeditionLobbyScreen : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Expedition|Lobby")
	bool StartExpedition();

	/** 房主在 Online Lobby 中打开当前平台的好友邀请 Overlay。 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Lobby")
	bool InviteFriends();

	/** 蓝图先显示确认框，再调用；Host 销毁房间，Client 离开房间。 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Lobby")
	void ConfirmLeaveLobby();

	UFUNCTION(BlueprintPure, Category="Expedition|Lobby")
	bool CanLocalPlayerStartExpedition() const;

	UFUNCTION(BlueprintPure, Category="Expedition|Lobby")
	bool CanInviteFriends() const;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

	UFUNCTION(BlueprintImplementableEvent, Category="Expedition|Lobby", meta=(DisplayName="On Lobby Data Changed"))
	void BP_OnLobbyDataChanged(
		FPrimaryAssetId SelectedMap,
		FPrimaryAssetId SelectedExperience,
		bool bAllowJoinInProgress,
		bool bBotFillAvailable,
		const TArray<FShootSessionPlayerInfo>& Players,
		bool bCanStart);

	UFUNCTION(BlueprintImplementableEvent, Category="Expedition|Lobby", meta=(DisplayName="On Lobby Session State Changed"))
	void BP_OnLobbySessionStateChanged(EShootSessionLifecycleState State, const FText& Status);

	/** 只控制业务可用状态；按钮文案、位置和样式由 W_ExpeditionLobbyScreen 维护。 */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> StartButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UCommonButtonBase> InviteButton;

private:
	void BindLobbyComponent();

	UFUNCTION()
	void RefreshLobbyData();
	void RefreshCoordinatorState();
	void HandleLeaveLobbyConfirmation(ECommonMessagingResult Result);

	UPROPERTY(Transient)
	TObjectPtr<UShootExpeditionLobbyComponent> LobbyComponent;

	UPROPERTY(Transient)
	TObjectPtr<UShootSessionCoordinatorSubsystem> Coordinator;

	FTimerHandle LobbyRefreshTimerHandle;
};
