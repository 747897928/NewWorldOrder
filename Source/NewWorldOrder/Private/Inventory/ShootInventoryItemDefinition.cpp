// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Inventory/ShootInventoryItemDefinition.h"

#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Player/ShootPlayerState.h"
#include "Templates/SubclassOf.h"
#include "UObject/ObjectPtr.h"
#include "Inventory/ShootInventoryManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootInventoryItemDefinition)

//////////////////////////////////////////////////////////////////////
// UShootInventoryItemDefinition

UShootInventoryItemDefinition::UShootInventoryItemDefinition(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/**
 * 查找指定类型的 Fragment 实现
 *
 * 执行流程：
 *   1. 验证 FragmentClass 不为 nullptr
 *   2. 遍历 Fragments 数组
 *   3. 检查每个 Fragment 是否为指定类型（或子类）
 *   4. 返回第一个匹配的 Fragment
 *   5. 如果未找到，返回 nullptr
 *
 * 性能：
 *   - O(n)，n = Fragment 数量
 *   - 通常 Fragment 数量很少（<= 5），性能可接受
 *
 * 注意：
 *   - 使用 IsA 检查类型，支持子类匹配
 *   - 返回第一个匹配项（如果有多个同类型 Fragment，只返回第一个）
 */
const UShootInventoryItemFragment* UShootInventoryItemDefinition::FindFragmentByClass(TSubclassOf<UShootInventoryItemFragment> FragmentClass) const
{
	if (FragmentClass != nullptr)
	{
		for (UShootInventoryItemFragment* Fragment : Fragments)
		{
			if (Fragment && Fragment->IsA(FragmentClass))
			{
				return Fragment;
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////
// UShootInventoryFunctionLibrary

/**
 * 蓝图函数：查找物品定义的指定 Fragment
 *
 * 实现细节：
 *   1. 验证 ItemDef 和 FragmentClass 不为 nullptr
 *   2. 通过 GetDefault 获取 ItemDef 的 CDO（Class Default Object）
 *      - CDO：每个类的默认对象，所有实例共享
 *      - 对于 Const 的 Definition，CDO 包含所有配置的数据
 *   3. 调用 CDO 的 FindFragmentByClass 查找 Fragment
 *   4. 返回结果
 *
 * 为什么使用 GetDefault：
 *   - Definition 是 Const，所有实例共享同一份数据
 *   - CDO 包含在编辑器中配置的所有数据（DisplayName、Fragments 等）
 *   - 避免创建不必要的实例
 *
 * 蓝图使用：
 *   ```
 *   FindItemDefinitionFragment(BP_Item_AK47, BP_Fragment_Weapon)
 *   -> WeaponFragment（类型自动推导）
 *   ```
 */
const UShootInventoryItemFragment* UShootInventoryFunctionLibrary::FindItemDefinitionFragment(TSubclassOf<UShootInventoryItemDefinition> ItemDef, TSubclassOf<UShootInventoryItemFragment> FragmentClass)
{
	if ((ItemDef != nullptr) && (FragmentClass != nullptr))
	{
		return GetDefault<UShootInventoryItemDefinition>(ItemDef)->FindFragmentByClass(FragmentClass);
	}
	return nullptr;
}

UShootInventoryManagerComponent* UShootInventoryFunctionLibrary::GetInventoryManager(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	if (const UShootInventoryManagerComponent* Manager = Cast<UShootInventoryManagerComponent>(WorldContextObject))
	{
		return const_cast<UShootInventoryManagerComponent*>(Manager);
	}

	const AActor* Actor = Cast<AActor>(WorldContextObject);
	if (!Actor)
	{
		if (const UActorComponent* Component = Cast<UActorComponent>(WorldContextObject))
		{
			Actor = Component->GetOwner();
		}
	}

	const AShootPlayerState* ShootPlayerState = nullptr;

	if (Actor)
	{
		if (const AShootPlayerState* AsPlayerState = Cast<AShootPlayerState>(Actor))
		{
			ShootPlayerState = AsPlayerState;
		}
		else if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			ShootPlayerState = Pawn->GetPlayerState<AShootPlayerState>();
			if (!ShootPlayerState)
			{
				if (const AController* Controller = Pawn->GetController())
				{
					ShootPlayerState = Controller->GetPlayerState<AShootPlayerState>();
				}
			}
		}
		else if (const AController* Controller = Cast<AController>(Actor))
		{
			ShootPlayerState = Controller->GetPlayerState<AShootPlayerState>();
		}
		else if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			ShootPlayerState = Cast<AShootPlayerState>(PlayerState);
		}
	}
	else
	{
		ShootPlayerState = Cast<AShootPlayerState>(WorldContextObject);
	}

	return ShootPlayerState ? ShootPlayerState->GetInventoryManagerComponent() : nullptr;
}
