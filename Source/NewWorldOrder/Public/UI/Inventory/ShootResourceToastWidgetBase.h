// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "UI/LyraActivatableWidget.h"
#include "Inventory/ResourceInventoryComponent.h"
#include "ShootResourceToastWidgetBase.generated.h"

/**
 * 资源拾取 Toast Widget 基类（主要承载数据推送，具体 UI 交给蓝图实现）
 *
 * 设计目的：
 * - ResourceInventory = 数量型账号仓库，拾取/制作仅通过消息告知 HUD
 * - 此 Widget 接收 GameplayMessage 推送的 FResourceChangedMessage，展示「+10 军用合金」等提示
 * - 设计师可在蓝图子类里排版/动画；C++ 负责缓存最近一次消息并调用事件
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootResourceToastWidgetBase : public ULyraActivatableWidget
{
	GENERATED_BODY()

public:
	/** HUD 组件调用，推送一条资源变化 Toast */
	UFUNCTION(BlueprintCallable, Category="ResourceToast")
	void PushResourceToast(const FResourceChangedMessage& Message);

protected:
	/** 最近一次 Toast 消息，蓝图可直接读取数据并播放动画 */
	UPROPERTY(BlueprintReadOnly, Category="ResourceToast")
	FResourceChangedMessage LastToastMessage;

	/** 蓝图实现：根据 Message 显示对应的 UI/动画/文本 */
	UFUNCTION(BlueprintImplementableEvent, Category="ResourceToast")
	void HandleResourceToast(const FResourceChangedMessage& Message);
};
