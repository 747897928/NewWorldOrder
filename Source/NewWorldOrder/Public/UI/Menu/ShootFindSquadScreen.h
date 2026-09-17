// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Online/ShootSessionCoordinatorSubsystem.h"
#include "UI/LyraActivatableWidget.h"

#include "ShootFindSquadScreen.generated.h"

class UCommonSession_SearchResult;
class UDynamicEntryBox;
class UShootObjectEntryButtonBase;
class UShootSessionCoordinatorSubsystem;
class UShootSquadListItem;

/** Find Squads 页面后端；复用现有 Coordinator，不创建第二套搜索或 Join 状态机。 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootFindSquadScreen : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	/** 注册蓝图选择的容器与条目类，并立即同步当前搜索结果。 */
	UFUNCTION(BlueprintCallable, Category="Expedition|Squad")
	void PopulateSquadEntries(
		UDynamicEntryBox* EntryBox,
		TSubclassOf<UShootSquadListItem> EntryWidgetClass);

	UFUNCTION(BlueprintCallable, Category="Expedition|Squad")
	void RefreshSquads();

	UFUNCTION(BlueprintCallable, Category="Expedition|Squad")
	bool JoinSelectedSquad();

	UFUNCTION(BlueprintCallable, Category="Expedition|Squad")
	void CloseScreen();

	UFUNCTION(BlueprintPure, Category="Expedition|Squad")
	bool CanJoinSelectedSquad() const;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual bool NativeOnHandleBackAction() override;

	UFUNCTION(BlueprintImplementableEvent, Category="Expedition|Squad", meta=(DisplayName="On Squad Search Changed"))
	void BP_OnSquadSearchChanged(
		EShootSessionLifecycleState State, const FText& Status, int32 ResultCount, bool bCanJoinSelection);

private:
	void RefreshFromCoordinator();
	void RebuildSquadEntries();
	void HandleEntryClicked(UShootObjectEntryButtonBase* Entry, UObject* EntryObject);
	void HandleEntryHovered(UShootObjectEntryButtonBase* Entry, UObject* EntryObject);
	void RefreshEntrySelection();
	APlayerController* GetOwningSessionPlayer() const;

	UPROPERTY(Transient)
	TObjectPtr<UShootSessionCoordinatorSubsystem> Coordinator;

	UPROPERTY(Transient)
	TObjectPtr<UDynamicEntryBox> SquadEntryBox;

	UPROPERTY(Transient)
	TSubclassOf<UShootSquadListItem> SquadEntryClass;

	UPROPERTY(Transient)
	TObjectPtr<UCommonSession_SearchResult> SelectedResult;
};
