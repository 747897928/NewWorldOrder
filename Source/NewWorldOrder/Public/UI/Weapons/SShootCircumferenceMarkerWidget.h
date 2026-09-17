#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

struct FShootCircumferenceMarkerEntry;
struct FSlateBrush;
class FPaintArgs;
class FSlateRect;
class FSlateWindowElementList;
struct FGeometry;
class FWidgetStyle;

/**
 * Slate 圆周标记渲染节点：负责在圆心周围绘制角标
 */
// 约定：此 Slate 节点由 UShootCircumferenceMarkerWidget 包装，外部无需直接引用。
class SShootCircumferenceMarkerWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SShootCircumferenceMarkerWidget)
		: _MarkerBrush(nullptr)
		, _Radius(48.0f)
		, _bReticleCornerOutsideSpreadRadius(false)
	{
	}
		SLATE_ARGUMENT(const FSlateBrush*, MarkerBrush)					// 准星角标的贴图资源
		SLATE_ARGUMENT(TArray<FShootCircumferenceMarkerEntry>, MarkerList)		// 需要绘制的角标配置
		SLATE_ATTRIBUTE(float, Radius)										// 当前散布半径（像素）
		SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)						// 颜色/透明度（UMG 控制）
		SLATE_ARGUMENT(bool, bReticleCornerOutsideSpreadRadius)				// 是否把角标稍微往外挪
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	//~SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual bool ComputeVolatility() const override { return true; }
	//~End of SWidget interface

	void SetRadius(float NewRadius);
	void SetMarkerList(const TArray<FShootCircumferenceMarkerEntry>& NewMarkerList);
	void SetReticleCornerOutsideSpreadRadius(bool bOutside);

private:
	FSlateRenderTransform GetMarkerRenderTransform(const FShootCircumferenceMarkerEntry& Marker,
		float BaseRadius, float HudScale) const;

private:
	const FSlateBrush* MarkerBrush = nullptr;
	TArray<FShootCircumferenceMarkerEntry> CachedMarkerList;
	TAttribute<float> Radius;
	TAttribute<FSlateColor> ColorAndOpacity;
	bool bColorAndOpacitySet = false;
	bool bReticleCornerOutsideSpreadRadius = false;
};
