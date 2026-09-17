#pragma once

#include "CommonUserWidget.h"
#include "ShootReticleHostWidget.generated.h"

class SOverlay;
class UShootRangedWeaponInstance;
class UShootReticleWidgetBase;
struct FGeometry;

/**
 * Lyra WeaponUserInterface 的项目层等价物。
 *
 * UIExtension 只创建这一枚稳定 Host；Rifle/Pistol/Shotgun 准星作为它的唯一子 Widget 切换。
 * Host 从所属 LocalPlayer 的 QuickBar 读取当前实例，因此本地分屏互不串线；Listen Server 的
 * 远端 Controller 不会注册 Host，远端客户端只在自己的本地 Controller 上创建玩家私有准星。
 */
UCLASS()
class UShootReticleHostWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UShootReticleHostWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	//~UUserWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~End of UUserWidget interface

private:
	void RefreshFromOwningQuickBar();
	TSubclassOf<UShootReticleWidgetBase> ResolveReticleClass(
		const UShootRangedWeaponInstance* WeaponInstance) const;

	UPROPERTY(Transient)
	TObjectPtr<UShootRangedWeaponInstance> CurrentWeaponInstance;

	UPROPERTY(Transient)
	TObjectPtr<UShootReticleWidgetBase> ActiveReticleWidget;

	TSharedPtr<SOverlay> ReticleOverlay;
};
