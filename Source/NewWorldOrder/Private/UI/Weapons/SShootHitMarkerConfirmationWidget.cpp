#include "UI/Weapons/SShootHitMarkerConfirmationWidget.h"

#include "Rendering/DrawElements.h"
#include "UI/Weapons/ShootHitMarkerTypes.h"

void SShootHitMarkerConfirmationWidget::Construct(const FArguments& InArgs)
{
	PerHitMarkerImage = InArgs._PerHitMarkerImage;
	AnyHitsMarkerImage = InArgs._AnyHitsMarkerImage;
	HitNotifyDuration = InArgs._HitNotifyDuration;
	bColorAndOpacitySet = InArgs._ColorAndOpacity.IsSet();
	ColorAndOpacity = InArgs._ColorAndOpacity;
	ZoneOverrideImages = InArgs._ZoneOverrideImages;
}

int32 SShootHitMarkerConfirmationWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
	const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const bool bIsEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect DrawEffects = bIsEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	const FVector2D LocalCenter = AllottedGeometry.GetLocalPositionAtCoordinates(FVector2D(0.5f, 0.5f));

	if (ActiveMarkers.Num() > 0)
	{
		for (const FActiveMarker& ActiveMarker : ActiveMarkers)
		{
			const float MarkerOpacity = GetMarkerOpacity(ActiveMarker);
			if (MarkerOpacity <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const FSlateBrush* MarkerBrush = ZoneOverrideImages.Find(ActiveMarker.Marker.HitZone);
			if (MarkerBrush == nullptr)
			{
				MarkerBrush = PerHitMarkerImage;
			}
			if (MarkerBrush == nullptr)
			{
				continue;
			}

			FLinearColor MarkerColor = bColorAndOpacitySet
				? ColorAndOpacity.Get().GetColor(InWidgetStyle)
				: (InWidgetStyle.GetColorAndOpacityTint() * MarkerBrush->GetTint(InWidgetStyle));
			// 命中淡出：透明度 = 1 - (存活时间 / 持续时间)
			MarkerColor.A *= MarkerOpacity;

			// 命中坐标以窗口左上角为原点，叠加 MyCullingRect 偏移可以兼容窗口化模式；随后转换到当前控件本地坐标。
			const FVector2D WindowSSLocation = ActiveMarker.Marker.ScreenPosition + MyCullingRect.GetTopLeft();
			const FSlateRenderTransform DrawPos(AllottedGeometry.AbsoluteToLocal(WindowSSLocation));
			const FPaintGeometry Geometry(AllottedGeometry.ToPaintGeometry(
				MarkerBrush->ImageSize,
				FSlateLayoutTransform(-(MarkerBrush->ImageSize * 0.5f)),
				DrawPos));

			FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry, MarkerBrush, DrawEffects, MarkerColor);
		}
	}

	if (AnyHitsMarkerImage != nullptr && CachedAnyHitOpacity > KINDA_SMALL_NUMBER)
	{
		FLinearColor MarkerColor = bColorAndOpacitySet
			? ColorAndOpacity.Get().GetColor(InWidgetStyle)
			: (InWidgetStyle.GetColorAndOpacityTint() * AnyHitsMarkerImage->GetTint(InWidgetStyle));
		MarkerColor.A *= CachedAnyHitOpacity;

		const FPaintGeometry Geometry(AllottedGeometry.ToPaintGeometry(
			AnyHitsMarkerImage->ImageSize,
			FSlateLayoutTransform(LocalCenter - (AnyHitsMarkerImage->ImageSize * 0.5f))));

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, Geometry, AnyHitsMarkerImage, DrawEffects, MarkerColor);
	}

	return LayerId;
}

void SShootHitMarkerConfirmationWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	const float Duration = FMath::Max(0.01f, HitNotifyDuration.Get());
	CachedAnyHitOpacity = 0.0f;

	if (CenterHitElapsedTime <= Duration)
	{
		CenterHitElapsedTime += InDeltaTime;
		const float CenterOpacity = FMath::Clamp(1.0f - (CenterHitElapsedTime / Duration), 0.0f, 1.0f);
		CachedAnyHitOpacity = FMath::Max(CachedAnyHitOpacity, CenterOpacity);
	}

	for (int32 Index = ActiveMarkers.Num() - 1; Index >= 0; --Index)
	{
		FActiveMarker& ActiveMarker = ActiveMarkers[Index];
		ActiveMarker.ElapsedTime += InDeltaTime;

		// 每帧根据存活时间实时刷新透明度；中心提示取最大值，实现多命中叠加的效果。
		const float MarkerOpacity = GetMarkerOpacity(ActiveMarker);
		CachedAnyHitOpacity = FMath::Max(CachedAnyHitOpacity, MarkerOpacity);

		if (ActiveMarker.ElapsedTime >= Duration)
		{
			ActiveMarkers.RemoveAtSwap(Index);
		}
	}

	if ((ActiveMarkers.Num() == 0) && (CenterHitElapsedTime > Duration))
	{
		// 没有命中数据时保持透明度为 0，避免残影。
		CachedAnyHitOpacity = 0.0f;
	}
}

FVector2D SShootHitMarkerConfirmationWidget::ComputeDesiredSize(float) const
{
	return FVector2D(100.0f, 100.0f);
}

void SShootHitMarkerConfirmationWidget::PushHitMarkers(const TArray<FShootReticleHitLocation>& NewMarkers, bool bAnySuccessfulHit)
{
	for (const FShootReticleHitLocation& Marker : NewMarkers)
	{
		FActiveMarker& NewEntry = ActiveMarkers.AddDefaulted_GetRef();
		NewEntry.Marker = Marker;
		NewEntry.ElapsedTime = 0.0f;
	}

	if (bAnySuccessfulHit || NewMarkers.Num() > 0)
	{
		// CenterHitElapsedTime=0 表示触发中心确认提示；即使没有具体屏幕坐标，也能给玩家“已命中”反馈。
		CenterHitElapsedTime = 0.0f;
	}
}

void SShootHitMarkerConfirmationWidget::SetPerHitMarkerImage(const FSlateBrush* InBrush)
{
	PerHitMarkerImage = InBrush;
}

void SShootHitMarkerConfirmationWidget::SetAnyHitsMarkerImage(const FSlateBrush* InBrush)
{
	AnyHitsMarkerImage = InBrush;
}

void SShootHitMarkerConfirmationWidget::SetHitNotifyDuration(float NewDuration)
{
	HitNotifyDuration = NewDuration;
}

void SShootHitMarkerConfirmationWidget::SetZoneOverrideImages(const FShootHitZoneBrushMap& InImages)
{
	ZoneOverrideImages = InImages;
}

void SShootHitMarkerConfirmationWidget::ResetMarkers()
{
	ActiveMarkers.Reset();
	CachedAnyHitOpacity = 0.0f;
	CenterHitElapsedTime = TNumericLimits<float>::Max();
}

float SShootHitMarkerConfirmationWidget::GetMarkerOpacity(const FActiveMarker& ActiveMarker) const
{
	const float Duration = FMath::Max(0.01f, HitNotifyDuration.Get());
	// 淡出曲线：线性插值 1 → 0，可后续替换为自定义曲线（如指数衰减）时统一改动此函数。
	const float Remaining = FMath::Clamp(1.0f - (ActiveMarker.ElapsedTime / Duration), 0.0f, 1.0f);
	return Remaining;
}
