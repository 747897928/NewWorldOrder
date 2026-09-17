// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "UI/LyraActivatableWidget.h"
#include "ShootSessionScreen.generated.h"

class UCommonSession_SearchResult;

/**
 * Host、Find、Join、Leave/Destroy、CleanUp 和邀请的单一联机页面。
 * 固定按钮的 OnClicked 由 WBP_SessionScreen EventGraph 连接到公开动作；C++ 只管理异步状态、
 * CommonSession 调用与原始数据通知，不持有文本、列表或布局控件。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootSessionScreen : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void HostOnlineSession();

	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void HostLanSession();

	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void FindOnlineSessions();

	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void FindLanSessions();

	/** 推入仅负责 Find/Join 的 Lyra 风格浏览页；固定按钮仍由 WBP_SessionScreen EventGraph 调用。 */
	UFUNCTION(BlueprintCallable, Category="Session|Navigation")
	void OpenSessionBrowser();

	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void ShowSessionInviteUI();

	/** 蓝图必须先显示确认框，只在 Confirmed 分支调用本函数。 */
	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void ConfirmLeaveOrDestroySession();

	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void CleanUpSessionState();

	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void CloseSessionScreen();

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual bool NativeOnHandleBackAction() override;

	/**
	 * HomeMap 位于 AssetManager 的 Map 类型目录中。UE 5.8 的 Map PrimaryAssetName 使用完整包名，
	 * 因此 WBP_SessionScreen 必须配置为 Map:/Game/Maps/HomeMap，而不是旧写法 Map:HomeMap。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Session|Host", meta=(AllowedTypes="World"))
	FPrimaryAssetId HostMapId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Session|Host", meta=(ClampMin="1"))
	int32 MaxPlayers = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Session|Host")
	FString AdvertisedMode = TEXT("HomeLobby");

	/** WBP_SessionScreen 配置为 W_SessionBrowserScreen；浏览页仍使用同一 Coordinator。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Session|Navigation")
	TSoftClassPtr<UCommonActivatableWidget> SessionBrowserClass;

	/**
	 * C++ 状态机变化后只发送业务状态和原始搜索结果。
	 * WBP_SessionScreen 负责清空/创建条目、空结果视觉和旧文案替换。
	 */
	UFUNCTION(BlueprintImplementableEvent, Category="Session|Data", meta=(DisplayName="On Session State Data Changed"))
	void BP_OnSessionStateDataChanged(EShootSessionLifecycleState State, const FText& Status,
		const TArray<UCommonSession_SearchResult*>& SearchResults);

	/** 每秒发送真实 NamedSession 与 PlayerArray 原始数据；蓝图负责标签、文本格式和玩家行视觉。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Session|Data", meta=(DisplayName="On Runtime Session Data Changed"))
	void BP_OnRuntimeSessionDataChanged(const FShootSessionRuntimeInfo& SessionInfo,
		const TArray<FShootSessionPlayerInfo>& Players);

private:
	APlayerController* GetOwningSessionPlayer() const;
	void RefreshFromCoordinator();
	void RefreshRuntimeInformation();

	UPROPERTY(Transient)
	TObjectPtr<UShootSessionCoordinatorSubsystem> Coordinator;

	FTimerHandle RuntimeInformationTimerHandle;
};
