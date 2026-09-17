// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted for NewWorldOrder without modifying Lyra or engine plugins.

#pragma once

#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"

#include "ShootTaggedWidget.generated.h"

/**
 * 项目版 LyraTaggedWidget。
 *
 * HUD Extension 片段不是菜单页，不应依赖 CommonActivatableWidget 的 Activate/Deactivate 才开始工作。
 * 该父类保留 Lyra 的 Construct/Destruct 与可见性语义，后续可继续接入 ASC Tag 隐藏规则。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootTaggedWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UShootTaggedWidget(const FObjectInitializer& ObjectInitializer);

	virtual void SetVisibility(ESlateVisibility InVisibility) override;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="HUD")
	FGameplayTagContainer HiddenByTags;

	UPROPERTY(EditAnywhere, Category="HUD")
	ESlateVisibility ShownVisibility = ESlateVisibility::Visible;

	UPROPERTY(EditAnywhere, Category="HUD")
	ESlateVisibility HiddenVisibility = ESlateVisibility::Collapsed;

	bool bWantsToBeVisible = true;

private:
	void RefreshVisibility();
};
