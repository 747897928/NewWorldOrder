// Copyright ZhaoYiJie

#include "AbilitySystem/Abilities/ShootAbilityCost_ItemTagStack.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "Equipment/ShootEquipmentInstance.h"
#include "Inventory/ShootInventoryItemInstance.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAbilityCost_ItemTagStack)

UShootAbilityCost_ItemTagStack::UShootAbilityCost_ItemTagStack()
{
	Quantity.SetValue(1.0f);

	// 构造阶段禁止访问 FShootGameplayTags::Get()，改用 RequestGameplayTag 兜底
	const FGameplayTag AbilityCostFailTag = FGameplayTag::RequestGameplayTag(
		FName("Ability.ActivateFail.Cost"), /*ErrorIfNotFound*/false);
	if (AbilityCostFailTag.IsValid())
	{
		FailureTag = AbilityCostFailTag;
	}
}

bool UShootAbilityCost_ItemTagStack::CheckCost(const UShootGameplayAbility* Ability,
                                                 const FGameplayAbilitySpecHandle Handle,
                                                 const FGameplayAbilityActorInfo* ActorInfo,
                                                 FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Ability || !ActorInfo)
	{
		return false;
	}

	// 从 Ability 的 SourceObject 获取装备实例
	// 参考 Lyra 的 ULyraGameplayAbility_FromEquipment::GetAssociatedItem
	UShootEquipmentInstance* EquipmentInstance = Cast<UShootEquipmentInstance>(Ability->GetSourceObject(Handle, ActorInfo));
	if (!EquipmentInstance)
	{
		return false;
	}

	// 从装备实例获取关联的物品实例（其中存储了 StatTags）
	UShootInventoryItemInstance* ItemInstance = Cast<UShootInventoryItemInstance>(EquipmentInstance->GetInstigator());
	if (!ItemInstance)
	{
		return false;
	}

	// 计算需要的堆栈数量（按 Ability 等级缩放）
	const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
	const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
	const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

	// 检查是否有足够的堆栈
	// 注意：CheckCost 在客户端和服务器都会调用
	// 客户端读取的是复制的 StatTags 值（可能有轻微延迟）
	// 服务器读取的是权威值
	const bool bCanApplyCost = ItemInstance->GetStatTagStackCount(Tag) >= NumStacks;

	// 如果无法支付成本，添加失败 Tag（供其他 Ability 判断）
	if (!bCanApplyCost && OptionalRelevantTags && FailureTag.IsValid())
	{
		OptionalRelevantTags->AddTag(FailureTag);
	}

	return bCanApplyCost;
}

void UShootAbilityCost_ItemTagStack::ApplyCost(const UShootGameplayAbility* Ability,
                                                 const FGameplayAbilitySpecHandle Handle,
                                                 const FGameplayAbilityActorInfo* ActorInfo,
                                                 const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 重点：仅在服务器执行（参考 Lyra 的实现）
	// 这是 Lyra 方案 A 的核心：客户端不修改 StatTags，只读取复制值
	// GAS 的 PredictionKey 机制负责 Ability 激活的预测，而不是弹药数值的预测
	if (ActorInfo->IsNetAuthority())
	{
		UShootEquipmentInstance* EquipmentInstance = Cast<UShootEquipmentInstance>(Ability->GetSourceObject(Handle, ActorInfo));
		if (EquipmentInstance)
		{
			UShootInventoryItemInstance* ItemInstance = Cast<UShootInventoryItemInstance>(EquipmentInstance->GetInstigator());
			if (ItemInstance)
			{
				const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
				const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
				const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

				// 服务器权威扣减
				ItemInstance->RemoveStatTagStack(Tag, NumStacks);

				// StatTags 会通过网络复制到客户端
				// 客户端在收到复制后更新 UI（有网络延迟，通常 50-150ms）
			}
		}
	}
	// 客户端不执行任何操作
	// 客户端依赖 StatTags 的复制来更新本地状态
}
