// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IShootCharacterSwitchEntry.h"
#include "Interaction/IInteractableTarget.h"
#include "Interaction/InteractionQuery.h"
#include "Interaction/ShootInteractionTypes.h"
#include "ShootCharacterSwitchEntryActorBase.generated.h"

class UGameplayAbility;

/**
 * 世界内角色切换入口 Actor 基类。
 * 这个基类只收口“入口提示、触发过滤、事件数据包装”这些共性逻辑。
 * 真正切换到谁仍由派生类或其他实现者通过 IShootCharacterSwitchEntry 决定。
 *
 * 注意：
 * - 适用于“站点 / 切换点 / 专用入口 Actor”。
 * - 如果未来入口本体已经继承了别的父类，例如 ACharacter 版另一位主角 NPC，
 *   那么不要硬改继承链，直接实现 IShootCharacterSwitchEntry 即可。
 *   当前仓库已经提供了 `AShootCharacterSwitchNPCBase` 作为 NPC 入口骨架。
 */
UCLASS(Abstract)
class NEWWORLDORDER_API AShootCharacterSwitchEntryActorBase : public AActor, public IInteractableTarget, public IShootCharacterSwitchEntry
{
	GENERATED_BODY()

public:
	AShootCharacterSwitchEntryActorBase();

	// IInteractableTarget
	virtual void GatherInteractionOptions(const FInteractionQuery& InteractQuery,
	                                      FInteractionOptionBuilder& OptionBuilder) override;
	virtual void CustomizeInteractionEventData(const FGameplayTag& InteractionEventTag,
	                                           FGameplayEventData& InOutEventData) override;
	virtual bool ResolveSwitchTargetGender(const APawn* RequestingPawn, ECharacterGender& OutTargetGender) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	EShootInteractionTriggerMode TriggerMode = EShootInteractionTriggerMode::PressToInteract;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	EShootInteractionUserFilter UserFilter = EShootInteractionUserFilter::PlayerOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	FText InteractionText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	FText InteractionSubText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CharacterSwitch|Interaction")
	TSubclassOf<UGameplayAbility> InteractionAbilityClass;

	// 派生类可以覆写文案格式，例如 NPC 入口和站点入口可能想展示不同文本。
	virtual FText BuildDefaultInteractionText(ECharacterGender TargetGender) const;
	virtual FText BuildDefaultInteractionSubText() const;

private:
	bool CanBeTriggeredBy(const APawn* Pawn) const;
	bool IsPlayerActor(const APawn* Pawn) const;
};
