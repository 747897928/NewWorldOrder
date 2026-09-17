// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "InteractionOption.generated.h"

class IInteractableTarget;
class UUserWidget;


/**
 * 交互选项结构体
 * 描述一个具体的交互方式及其相关数据
 */
USTRUCT(BlueprintType)
struct FInteractionOption
{
	GENERATED_BODY()

public:
	/** The interactable target */
	/** 交互目标对象（实现IInteractableTarget接口的对象） */
	UPROPERTY(BlueprintReadWrite)
	TScriptInterface<IInteractableTarget> InteractableTarget;

	/** Simple text the interaction might return */
	/** 显示给用户的主要文本（如"开门"、"拾取"） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Text;

	/** Simple sub-text the interaction might return */
	/** 显示给用户的辅助文本（如"需要钥匙"） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SubText;

	/** 大于 0 时，交互主能力必须持续按住输入达到该秒数后才触发执行能力。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ForceUnits="s"))
	float HoldDuration = 0.0f;

	// METHODS OF INTERACTION
	//--------------------------------------------------------------

	// 1) Place an ability on the avatar that they can activate when they perform interaction.

	/** The ability to grant the avatar when they get near interactable objects. */
	// 交互方法1：授予玩家能力
	//--------------------------------------------------------------

	/** 
	 * 要授予给玩家的交互能力类
	 * 当玩家靠近可交互对象时，这个能力会被动态授予
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility> InteractionAbilityToGrant;

	// - OR -

	// 2) Allow the object we're interacting with to have its own ability system and interaction ability, that we can activate instead.

	/** The ability system on the target that can be used for the TargetInteractionHandle and sending the event, if needed. */
	// 交互方法2：在目标对象上触发能力
	//--------------------------------------------------------------

	/** 
	 * 目标对象的AbilitySystemComponent
	 * 用于在目标对象上直接触发交互能力
	 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UAbilitySystemComponent> TargetAbilitySystem = nullptr;

	/** The ability spec to activate on the object for this option. */
	/** 
	* 目标对象上交互能力的句柄
	* 用于在目标ASC中找到具体的能力实例
	*/
	UPROPERTY(BlueprintReadOnly)
	FGameplayAbilitySpecHandle TargetInteractionAbilityHandle;

	// UI
	//--------------------------------------------------------------

	/** The widget to show for this kind of interaction. */
	/** 为此交互类型显示的UI控件类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UUserWidget> InteractionWidgetClass;

	//--------------------------------------------------------------

public:
	// 比较运算符重载，用于选项去重和排序
	FORCEINLINE bool operator==(const FInteractionOption& Other) const
	{
		return InteractableTarget == Other.InteractableTarget &&
			InteractionAbilityToGrant == Other.InteractionAbilityToGrant&&
			TargetAbilitySystem == Other.TargetAbilitySystem &&
			TargetInteractionAbilityHandle == Other.TargetInteractionAbilityHandle &&
			InteractionWidgetClass == Other.InteractionWidgetClass &&
			FMath::IsNearlyEqual(HoldDuration, Other.HoldDuration) &&
			Text.IdenticalTo(Other.Text) &&
			SubText.IdenticalTo(Other.SubText);
	}

	FORCEINLINE bool operator!=(const FInteractionOption& Other) const
	{
		return !operator==(Other);
	}

	FORCEINLINE bool operator<(const FInteractionOption& Other) const
	{
		return InteractableTarget.GetInterface() < Other.InteractableTarget.GetInterface();
	}
};
