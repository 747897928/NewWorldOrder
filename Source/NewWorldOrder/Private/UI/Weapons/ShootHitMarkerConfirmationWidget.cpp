#include "UI/Weapons/ShootHitMarkerConfirmationWidget.h"

#include "UI/Weapons/SShootHitMarkerConfirmationWidget.h"

UShootHitMarkerConfirmationWidget::UShootHitMarkerConfirmationWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Hit Marker 纯展示用：不参与交互，随命中事件更新。
	SetVisibility(ESlateVisibility::HitTestInvisible);
	bIsVolatile = true;
	AnyHitsMarkerImage.DrawAs = ESlateBrushDrawType::NoDrawType;
}

void UShootHitMarkerConfirmationWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	// Slate 生命周期与 UWidget 独立，释放时立即销毁内部渲染节点。
	MarkerWidget.Reset();
}

TSharedRef<SWidget> UShootHitMarkerConfirmationWidget::RebuildWidget()
{
	MarkerWidget = SNew(SShootHitMarkerConfirmationWidget)
		.PerHitMarkerImage(&PerHitMarkerImage)
		.AnyHitsMarkerImage(&AnyHitsMarkerImage)
		.HitNotifyDuration(HitNotifyDuration)
		.ZoneOverrideImages(PerHitMarkerZoneOverrideImages);

	return MarkerWidget.ToSharedRef();
}

void UShootHitMarkerConfirmationWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MarkerWidget.IsValid())
	{
		// 属性发生变化时即时同步到 Slate：贴图/时长/标签覆盖表会影响渲染结果。
		MarkerWidget->SetPerHitMarkerImage(&PerHitMarkerImage);
		MarkerWidget->SetAnyHitsMarkerImage(&AnyHitsMarkerImage);
		MarkerWidget->SetHitNotifyDuration(HitNotifyDuration);
		MarkerWidget->SetZoneOverrideImages(PerHitMarkerZoneOverrideImages);
	}
}

void UShootHitMarkerConfirmationWidget::HandleHitNotification(const FShootReticleHitNotifyMessage& Message)
{
	if (!MarkerWidget.IsValid())
	{
		return;
	}

	// Slate 层将根据 HitNotifyDuration 自动淡出，这里只负责把新数据推入。
	MarkerWidget->PushHitMarkers(Message.HitMarkers, Message.bHasSuccessfulHit);

	// 若需要在蓝图里联动其他特效，可在调用该函数前后自行处理。
}

void UShootHitMarkerConfirmationWidget::ResetHitMarkers()
{
	if (MarkerWidget.IsValid())
	{
		MarkerWidget->ResetMarkers();
	}
}

void UShootHitMarkerConfirmationWidget::RefreshAppearance()
{
	if (MarkerWidget.IsValid())
	{
		MarkerWidget->SetPerHitMarkerImage(&PerHitMarkerImage);
		MarkerWidget->SetAnyHitsMarkerImage(&AnyHitsMarkerImage);
		MarkerWidget->SetHitNotifyDuration(HitNotifyDuration);
		MarkerWidget->SetZoneOverrideImages(PerHitMarkerZoneOverrideImages);
	}
}
