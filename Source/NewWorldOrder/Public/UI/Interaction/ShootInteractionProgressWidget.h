// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "ShootInteractionProgressWidget.generated.h"

class UCommonTextBlock;
class UProgressBar;

/** 每个 LocalPlayer 独立的通用交互读条；当前由补给站使用。 */
UCLASS()
class NEWWORLDORDER_API UShootInteractionProgressWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UShootInteractionProgressWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 由 Interactive_Progress_Bar 的 Event Tick 调用，读取所属 LocalPlayer Controller 的读条状态。
	 * Lyra 的 W_WeaponReticleHost 同样保留已连线的蓝图 Tick 作为可读表现入口；空 Tick 会被 UMG 编译器裁掉。
	 */
	UFUNCTION(BlueprintCallable, Category="UI|Interaction")
	virtual void RefreshInteractionProgress();

protected:
	virtual void NativeConstruct() override;

	/** 由 Interactive_Progress_Bar Widget Blueprint 提供；样式和布局留在 UMG。 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UProgressBar> InteractionProgressBar;

	/** 由 Interactive_Progress_Bar Widget Blueprint 提供；文案由蓝图预设。 */
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCommonTextBlock> MessageCommonText;
};
