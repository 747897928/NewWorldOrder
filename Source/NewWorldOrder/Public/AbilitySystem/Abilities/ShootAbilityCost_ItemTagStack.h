// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ShootAbilityCost.h"
#include "ScalableFloat.h"
#include "ShootAbilityCost_ItemTagStack.generated.h"

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpecHandle;

class UShootGameplayAbility;
class UObject;
struct FGameplayAbilityActorInfo;

/**
 * UShootAbilityCost_ItemTagStack
 *
 * 表示需要消耗物品实例上的 StatTags 堆栈作为成本
 * 参考 Lyra 的 LyraAbilityCost_ItemTagStack 实现
 *
 * 特点：
 * - CheckCost：客户端和服务器都读取复制的 StatTags 值
 * - ApplyCost：仅在服务器修改 StatTags（IsNetAuthority）
 * - 通过 GAS PredictionKey 机制处理预测，无需手动预测变量
 *
 * 用法：
 * 1. 在武器定义的 AbilitySet 中为开火/换弹 Ability 添加此 Cost
 * 2. 配置 Tag（如 Inventory.Ammo.Magazine）
 * 3. 配置 Quantity（每次消耗的数量）
 * 4. CommitAbility 时自动调用 CheckCost 和 ApplyCost
 */
UCLASS(meta=(DisplayName="Item Tag Stack"))
class NEWWORLDORDER_API UShootAbilityCost_ItemTagStack : public UShootAbilityCost
{
	GENERATED_BODY()

public:
	UShootAbilityCost_ItemTagStack();

	//~UShootAbilityCost interface
	virtual bool CheckCost(const UShootGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ApplyCost(const UShootGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	//~End of UShootAbilityCost interface

protected:
	/** 要消耗的 StatTag 堆栈数量（按 Ability 等级缩放） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
	FScalableFloat Quantity;

	/** 要消耗的 StatTag（如 Inventory.Ammo.Magazine） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
	FGameplayTag Tag;

	/** 当无法支付此成本时返回的失败 Tag */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
	FGameplayTag FailureTag;
};
