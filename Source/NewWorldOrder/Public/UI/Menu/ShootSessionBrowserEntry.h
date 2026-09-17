// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "ShootSessionBrowserEntry.generated.h"

class UCommonSession_SearchResult;

/**
 * Lyra 风格会话列表行的数据父类。
 * W_SessionBrowserEntry 的控件、文案、图片与按钮 OnClicked 全部留在蓝图；
 * 本类只把 CommonSession 原始结果拆成字段，并把加入动作送回项目 Coordinator。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootSessionBrowserEntry : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	/** W_SessionBrowserEntry 的 SessionButton OnClicked 调用本函数。 */
	UFUNCTION(BlueprintCallable, Category="Session|Actions")
	void JoinStoredSession();

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/** 蓝图决定所有文本组合、图标、满员样式和响应式排版。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Session|Data", meta=(DisplayName="On Search Result Data Changed"))
	void BP_OnSearchResultDataChanged(bool bIsValid, bool bCanJoin, const FString& GameMode,
		const FString& MapName, int32 CurrentPlayers, int32 MaxPlayers, int32 PingMilliseconds);

private:
	UPROPERTY(Transient)
	TObjectPtr<UCommonSession_SearchResult> SearchResult;
};
