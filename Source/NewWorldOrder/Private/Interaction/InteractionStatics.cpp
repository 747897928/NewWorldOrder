// Copyright Epic Games, Inc. All Rights Reserved.

#include "Interaction/InteractionStatics.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/OverlapResult.h"
#include "Interaction/IInteractableTarget.h"
#include "UObject/ScriptInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InteractionStatics)

UInteractionStatics::UInteractionStatics()
	: Super(FObjectInitializer::Get())
{
}

// 从交互目标接口获取实际的Actor对象
AActor* UInteractionStatics::GetActorFromInteractableTarget(TScriptInterface<IInteractableTarget> InteractableTarget)
{
	if (UObject* Object = InteractableTarget.GetObject())
	{
		// 如果交互目标本身就是Actor，直接返回
		if (AActor* Actor = Cast<AActor>(Object))
		{
			return Actor;
		}
		// 如果交互目标是Actor组件，返回其所有者
		else if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
		{
			return ActorComponent->GetOwner();
		}
		else
		{
			// 其他情况暂未实现
			unimplemented();
		}
	}

	return nullptr;
}

// 从Actor获取所有可交互目标
void UInteractionStatics::GetInteractableTargetsFromActor(AActor* Actor, TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets)
{
	//TScriptInterface<IInteractableTarget> InteractableActor(Actor); 只是把已有的 Actor（UObject*）包装成接口类型；不会分配内存。
	//若 Actor 实现了 IInteractableTarget，包装器会保存 UObject* 和接口指针，否则为空。
	// 首先检查Actor本身是否实现了交互接口
	TScriptInterface<IInteractableTarget> InteractableActor(Actor);
	if (InteractableActor)
	{
		OutInteractableTargets.Add(InteractableActor);
	}

	// 然后检查Actor的所有组件是否实现了交互接口
	// If the actor isn't interactable, it might have a component that has a interactable interface.
	TArray<UActorComponent*> InteractableComponents = Actor ? Actor->GetComponentsByInterface(UInteractableTarget::StaticClass()) : TArray<UActorComponent*>();
	for (UActorComponent* InteractableComponent : InteractableComponents)
	{
		OutInteractableTargets.Add(TScriptInterface<IInteractableTarget>(InteractableComponent));
	}
}

// 从物理重叠结果中提取可交互目标
void UInteractionStatics::AppendInteractableTargetsFromOverlapResults(const TArray<FOverlapResult>& OverlapResults, TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets)
{
	for (const FOverlapResult& Overlap : OverlapResults)
	{
		// 场景家具通常由 PrimitiveComponent 提供碰撞、由独立 ActorComponent 提供业务交互。
		// 只检查实际重叠的碰撞组件会漏掉同一 Actor 上的交互组件，因此必须同时收集 Actor 的全部接口组件。
		TArray<TScriptInterface<IInteractableTarget>> ActorTargets;
		GetInteractableTargetsFromActor(Overlap.GetActor(), ActorTargets);
		for (const TScriptInterface<IInteractableTarget>& ActorTarget : ActorTargets)
		{
			OutInteractableTargets.AddUnique(ActorTarget);
		}

		// 仍保留对实际重叠组件的检查，兼容组件所有者为空或尚未注册到 Actor 组件数组的边界情况。
		TScriptInterface<IInteractableTarget> InteractableComponent(Overlap.GetComponent());
		if (InteractableComponent)
		{
			OutInteractableTargets.AddUnique(InteractableComponent);
		}
	}
}

// 从射线检测命中结果中提取可交互目标
void UInteractionStatics::AppendInteractableTargetsFromHitResult(const FHitResult& HitResult, TArray<TScriptInterface<IInteractableTarget>>& OutInteractableTargets)
{
	// 单线检测命中家具网格或碰撞体时，也要发现同一 Actor 上独立承载业务逻辑的交互组件。
	TArray<TScriptInterface<IInteractableTarget>> ActorTargets;
	GetInteractableTargetsFromActor(HitResult.GetActor(), ActorTargets);
	for (const TScriptInterface<IInteractableTarget>& ActorTarget : ActorTargets)
	{
		OutInteractableTargets.AddUnique(ActorTarget);
	}

	// 仍保留对实际命中组件的检查，AddUnique 会去掉已由 Actor 收集到的重复项。
	TScriptInterface<IInteractableTarget> InteractableComponent(HitResult.GetComponent());
	if (InteractableComponent)
	{
		OutInteractableTargets.AddUnique(InteractableComponent);
	}
}
