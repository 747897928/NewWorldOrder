// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CommonButtonBase.h"
#include "CoreMinimal.h"
#include "ShootSessionResultButton.generated.h"

class UCommonSession_SearchResult;

/** 搜索结果的数据/动作父类；文本、可用状态、点击事件和全部视觉由 Widget 蓝图子类维护。 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootSessionResultButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Session|Data")
	void InitializeSearchResult(UCommonSession_SearchResult* InSearchResult);
	UCommonSession_SearchResult* GetSearchResult() const { return SearchResult; }

	/** WBP_SessionResultButton 的 OnClicked 调用本函数，业务层再进入 Coordinator Join。 */
	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void JoinStoredSession();

	/** 原始搜索结果改变时通知蓝图；蓝图决定文案、文本控件、满员样式和 InputActionWidget。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Session|Data", meta=(DisplayName="On Search Result Data Changed"))
	void BP_OnSearchResultDataChanged(bool bIsValid, bool bCanJoin, const FString& GameMode,
		const FString& MapName, int32 CurrentPlayers, int32 MaxPlayers, int32 PingMilliseconds);

private:
	UPROPERTY(Transient)
	TObjectPtr<UCommonSession_SearchResult> SearchResult;
};
