#include "UI/Weapons/ShootCircumferenceMarkerWidget.h"

#include "UI/Weapons/SShootCircumferenceMarkerWidget.h"

UShootCircumferenceMarkerWidget::UShootCircumferenceMarkerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 圆周准星通常只负责显示，不需要命中检测，因此默认不可交互并启用频繁重绘（随散布变化）。
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsVolatile = true;
}

void UShootCircumferenceMarkerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	// Slate 生命周期与 UWidget 不同，此处显式销毁，防止旧实例持有无效引用。
	MarkerWidget.Reset();
}

TSharedRef<SWidget> UShootCircumferenceMarkerWidget::RebuildWidget()
{
	// 与 Lyra 类似，将 Slate 层封装为控件，蓝图只需传参即可。
	MarkerWidget = SNew(SShootCircumferenceMarkerWidget)
		.MarkerBrush(&MarkerImage)
		.MarkerList(MarkerList)
		.Radius(Radius)
		.bReticleCornerOutsideSpreadRadius(bReticleCornerOutsideSpreadRadius);
	// MarkerBrush 使用引用是为了让蓝图/数据资产更新材质时可即时生效（无需复制整份 Brush）。

	return MarkerWidget.ToSharedRef();
}

void UShootCircumferenceMarkerWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MarkerWidget.IsValid())
	{
		// 蓝图/属性面板数值变化时，推送到 Slate 层。
		MarkerWidget->SetRadius(Radius);
		MarkerWidget->SetMarkerList(MarkerList);
		MarkerWidget->SetReticleCornerOutsideSpreadRadius(bReticleCornerOutsideSpreadRadius);
	}
}

void UShootCircumferenceMarkerWidget::SetRadius(float InRadius)
{
	Radius = InRadius;
	if (MarkerWidget.IsValid())
	{
		// HUD 组件可在运行时调用（例如散布缩放消息），此处立刻刷新 Slate。
		// 推荐流程：UShootReticleWidgetBase::ComputeMaxScreenspaceSpreadRadius() → SetRadius()。
		MarkerWidget->SetRadius(InRadius);
	}
}

void UShootCircumferenceMarkerWidget::SetMarkerList(const TArray<FShootCircumferenceMarkerEntry>& InMarkerList)
{
	MarkerList = InMarkerList;
	if (MarkerWidget.IsValid())
	{
		// 更换不同武器布局时调用，可在 C++ 或蓝图中设置自定义角度。
		MarkerWidget->SetMarkerList(InMarkerList);
	}
}

void UShootCircumferenceMarkerWidget::SetReticleCornerOutsideSpreadRadius(bool bOutside)
{
	bReticleCornerOutsideSpreadRadius = bOutside;
	if (MarkerWidget.IsValid())
	{
		MarkerWidget->SetReticleCornerOutsideSpreadRadius(bOutside);
	}
}
