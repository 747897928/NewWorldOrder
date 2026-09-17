#pragma once

#include "CommonUserWidget.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "UI/Weapons/ShootHitMarkerTypes.h"
#include "ShootReticleWidgetBase.generated.h"

class UShootInventoryItemInstance;
class UShootRangedWeaponInstance;

/**
 * 项目版 LyraReticleWidgetBase。
 *
 * C++ 只提供武器数据、Lyra 散布投影公式，以及按 OwningPlayer 隔离的项目消息入口。
 * Rifle/Pistol/Shotgun 的尺寸、材质、Marker、Tick 更新与动画都保留在各自 Widget Blueprint，
 * 不在这里绑定具体 UMG 控件或生成 fallback 控件树。
 */
UCLASS(Abstract, Blueprintable)
class UShootReticleWidgetBase : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UShootReticleWidgetBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 与 Lyra 同名入口：设置 Weapon/Inventory 实例后通知蓝图。 */
	UFUNCTION(BlueprintCallable, Category="Reticle")
	void InitializeFromWeapon(UShootRangedWeaponInstance* InWeapon);

	UFUNCTION(BlueprintPure, Category="Reticle")
	float ComputeSpreadAngle() const;

	UFUNCTION(BlueprintPure, Category="Reticle")
	float ComputeMaxScreenspaceSpreadRadius() const;

	UFUNCTION(BlueprintPure, Category="Reticle")
	bool HasFirstShotAccuracy() const;

protected:
	//~UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~End of UUserWidget interface

	/** 与 Lyra 一致：武器和 Inventory 引用准备完成后由具体准星蓝图读取。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Reticle")
	void OnWeaponInitialized();

	/** 项目分屏消息适配：C++ 只做玩家归属过滤和世界坐标投影，具体 HitMarker 控件仍由蓝图调用。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Reticle|Visual")
	void OnReticleHitNotification(const FShootReticleHitNotifyMessage& Message);

	/** 项目消息已按 Owning Pawn 过滤；具体 AimDownSights 动画由武器准星蓝图实现。 */
	UFUNCTION(BlueprintNativeEvent, Category="Reticle|Visual")
	void OnReticleADSVisualChanged(bool bIsADS);

	/** 项目消息已按 Owning Pawn 过滤；具体 Elimination 动画由武器准星蓝图实现。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Reticle|Visual")
	void OnReticleEliminationVisual();

	void HandleHitNotificationMessage(FGameplayTag Channel, const FShootReticleHitNotifyMessage& Message);
	void HandleAdsMessage(FGameplayTag Channel, const FShootReticleADSMessage& Message);
	void HandleEliminationMessage(FGameplayTag Channel, const FShootReticleEliminationMessage& Message);

protected:
	UPROPERTY(BlueprintReadOnly, Category="Reticle")
	TObjectPtr<UShootRangedWeaponInstance> WeaponInstance;

	UPROPERTY(BlueprintReadOnly, Category="Reticle")
	TObjectPtr<UShootInventoryItemInstance> InventoryInstance;

	FGameplayMessageListenerHandle HitNotifyHandle;
	FGameplayMessageListenerHandle AdsHandle;
	FGameplayMessageListenerHandle EliminationHandle;
};
