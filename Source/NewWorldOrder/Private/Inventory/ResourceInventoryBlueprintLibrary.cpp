// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Inventory/ResourceInventoryBlueprintLibrary.h"

#include "Inventory/ResourceInventoryComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Inventory/ShootInventoryItemDefinition.h"
#include "Player/ShootPlayerState.h"

namespace
{
	/** 内部工具：从任意对象获取 PlayerState（包含 Spectator/远程客户端） */
	const AShootPlayerState* ResolveShootPlayerState(const UObject* WorldContextObject)
	{
		if (!WorldContextObject)
		{
			return nullptr;
		}

		// 直接是 PlayerState
		if (const AShootPlayerState* PS = Cast<AShootPlayerState>(WorldContextObject))
		{
			return PS;
		}

		// 是 Pawn
		if (const APawn* Pawn = Cast<APawn>(WorldContextObject))
		{
			if (const AShootPlayerState* PS = Pawn->GetPlayerState<AShootPlayerState>())
			{
				return PS;
			}
			if (const AController* PC = Pawn->GetController())
			{
				if (const AShootPlayerState* CPS = PC->GetPlayerState<AShootPlayerState>())
				{
					return CPS;
				}
			}
		}

		// 是 Controller
		if (const AController* PC = Cast<AController>(WorldContextObject))
		{
			if (const AShootPlayerState* PS = PC->GetPlayerState<AShootPlayerState>())
			{
				return PS;
			}
		}

		// 是组件或其他 UObject，尝试取 Outer
		if (const UActorComponent* Component = Cast<UActorComponent>(WorldContextObject))
		{
			if (const AActor* Owner = Component->GetOwner())
			{
				return ResolveShootPlayerState(Owner);
			}
		}

		return nullptr;
	}
}

UResourceInventoryComponent* UResourceInventoryBlueprintLibrary::GetResourceInventory(const UObject* WorldContextObject)
{
	const AShootPlayerState* PS = ResolveShootPlayerState(WorldContextObject);
	return PS ? PS->GetResourceInventoryComponent() : nullptr;
}

bool UResourceInventoryBlueprintLibrary::AddResource(const UObject* WorldContextObject, TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 Delta)
{
	if (UResourceInventoryComponent* ResourceComp = GetResourceInventory(WorldContextObject))
	{
		// 账号层资源仅允许服务器修改，拾取/奖励统一入口
		return ResourceComp->AddResource(ItemDef, Delta);
	}
	return false;
}

bool UResourceInventoryBlueprintLibrary::ConsumeResource(const UObject* WorldContextObject, TSubclassOf<UShootInventoryItemDefinition> ItemDef, int32 Delta)
{
	if (UResourceInventoryComponent* ResourceComp = GetResourceInventory(WorldContextObject))
	{
		return ResourceComp->ConsumeResource(ItemDef, Delta);
	}
	return false;
}

int32 UResourceInventoryBlueprintLibrary::GetResourceCount(const UObject* WorldContextObject, TSubclassOf<UShootInventoryItemDefinition> ItemDef)
{
	if (UResourceInventoryComponent* ResourceComp = GetResourceInventory(WorldContextObject))
	{
		return ResourceComp->GetResourceCount(ItemDef);
	}
	return 0;
}

void UResourceInventoryBlueprintLibrary::GetAllResources(const UObject* WorldContextObject, TArray<FResourceEntry>& OutEntries)
{
	if (UResourceInventoryComponent* ResourceComp = GetResourceInventory(WorldContextObject))
	{
		ResourceComp->GetAllResources(OutEntries);
		return;
	}
	OutEntries.Reset();
}

bool UResourceInventoryBlueprintLibrary::HasEnoughResource(const UObject* WorldContextObject, const FResourceCost& ResourceCost)
{
	if (const UResourceInventoryComponent* ResourceComp = GetResourceInventory(WorldContextObject))
	{
		return ResourceComp->HasEnoughResource(ResourceCost);
	}
	return false;
}
