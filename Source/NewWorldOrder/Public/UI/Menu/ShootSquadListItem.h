// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "UI/Foundation/ShootObjectEntryButtonBase.h"

#include "ShootSquadListItem.generated.h"

class UCommonSession_SearchResult;

/**
 * 在线小队搜索结果的可复用条目后端。
 *
 * 页面把 CommonSession 的原始结果注入本类；蓝图只接收可展示字段并决定布局、图标和选中材质。
 * 条目点击只上报给所属页面，不在条目内部直接 Join，保证玩家先选中、再按 Join Squad 确认。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class NEWWORLDORDER_API UShootSquadListItem : public UShootObjectEntryButtonBase
{
	GENERATED_BODY()

public:
	void InitializeSquadResult(UCommonSession_SearchResult* InSearchResult);

	UCommonSession_SearchResult* GetSearchResult() const { return SearchResult; }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="Expedition|Squad", meta=(DisplayName="On Squad Result Changed"))
	void BP_OnSquadResultChanged(bool bCanJoin, const FText& ExpeditionName,
		const FString& MapName, int32 CurrentPlayers, int32 MaxPlayers, int32 PingMilliseconds);

private:
	UPROPERTY(Transient)
	TObjectPtr<UCommonSession_SearchResult> SearchResult;
};
