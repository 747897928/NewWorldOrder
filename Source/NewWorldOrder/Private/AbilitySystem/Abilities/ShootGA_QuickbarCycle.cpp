// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Abilities/ShootGA_QuickbarCycle.h"

#include "Equipment/ShootQuickBarComponent.h"
#include "GameFramework/Controller.h"
#include "Player/ShootPlayerController.h"

UShootGA_QuickbarCycle::UShootGA_QuickbarCycle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 切槽会创建/卸载 Pawn 装备，必须由服务器执行，再通过现有槽位复制刷新所有客户端表现。
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UShootGA_QuickbarCycle::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (ActorInfo)
	{
		// ASC 固定在 PlayerState，而 QuickBar 固定在 PlayerController；通过当前 Avatar 的 Controller 连接两层，
		// 不使用 GetFirstPlayerController，因此本地双人和 Listen Server 客户端各自只切自己的会话武器。
		if (const AShootPlayerController* PlayerController = Cast<AShootPlayerController>(ActorInfo->PlayerController.Get()))
		{
			if (UShootQuickBarComponent* QuickBar = PlayerController->GetGameplayQuickBarComponent())
			{
				QuickBar->CycleRuntimeSlot(bCycleForward);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
