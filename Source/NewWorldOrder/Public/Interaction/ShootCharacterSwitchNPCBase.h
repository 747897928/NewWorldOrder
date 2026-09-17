// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Interaction/IShootCharacterSwitchEntry.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/ShootInteractionTypes.h"
#include "Character/CharacterGender.h"
#include "ShootCharacterSwitchNPCBase.generated.h"

class UGameplayAbility;

/**
 * 另一位主角 NPC 的最小 C++ 骨架。
 * 这个基类只解决“世界入口如何暴露给交互系统”：
 * - 实现 IInteractableTarget，提供提示与交互选项
 * - 实现 IShootCharacterSwitchEntry，告诉桥接能力当前应该切到谁
 * - 不承载角色切换业务本体，真正切换仍统一走 PlayerController / PlayerState 后端
 *
 * 蓝图子类至少需要关注两件事：
 * 1. 把 RepresentedGender 设成这个 NPC 代表的主角性别
 * 2. 配置外观、待机、表情、剧情资源等表现层内容
 */
UCLASS(Abstract)
class NEWWORLDORDER_API AShootCharacterSwitchNPCBase : public ACharacter, public IInteractableTarget, public IShootCharacterSwitchEntry
{
	GENERATED_BODY()

public:
	AShootCharacterSwitchNPCBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// IInteractableTarget
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	                                      FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
	                                           FGameplayEventData& InOutEventData) override;

	// IShootCharacterSwitchEntry
	virtual bool ResolveSwitchTargetGender(const APawn* RequestingPawn, ECharacterGender& OutTargetGender) const override;

protected:
	/** 这个 NPC 当前代表哪个主角。若与请求者当前主角相同，则不再暴露切换选项。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Identity")
	ECharacterGender RepresentedGender = ECharacterGender::UNKNOWN;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	EShootInteractionTriggerMode TriggerMode = EShootInteractionTriggerMode::PressToInteract;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	EShootInteractionUserFilter UserFilter = EShootInteractionUserFilter::PlayerOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	FText InteractionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	FText InteractionSubText;

	/** 默认桥接到世界入口请求能力；蓝图若确有必要，可换成兼容同一后端协议的子类。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	virtual FText BuildDefaultInteractionText(ECharacterGender TargetGender) const;
	virtual FText BuildDefaultInteractionSubText() const;

private:
	bool CanBeTriggeredBy(const APawn* Pawn) const;
	bool IsPlayerActor(const APawn* Pawn) const;
	ECharacterGender ResolveRequesterCurrentGender(const APawn* RequestingPawn) const;
};
