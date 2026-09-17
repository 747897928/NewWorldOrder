// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "ShootInteractionTypes.generated.h"

class UGameplayEffect;

/**
 * 枚举交互触发模式：
 * - AutoOverlap：进入碰撞体立即触发（沿用 AShootResourcePickup 旧逻辑）
 * - PressToInteract：交由 UShootGA_Interact 监听按键
 * - AIScripted：行为树/脚本显式调用，不依赖输入
 */
UENUM(BlueprintType)
enum class EShootInteractionTriggerMode : uint8
{
	None UMETA(DisplayName="None / 禁用"),
	AutoOverlap UMETA(DisplayName="AutoOverlap / 自动拾取"),
	PressToInteract UMETA(DisplayName="PressToInteract / 按键交互"),
	AIScripted UMETA(DisplayName="AIScripted / AI脚本触发")
};

/**
 * 枚举交互对象可被哪些使用者触发：
 * - PlayerOnly：仅玩家
 * - AIOnly：仅AI/敌人
 * - PlayerAndAI：双方都可
 */
UENUM(BlueprintType)
enum class EShootInteractionUserFilter : uint8
{
	PlayerOnly UMETA(DisplayName="PlayerOnly / 仅玩家"),
	AIOnly UMETA(DisplayName="AIOnly / 仅AI"),
	PlayerAndAI UMETA(DisplayName="PlayerAndAI / 玩家与AI")
};

/**
 * 阵营驱动的拾取/交互效果配置
 *
 * 数据驱动，避免在C++里写 if(IsPlayer)。
 * Instigator 的 ASC 只要带有 Faction Tag，即可根据该表选择合适的 GameplayEffect。
 */
USTRUCT(BlueprintType)
struct FShootFactionEffectEntry
{
	GENERATED_BODY()

	/** 需要匹配的阵营Tag，例如 Faction.Player / Faction.Enemy */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	FGameplayTag FactionTag;

	/** 对应的 GameplayEffect（作用在 Instigator 身上） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
	TSubclassOf<UGameplayEffect> EffectClass;

	FShootFactionEffectEntry()
		: FactionTag()
		, EffectClass(nullptr)
	{
	}
};
