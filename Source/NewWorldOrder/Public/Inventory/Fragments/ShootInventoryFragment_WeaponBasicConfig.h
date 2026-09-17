// 武器基础配置：只保留身份、准星、弹药 UI 和掉落表现入口。
#pragma once

#include "CoreMinimal.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Styling/SlateBrush.h"
#include "Templates/SubclassOf.h"
#include "ShootInventoryFragment_WeaponBasicConfig.generated.h"

class AShootWeaponPickupActor;
class UShootReticleWidgetBase;

UCLASS(BlueprintType)
class UShootInventoryFragment_WeaponBasicConfig : public UShootInventoryItemFragment
{
	GENERATED_BODY()

public:
	/**
	 * 武器基础片段只承载身份和运行时入口：WeaponId、准星、弹药图标、掉落表现蓝图。
	 * 物品名称和图标统一使用 ItemDefinition 根部的 DisplayName/Icon；拾取 Actor 只负责表现壳，
	 * AmmoIcon 表达的是弹种而不是物品卡片，因此由 BasicConfig 独立配置并随 QuickBar 消息下发。
	 */
	/** 武器标识（可用于拾取/日志/GameplayMessage） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	FName WeaponId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|UI")
	TSubclassOf<UShootReticleWidgetBase> ReticleWidgetClass;

	/**
	 * QuickBar 槽位与当前武器弹药栏共用的弹种遮罩纹理来源；武器卡片图标仍使用 ItemDefinition.Icon。
	 * 这里沿用 FSlateBrush 以便像 Lyra 的 InventoryFragment_QuickBarIcon.AmmoBrush 一样直接配置纹理，
	 * 但运行时不会把整个 Brush 覆盖到 UImage：Widget 保留自身材质和布局，只读取 ResourceObject。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|UI")
	FSlateBrush AmmoIcon;

	/**
	 * 该武器丢弃时生成的可交互表现蓝图。
	 * ItemDefinition 负责选择 Rifle/Pistol/Shotgun 各自外观；CombatComponent 只执行服务器权威生成。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon|Pickup")
	TSubclassOf<AShootWeaponPickupActor> DroppedPickupActorClass;
};
