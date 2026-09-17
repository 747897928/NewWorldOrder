// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShootAnimLayerSelectionSet.generated.h"

class UAnimInstance;

/** 一条按外观标签选择 AnimLayer 的规则；数组顺序即匹配优先级。 */
USTRUCT(BlueprintType)
struct FShootAnimLayerSelectionEntry
{
	GENERATED_BODY()

	/** RequiredTags 全部满足时选用的动画层。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> Layer;

	/** 角色外观必须同时具有的标签；空容器可作为高优先级通用规则。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(Categories="Cosmetic"))
	FGameplayTagContainer RequiredTags;
};

/**
 * 项目版 Lyra AnimLayer 选择集。
 *
 * 这里保存的是“从多个候选层中选择一个”的规则，不是把数组中的类全部 Link。
 * 调用链：WeaponInstance 装备/卸载 -> AShootCharacter::ApplyWeaponPresentation
 * -> SelectBestLayer -> 正式 CC Mesh 主 AnimInstance 的 LinkAnimClassLayers。
 * 武器层使用项目专用接口，与 Hair/Shoe 接口隔离，因此只替换当前武器姿势实现。
 */
USTRUCT(BlueprintType)
struct FShootAnimLayerSelectionSet
{
	GENERATED_BODY()

	/** 首条 RequiredTags 全部匹配的规则获胜。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty=Layer))
	TArray<FShootAnimLayerSelectionEntry> LayerRules;

	/** 没有规则匹配时使用的层，通常配置男性层。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> DefaultLayer;

	/** 按 Lyra 的首条命中语义选出唯一动画层。 */
	TSubclassOf<UAnimInstance> SelectBestLayer(const FGameplayTagContainer& CosmeticTags) const;
};
