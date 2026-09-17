// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "InteractionOption.h"
#include "IInteractableTarget.generated.h"

struct FInteractionQuery;
/**
 * 交互选项构建器
 * 用于帮助IInteractableTarget实现者构建交互选项
 */
class FInteractionOptionBuilder
{
public:
	FInteractionOptionBuilder(TScriptInterface<IInteractableTarget> InterfaceTargetScope, TArray<FInteractionOption>& InteractOptions)
		: Scope(InterfaceTargetScope)
		, Options(InteractOptions)
	{
	}

	// 添加交互选项到列表中
	void AddInteractionOption(const FInteractionOption& Option)
	{
		FInteractionOption& OptionEntry = Options.Add_GetRef(Option);
		OptionEntry.InteractableTarget = Scope; // 自动设置交互目标
	}

private:
	TScriptInterface<IInteractableTarget> Scope; // 当前构建器所属的交互目标
	TArray<FInteractionOption>& Options;         // 输出的交互选项列表
};

/** 
 * 可交互目标接口
 * 任何希望被交互系统识别的Actor或Component都应该实现此接口
 */
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UInteractableTarget : public UInterface
{
	GENERATED_BODY()
};

class IInteractableTarget
{
	GENERATED_BODY()

public:
	/**
	 * 收集交互选项的核心方法
	 * @param InteractQuery - 交互查询，包含请求者等信息
	 * @param OptionBuilder - 选项构建器，用于添加交互选项
	 */
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder) = 0;

	/**
	 * 自定义交互事件数据（可选实现）
	 * 在触发交互前可以修改事件数据
	 * @param InteractionEventTag - 交互事件标签
	 * @param InOutEventData - 输入输出的游戏事件数据
	 */
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag, FGameplayEventData& InOutEventData) { }
};
