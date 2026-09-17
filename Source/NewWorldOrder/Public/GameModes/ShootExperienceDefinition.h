// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/ShootAbilitySet.h"
#include "Camera/ShootCameraTypes.h"

#include "ShootExperienceDefinition.generated.h"

class UUserWidget;
class UShootSkillLoadoutConfig;

/**
 * 对齐 Lyra FLyraHUDLayoutRequest：声明当前玩法需要推入 CommonUI 层级的 HUD 根布局。
 */
USTRUCT(BlueprintType)
struct FShootHUDLayoutRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSoftClassPtr<UCommonActivatableWidget> LayoutClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI", meta=(Categories="UI.Layer"))
	FGameplayTag LayerID;
};

/**
 * 对齐 Lyra FLyraHUDElementEntry：声明当前玩法向 HUD 插槽注入的稳定片段。
 */
USTRUCT(BlueprintType)
struct FShootHUDElementEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSoftClassPtr<UUserWidget> WidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI", meta=(Categories="HUD.Slot"))
	FGameplayTag SlotID;
};

/**
 * 一局玩法的项目层 Experience 定义。
 *
 * 当前已收敛模式 HUD、AbilitySet 和相机视角策略；后续 PawnData 与规则组件继续追加到同一个资产，
 * 不再由 PlayerController 构造函数决定“这个模式应该显示什么 UI”。
 */
UCLASS(BlueprintType, Const)
class NEWWORLDORDER_API UShootExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TArray<FShootHUDLayoutRequest> HUDLayouts;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI")
	TArray<FShootHUDElementEntry> HUDWidgets;

	/** 当前性别主角的 主动+被动 技能套件(AbilitySet DA)。技能为副本数据: 进入副本授予, 出副本取回, 存档不持有。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	TObjectPtr<UShootAbilitySet> MaleProtagonistAbilitySet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	TObjectPtr<UShootAbilitySet> FemaleProtagonistAbilitySet;

	/** 不区分性别的玩法能力，例如视角切换；与当前 Experience 共生共灭。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities")
	TObjectPtr<UShootAbilitySet> CommonAbilitySet;

	/** 本 Experience 的四槽 Match Skill 输入与随机池；为空时不启用随机技能获取。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Abilities|Match Skills")
	TObjectPtr<const UShootSkillLoadoutConfig> SkillLoadoutConfig;

	/** 进入本 Experience 时每个 LocalPlayer 的默认基础视角。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
	EShootCameraPerspective DefaultCameraPerspective = EShootCameraPerspective::ThirdPerson;

	/** 关闭时即使输入资产仍有映射，视角切换 GA 也不会激活。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Camera")
	bool bAllowCameraPerspectiveSwitch = false;

	/**
	 * 是否允许本 Experience 的角色进入 CharacterMovement 内置 MOVE_Swimming。
	 * 水体体积仍由地图配置；这个开关只负责让玩法生命周期决定该移动能力是否有效。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Movement|Swimming")
	bool bAllowSwimming = false;
};
