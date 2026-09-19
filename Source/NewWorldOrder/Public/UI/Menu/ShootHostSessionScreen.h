// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonSessionSubsystem.h"
#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "UI/LyraActivatableWidget.h"

#include "ShootHostSessionScreen.generated.h"

class ULyraUserFacingExperienceDefinition;
class UCommonActivatableWidget;
class UCommonSession_SearchResult;
class UDynamicEntryBox;
class UShootObjectEntryButtonBase;
class UShootLocalCoopSetupScreen;
class UShootSessionCoordinatorSubsystem;

/**
 * HomeMap 副本门打开的统一页面后端。
 *
 * UserFacingExperience 只描述目录卡片、目标地图和开房参数；实际 HUD、能力与相机仍由
 * UShootExperienceDefinition 管理。Widget Blueprint 负责列表布局和固定按钮，本类只提供
 * 可枚举数据、选项状态与 CommonSession 调用，避免第二套 Session/Experience 状态机。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootHostSessionScreen : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Expedition|Selection")
	void SelectExperience(ULyraUserFacingExperienceDefinition* Experience);

	/**
	 * 设置当前游玩方式并刷新目录条目。
	 * ModeId 必须与 Tab 的稳定 Name ID 一致，例如 Experience.Participation.LocalCoop。
	 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Selection")
	void SetExpeditionMode(FName ModeId);

	UFUNCTION(BlueprintPure, Category="Expedition|Selection")
	FName GetExpeditionMode() const { return SelectedExpeditionMode; }

	/**
	 * 将现有 UserFacingExperience 目录填入蓝图指定的 DynamicEntryBox。
	 * 容器和条目样式仍由 UMG 决定；C++ 只负责运行时创建、对象注入与选择回调，页面无需 BindWidget。
	 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Selection")
	void PopulateExperienceEntries(
		UDynamicEntryBox* EntryBox,
		TSubclassOf<UShootObjectEntryButtonBase> EntryWidgetClass);

	UFUNCTION(BlueprintPure, Category="Expedition|Selection")
	ULyraUserFacingExperienceDefinition* GetSelectedExperience() const { return SelectedDefinition; }

	/** 蓝图刷新详情时使用安全值，避免目录尚未加载时从空 Experience 读取字段。 */
	UFUNCTION(BlueprintPure, Category="Expedition|Selection")
	FText GetSelectedExperienceTitle() const;

	UFUNCTION(BlueprintPure, Category="Expedition|Selection")
	FText GetSelectedExperienceDescription() const;

	UFUNCTION(BlueprintPure, Category="Expedition|Selection")
	int32 GetSelectedExperienceMaxPlayerCount() const;

	UFUNCTION(BlueprintCallable, Category="Expedition|Options")
	void SetOnlineMode(bool bOnline);

	UFUNCTION(BlueprintCallable, Category="Expedition|Options")
	void SetRequestedMaxPlayers(int32 NewMaxPlayers);

	UFUNCTION(BlueprintCallable, Category="Expedition|Options")
	void SetLocalPlayerCount(int32 NewLocalPlayerCount);

	UFUNCTION(BlueprintCallable, Category="Expedition|Options")
	void AddLocalPlayer();

	UFUNCTION(BlueprintCallable, Category="Expedition|Options")
	void RemoveLocalPlayer();

	UFUNCTION(BlueprintCallable, Category="Expedition|Options")
	void SetAllowJoinInProgress(bool bAllowed);

	UFUNCTION(BlueprintCallable, Category="Expedition|Options")
	void SetFillEmptySlotsWithBots(bool bEnabled);

	UFUNCTION(BlueprintPure, Category="Expedition|Options")
	bool IsOnlineMode() const { return SelectedOnlineMode == ECommonSessionOnlineMode::Online; }

	/** 当前副本是否允许创建 Listen Server；蓝图可据此禁用 Online 选项并显示说明。 */
	UFUNCTION(BlueprintPure, Category="Expedition|Options")
	bool DoesSelectedExperienceSupportOnline() const;

	UFUNCTION(BlueprintPure, Category="Expedition|Options")
	int32 GetRequestedMaxPlayers() const { return RequestedMaxPlayers; }

	UFUNCTION(BlueprintPure, Category="Expedition|Options")
	int32 GetLocalPlayerCount() const { return LocalPlayerCount; }

	UFUNCTION(BlueprintPure, Category="Expedition|Options")
	bool CanAddLocalPlayer() const { return !IsOnlineMode() && LocalPlayerCount < 2; }

	UFUNCTION(BlueprintPure, Category="Expedition|Options")
	bool CanRemoveLocalPlayer() const { return !IsOnlineMode() && LocalPlayerCount > 1; }

	UFUNCTION(BlueprintPure, Category="Expedition|Options")
	bool IsJoinInProgressAllowed() const { return bAllowJoinInProgress; }

	UFUNCTION(BlueprintPure, Category="Expedition|Options")
	bool IsBotFillEnabled() const { return bFillEmptySlotsWithBots; }

	/** AI 队友尚未落地时返回 false；蓝图应显示禁用态和“后续开放”，不能伪装为已生效。 */
	UFUNCTION(BlueprintPure, Category="Expedition|Options")
	bool IsBotFillOptionAvailable() const { return bEnableBotFillOption; }

	UFUNCTION(BlueprintCallable, Category="Expedition|Actions")
	bool HostSelectedExperience();

	/** 单人入口的完整语义动作；蓝图不再先拼装 OnlineMode、人数等内部选项。 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Actions")
	bool StartSinglePlayerExperience();

	/** 打开独立的本地双人设备与主角分配页面。 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Actions")
	void OpenLocalCoopSetup();

	/** 在线入口直接创建 1–4 人 Listen Server 等待大厅。 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Actions")
	bool CreateOnlineSquad();

	UFUNCTION(BlueprintCallable, Category="Expedition|Actions")
	void FindOnlineSessions();

	UFUNCTION(BlueprintCallable, Category="Expedition|Navigation")
	void OpenSessionBrowser();

	UFUNCTION(BlueprintCallable, Category="Expedition|Navigation")
	void CloseScreen();

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual bool NativeOnHandleBackAction() override;

	/** C++ 刷新目录后，蓝图只需重建 W_ExperienceList 并选择 SuggestedSelection。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Expedition|Data", meta=(DisplayName="On Experience Catalog Changed"))
	void BP_OnExperienceCatalogChanged(
		const TArray<ULyraUserFacingExperienceDefinition*>& Experiences,
		ULyraUserFacingExperienceDefinition* SuggestedSelection);

	/** 选项或选择发生变化时通知蓝图刷新按钮可用性与摘要。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Expedition|Data", meta=(DisplayName="On Expedition Options Changed"))
	void BP_OnExpeditionOptionsChanged();

	/** Hosting/Searching/Error 等异步状态统一来自现有 Coordinator。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Expedition|Data", meta=(DisplayName="On Expedition Session State Changed"))
	void BP_OnExpeditionSessionStateChanged(EShootSessionLifecycleState State, const FText& Status);

	/** 默认 false：先保留最终语义，不在没有 AI 队友实现时让开关产生虚假效果。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Expedition|Bots")
	bool bEnableBotFillOption = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Expedition|Navigation")
	TSoftClassPtr<UCommonActivatableWidget> SessionBrowserClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Expedition|Navigation")
	TSoftClassPtr<UCommonActivatableWidget> LocalCoopSetupClass;

private:
	void RefreshExperienceCatalog();
	void RefreshExperienceCatalogForSelectedMode();
	void RefreshCoordinatorState();
	void NotifyOptionsChanged();
	void HandleExperienceEntryClicked(UShootObjectEntryButtonBase* Entry, UObject* EntryObject);
	void HandleExperienceEntryHovered(UShootObjectEntryButtonBase* Entry, UObject* EntryObject);
	void RefreshExperienceEntrySelection();
	bool HostSelectedExperienceWithOptions(ECommonSessionOnlineMode OnlineMode,
		int32 MaxPlayers, int32 LocalPlayers, bool bAllowJoinInProgress);
	APlayerController* GetOwningSessionPlayer() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULyraUserFacingExperienceDefinition>> ExperienceCatalog;

	/** UFE 是唯一目录源；ExperienceCatalog 是当前 Tab 的过滤视图，AllExperienceCatalog 保留完整目录。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ULyraUserFacingExperienceDefinition>> AllExperienceCatalog;

	UPROPERTY(Transient)
	TObjectPtr<ULyraUserFacingExperienceDefinition> SelectedDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UShootSessionCoordinatorSubsystem> Coordinator;

	/** 由 PopulateExperienceEntries 注入，页面可自由替换或移动容器，不形成固定 BindWidget 契约。 */
	UPROPERTY(Transient)
	TObjectPtr<UDynamicEntryBox> ExperienceEntryBox;

	ECommonSessionOnlineMode SelectedOnlineMode = ECommonSessionOnlineMode::Offline;
	FName SelectedExpeditionMode = TEXT("Experience.Participation.SinglePlayer");
	int32 RequestedMaxPlayers = 4;
	int32 LocalPlayerCount = 1;
	bool bAllowJoinInProgress = true;
	bool bFillEmptySlotsWithBots = false;
};
