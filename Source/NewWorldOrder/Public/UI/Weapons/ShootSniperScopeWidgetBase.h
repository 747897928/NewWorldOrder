#pragma once

#include "UI/Weapons/ShootReticleWidgetBase.h"
#include "ShootSniperScopeWidgetBase.generated.h"

class UWidgetAnimation;

/**
 * 狙击镜仍由 Reticle Host 按当前 LocalPlayer 创建，不走 AddToViewport 或 Character 持有引用。
 * W_SniperScope 蓝图只需继承本类并保留名为 ScopeZoomIn 的动画；进入 ADS 正放，退出反放后隐藏。
 */
UCLASS(Abstract, Blueprintable)
class NEWWORLDORDER_API UShootSniperScopeWidgetBase : public UShootReticleWidgetBase
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void OnReticleADSVisualChanged_Implementation(bool bIsADS) override;

	UFUNCTION()
	void HandleScopeAnimationFinished();

	UPROPERTY(Transient, meta=(BindWidgetAnimOptional))
	TObjectPtr<UWidgetAnimation> ScopeZoomIn;

private:
	bool bHideWhenAnimationFinishes = false;
};
