// Copyright Epic Games, Inc. All Rights Reserved.
// Adapted for NewWorldOrder without modifying Lyra or engine plugins.

#include "UI/ShootTaggedWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootTaggedWidget)

UShootTaggedWidget::UShootTaggedWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootTaggedWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (!IsDesignTime())
	{
		RefreshVisibility();
	}
}

void UShootTaggedWidget::NativeDestruct()
{
	Super::NativeDestruct();
}

void UShootTaggedWidget::SetVisibility(ESlateVisibility InVisibility)
{
#if WITH_EDITORONLY_DATA
	if (IsDesignTime())
	{
		Super::SetVisibility(InVisibility);
		return;
	}
#endif

	// 与 LyraTaggedWidget 一致：记住调用者想要的状态，Tag 抑制解除后仍可恢复。
	bWantsToBeVisible = ConvertSerializedVisibilityToRuntime(InVisibility).IsVisible();
	if (bWantsToBeVisible)
	{
		ShownVisibility = InVisibility;
	}
	else
	{
		HiddenVisibility = InVisibility;
	}
	RefreshVisibility();
}

void UShootTaggedWidget::RefreshVisibility()
{
	// Lyra 5.8 示例本身仍把 Tag 查询标为 TODO；这里保持官方当前行为，不虚构第二套 ASC 监听。
	const bool bHasHiddenTags = false;
	const ESlateVisibility DesiredVisibility =
		(bWantsToBeVisible && !bHasHiddenTags) ? ShownVisibility : HiddenVisibility;
	if (GetVisibility() != DesiredVisibility)
	{
		Super::SetVisibility(DesiredVisibility);
	}
}
