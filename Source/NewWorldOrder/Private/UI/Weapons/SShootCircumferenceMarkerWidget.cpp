#include "UI/Weapons/SShootCircumferenceMarkerWidget.h"

#include "Engine/UserInterfaceSettings.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderTransform.h"
#include "Styling/SlateBrush.h"
#include "UI/Weapons/ShootCircumferenceMarkerWidget.h"

void SShootCircumferenceMarkerWidget::Construct(const FArguments& InArgs)
{
	// 保存 UMG 传入的绘制参数，后续 OnPaint 会频繁读取。
	MarkerBrush = InArgs._MarkerBrush;
	CachedMarkerList = InArgs._MarkerList;
	Radius = InArgs._Radius;
	bColorAndOpacitySet = InArgs._ColorAndOpacity.IsSet();
	ColorAndOpacity = InArgs._ColorAndOpacity;
	bReticleCornerOutsideSpreadRadius = InArgs._bReticleCornerOutsideSpreadRadius;
}

int32 SShootCircumferenceMarkerWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 每次绘制都会基于最新半径重新计算所有角标位置，用于表现散布变化。
	const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	// LocalCenter：计算以控件中心为圆心，保证无论控件尺寸如何变化，角标都围绕中心扩张。
	const FVector2D LocalCenter = AllottedGeometry.GetLocalPositionAtCoordinates(FVector2D(0.5f, 0.5f));

	const bool bDrawMarkers = (CachedMarkerList.Num() > 0) && (MarkerBrush != nullptr);
	if (bDrawMarkers)
	{
		// MarkerColor：优先使用 UMG 传入的颜色（可支持动画驱动），否则按照 Brush 默认色叠加 Widget Tint。
		const FLinearColor MarkerColor = bColorAndOpacitySet
			? ColorAndOpacity.Get().GetColor(InWidgetStyle)
			: (InWidgetStyle.GetColorAndOpacityTint() * MarkerBrush->GetTint(InWidgetStyle));

		if (MarkerColor.A > KINDA_SMALL_NUMBER)
		{
			const float BaseRadius = Radius.Get();
			// UUserInterfaceSettings 的 ApplicationScale 可适配不同 UI 缩放，确保在 4K/超宽等分辨率下位置仍正确。
			const float ApplicationScale = GetDefault<UUserInterfaceSettings>()->ApplicationScale;

			for (const FShootCircumferenceMarkerEntry& Marker : CachedMarkerList)
			{
				const FSlateRenderTransform MarkerTransform =
					GetMarkerRenderTransform(Marker, BaseRadius, ApplicationScale);

				// ToPaintGeometry：将角标图像尺寸（Slate 单位）转换为世界坐标，叠加旋转和平移。
				const FPaintGeometry Geometry(AllottedGeometry.ToPaintGeometry(
					MarkerBrush->ImageSize,
					FSlateLayoutTransform(LocalCenter - (MarkerBrush->ImageSize * 0.5f)),
					MarkerTransform,
					FVector2D::ZeroVector));

				// 实际 DrawCall：按颜色 / 透明度绘制 Box（即角标图像）。
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry, MarkerBrush, DrawEffects, MarkerColor);
			}
		}
	}

	return LayerId;
}

FVector2D SShootCircumferenceMarkerWidget::ComputeDesiredSize(float) const
{
	check(MarkerBrush);
	const float SampledRadius = Radius.Get();
	// 控件尺寸 = 圆直径 + 图像尺寸，用于 UMG 在布局时预留空间。
	return FVector2D((MarkerBrush->ImageSize.X + SampledRadius) * 2.0f,
		(MarkerBrush->ImageSize.Y + SampledRadius) * 2.0f);
}

void SShootCircumferenceMarkerWidget::SetRadius(float NewRadius)
{
	if (Radius.IsBound() || (Radius.Get() != NewRadius))
	{
		Radius = NewRadius;
		// 半径影响控件布局范围 → 触发布局无效化，让 UMG 重新计算 DesiredSize。
		Invalidate(EInvalidateWidgetReason::Layout);
	}
}

void SShootCircumferenceMarkerWidget::SetMarkerList(const TArray<FShootCircumferenceMarkerEntry>& NewMarkerList)
{
	CachedMarkerList = NewMarkerList;
	// Marker 数量或角度发生变化，需要重绘。
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SShootCircumferenceMarkerWidget::SetReticleCornerOutsideSpreadRadius(bool bOutside)
{
	if (bReticleCornerOutsideSpreadRadius != bOutside)
	{
		bReticleCornerOutsideSpreadRadius = bOutside;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

FSlateRenderTransform SShootCircumferenceMarkerWidget::GetMarkerRenderTransform(
	const FShootCircumferenceMarkerEntry& Marker, float BaseRadius, float HudScale) const
{
	float XRadius = BaseRadius;
	float YRadius = BaseRadius;
	if (bReticleCornerOutsideSpreadRadius && MarkerBrush)
	{
		// 当角标需要绘制在散布圈外侧时，把半径向外偏移半个图像宽度，让视觉效果贴合圈边缘。
		XRadius += MarkerBrush->ImageSize.X * 0.5f;
		YRadius += MarkerBrush->ImageSize.Y * 0.5f;
	}

	// PositionAngle：角标在圆周上的方位；ImageRotationAngle：角标自身的朝向。
	// 先用四元数旋转角标自身，再把它平移到圆周上的对应位置。
	const float LocalRotationRadians = FMath::DegreesToRadians(Marker.ImageRotationAngle);
	const float PositionAngleRadians = FMath::DegreesToRadians(Marker.PositionAngle);

	const FSlateRenderTransform RotateAboutOrigin(Concatenate(
		FVector2D(-MarkerBrush->ImageSize.X * 0.5f, -MarkerBrush->ImageSize.Y * 0.5f),
		FQuat2D(LocalRotationRadians),
		FVector2D(MarkerBrush->ImageSize.X * 0.5f, MarkerBrush->ImageSize.Y * 0.5f)));

	return TransformCast<FSlateRenderTransform>(Concatenate(
		RotateAboutOrigin,
		FVector2D(XRadius * FMath::Sin(PositionAngleRadians) * HudScale,
			-YRadius * FMath::Cos(PositionAngleRadians) * HudScale)));
	// 注意：屏幕坐标系 X 向右、Y 向下，因此需要 sin 放在 X，cos 放在 Y，并对 Y 取负号，让 0° 对应屏幕正上方。
}
