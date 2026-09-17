// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Abilities/ShootGA_DropWeapon.h"

#include "Equipment/ShootQuickBarComponent.h"
#include "Player/ShootPlayerController.h"
#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_DropWeapon)

UShootGA_DropWeapon::UShootGA_DropWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// InputTag.Weapon.Drop 是 Native Tag，注册时机晚于该原生 CDO 构造。
	// `/Game/Weapons/Quickbar/GA_DropWeapon` 蓝图子类负责配置 StartupInputTag；
	// 本类只保留网络权威业务，避免构造期请求尚未注册的 Tag 得到空值。
}

void UShootGA_DropWeapon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (ActorInfo)
	{
		// ASC 位于 PlayerState，QuickBar 位于拥有输入权的 PlayerController。
		// 通过 ActorInfo 连接两者，不使用 Player 0 或全局 Controller 查询。
		if (const AShootPlayerController* PlayerController =
			Cast<AShootPlayerController>(ActorInfo->PlayerController.Get()))
		{
			if (UShootQuickBarComponent* QuickBar = PlayerController->GetGameplayQuickBarComponent())
			{
				QuickBar->DropActiveRuntimeWeapon();
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
