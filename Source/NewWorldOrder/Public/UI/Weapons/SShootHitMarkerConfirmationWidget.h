#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateColor.h"
#include "Styling/SlateBrush.h"
#include "UI/Weapons/ShootHitMarkerTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"

class FPaintArgs;
class FSlateRect;
class FSlateWindowElementList;
class FWidgetStyle;

using FShootHitZoneBrushMap = TMap<FGameplayTag, FSlateBrush>;

/**
 * Slate 命中提示渲染节点：负责淡出动画与屏幕空间绘制
 * 说明：命中数据通过 PushHitMarkers 传入，Tick 内更新存活时间并驱动透明度
 */
class SShootHitMarkerConfirmationWidget : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SShootHitMarkerConfirmationWidget)
		: _PerHitMarkerImage(nullptr)
		, _AnyHitsMarkerImage(nullptr)
		, _HitNotifyDuration(0.4f)
	{
	}
		SLATE_ARGUMENT(const FSlateBrush*, PerHitMarkerImage)						// 屏幕空间命中点默认贴图
		SLATE_ARGUMENT(const FSlateBrush*, AnyHitsMarkerImage)						// 任意命中时绘制在中心的提示贴图
		SLATE_ATTRIBUTE(float, HitNotifyDuration)									// 每次命中淡出的持续时间
		SLATE_ATTRIBUTE(FSlateColor, ColorAndOpacity)								// 颜色/透明度（可由 UMG 动画驱动）
		SLATE_ARGUMENT(FShootHitZoneBrushMap, ZoneOverrideImages)					// 按命中区域覆盖贴图
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	//~SWidget interface
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
		const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual bool ComputeVolatility() const override { return true; }
	//~End of SWidget interface

	/** 推入新的命中点数据（会立即开始计时并参与淡出） */
	void PushHitMarkers(const TArray<FShootReticleHitLocation>& NewMarkers, bool bAnySuccessfulHit);

	/** 更新默认贴图引用（当 UWidget 属性改变时调用） */
	void SetPerHitMarkerImage(const FSlateBrush* InBrush);

	/** 更新任意命中的中心贴图 */
	void SetAnyHitsMarkerImage(const FSlateBrush* InBrush);

	/** 更新淡出时间 */
	void SetHitNotifyDuration(float NewDuration);

	/** 更新命中区域覆盖贴图 */
	void SetZoneOverrideImages(const FShootHitZoneBrushMap& InImages);

	/** 清空历史命中点 */
	void ResetMarkers();

private:
	struct FActiveMarker
	{
		FShootReticleHitLocation Marker;	// 命中屏幕坐标与标签
		float ElapsedTime = 0.0f;			// 已经存在的时间（秒）
	};

	float GetMarkerOpacity(const FActiveMarker& ActiveMarker) const;

private:
	const FSlateBrush* PerHitMarkerImage = nullptr;
	const FSlateBrush* AnyHitsMarkerImage = nullptr;
	TAttribute<float> HitNotifyDuration;
	TAttribute<FSlateColor> ColorAndOpacity;
	bool bColorAndOpacitySet = false;

	/** 不同标签对应的自定义贴图 */
	FShootHitZoneBrushMap ZoneOverrideImages;

	/** 当前仍在淡出队列中的命中点 */
	TArray<FActiveMarker> ActiveMarkers;

	/** 用于中心贴图透明度的缓存（Tick 中更新，OnPaint 中使用） */
	float CachedAnyHitOpacity = 0.0f;

	/** 中心命中提示的存活时间（与 HitNotifyDuration 同步递减） */
	float CenterHitElapsedTime = TNumericLimits<float>::Max();
};
