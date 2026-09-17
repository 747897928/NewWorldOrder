// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"

#include "ShootWardrobeInteractionComponent.generated.h"

class UCommonActivatableWidget;
class UGameplayAbility;
class APawn;

/**
 * 把现有场景家具接入项目统一交互链的轻量组件。
 *
 * Dressing_Table_Set 继续由蓝图维护网格、碰撞、提示图标和视觉表现；本组件只提供
 * IInteractableTarget 选项，并把衣柜页面推入发起交互的 LocalPlayer 对应 CommonUI 层。
 * 这样本地分屏玩家不会打开或关闭另一名玩家的衣柜，也不需要在家具蓝图里 Cast 具体角色。
 */
UCLASS(ClassGroup=(Interaction), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class NEWWORLDORDER_API UShootWardrobeInteractionComponent : public UActorComponent, public IInteractableTarget
{
	GENERATED_BODY()

public:
	UShootWardrobeInteractionComponent();

	virtual void GatherInteractionOptions(
		const FInteractionQuery& InteractQuery, FInteractionOptionBuilder& OptionBuilder) override;

	/** 由本地交互 GA 调用；页面只属于触发交互的 Pawn/LocalPlayer。 */
	bool OpenWardrobeScreenForPawn(APawn* InstigatorPawn) const;

protected:
	/** 具体 W_Cloth 资产由 BP_Dressing_Table_Set 配置，C++ 不引用 /Game 路径。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|UI")
	TSoftClassPtr<UCommonActivatableWidget> WardrobeScreenClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Interaction")
	FText InteractionText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Interaction")
	FText InteractionSubText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Wardrobe|Interaction")
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	/** 交互选项本身也受此范围限制，避免提示隐藏后仍可从远处按键触发。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wardrobe|Interaction", meta=(ClampMin="0"))
	float InteractionRange = 160.0f;
};
