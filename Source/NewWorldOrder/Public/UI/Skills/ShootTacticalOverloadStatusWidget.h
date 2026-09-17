// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "UI/Interaction/ShootInteractionProgressWidget.h"

#include "ShootTacticalOverloadStatusWidget.generated.h"

/**
 * 战术超载的玩家私有 HUD 片段。
 *
 * 蓝图子类复用 Interactive_Progress_Bar 的控件树和 Event Tick；本类只把同一个刷新入口改为读取
 * Status.Overload 对应的持续 GE。进度、剩余秒数、无限弹药 Tag 和角色环绕 Cue 因此共用 GAS 生命周期。
 */
UCLASS(Blueprintable)
class NEWWORLDORDER_API UShootTacticalOverloadStatusWidget : public UShootInteractionProgressWidget
{
	GENERATED_BODY()

public:
	virtual void RefreshInteractionProgress() override;

protected:
	/** 固定文案由 W_TacticalOverloadStatus 蓝图配置，C++ 只追加运行时剩余秒数。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI|Tactical Overload")
	FText ActiveLabel;
};
