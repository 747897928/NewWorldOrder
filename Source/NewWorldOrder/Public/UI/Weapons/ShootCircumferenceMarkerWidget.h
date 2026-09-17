#pragma once

#include "Components/Widget.h"
#include "Styling/SlateBrush.h"
#include "ShootCircumferenceMarkerWidget.generated.h"

class SShootCircumferenceMarkerWidget;

USTRUCT(BlueprintType)
struct FShootCircumferenceMarkerEntry
{
	GENERATED_BODY()

	/** 将该标记放置在圆周上的角度（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ForceUnits=deg))
	float PositionAngle = 0.0f;

	/** 标记图像自身的旋转角度（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ForceUnits=deg))
	float ImageRotationAngle = 0.0f;
};

/**
 * 圆周型准星标记 Widget：支持在圆周上绘制角标 / reticle corner
 */
UCLASS()
class UShootCircumferenceMarkerWidget : public UWidget
{
	GENERATED_BODY()

public:
	UShootCircumferenceMarkerWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UWidget interface
	virtual void SynchronizeProperties() override;
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	//~End of UWidget interface

	//~UVisual interface
public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	//~End of UVisual interface

public:
	/** 圆周上每个标记的角度与朝向 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
	TArray<FShootCircumferenceMarkerEntry> MarkerList;

	/** 圆周半径（像素） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance", meta=(ClampMin=0.0))
	float Radius = 48.0f;

	/** 角标使用的纹理（Slate Brush） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Appearance")
	FSlateBrush MarkerImage;

	/** 角标是否绘制在散布半径外侧 */
	UPROPERTY(EditAnywhere, Category="Corner")
	uint8 bReticleCornerOutsideSpreadRadius : 1 = false;

public:
	// 蓝图集成提示：
	// 1. HUD 或 Reticle 蓝图可在 Tick/消息回调中调用 SetRadius，传入 UShootReticleWidgetBase::ComputeMaxScreenspaceSpreadRadius()。
	// 2. 若需要自定义角标数量，可在武器切换时调用 SetMarkerList 替换默认布局。
	// 3. 关键半径计算已在 C++ 实现，蓝图只负责把值传到控件，避免公式分散。
	/** 更新圆周半径（运行时） */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetRadius(float InRadius);

	/** 更新角标列表（运行时） */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetMarkerList(const TArray<FShootCircumferenceMarkerEntry>& InMarkerList);

	/** 切换角标是否绘制在散布半径外侧 */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetReticleCornerOutsideSpreadRadius(bool bOutside);

private:
	/** Slate 渲染对象 */
	TSharedPtr<SShootCircumferenceMarkerWidget> MarkerWidget;
};
