#pragma once

#include "Components/Widget.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"
#include "UI/Weapons/ShootHitMarkerTypes.h"
#include "ShootHitMarkerConfirmationWidget.generated.h"

class SShootHitMarkerConfirmationWidget;

/**
 * 命中提示控件（Reticle 中央的 Hit Marker）
 * - 关键逻辑下沉到 C++：Slate 节点负责绘制与淡出，蓝图只需传入贴图和命中数据
 * - HUD/准星蓝图在收到命中消息（UGameplayMessageSubsystem 或 Ability 委托）后，调用 HandleHitNotification 即可
 */
UCLASS()
class UShootHitMarkerConfirmationWidget : public UWidget
{
	GENERATED_BODY()

public:
	UShootHitMarkerConfirmationWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UWidget interface
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	//~End of UWidget interface

	//~UVisual interface
public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~End of UVisual interface

public:
	/** 命中提示持续时间（秒），超时后淡出并移除 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance", meta=(ClampMin=0.0, ForceUnits=s))
	float HitNotifyDuration = 0.4f;

	/** 默认命中标记贴图，用于每个屏幕空间命中点 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
	FSlateBrush PerHitMarkerImage;

	/** 按命中区域（HitZone）覆盖默认贴图，例如弱点命中显示不同颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
	TMap<FGameplayTag, FSlateBrush> PerHitMarkerZoneOverrideImages;

	/** 任意命中时显示的中心提示（可为空，不绘制） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
	FSlateBrush AnyHitsMarkerImage;

	/** HUD 在处理命中消息后调用，把命中点推送到 Slate 进行渲染 */
	UFUNCTION(BlueprintCallable, Category="HitMarker")
	void HandleHitNotification(const FShootReticleHitNotifyMessage& Message);

	/** 清空当前命中提示（例如切换武器或 HUD 重置时调用） */
	UFUNCTION(BlueprintCallable, Category="HitMarker")
	void ResetHitMarkers();

	/** 外部更新贴图/时长后调用，用于立即刷新 Slate 外观 */
	UFUNCTION(BlueprintCallable, Category="HitMarker")
	void RefreshAppearance();

private:
	/** Slate 渲染对象 */
	TSharedPtr<SShootHitMarkerConfirmationWidget> MarkerWidget;
};
