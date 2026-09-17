// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Interaction/Abilities/ShootGA_Interaction_RefillAmmo.h"

#include "Equipment/ShootQuickBarComponent.h"
#include "Interaction/ShootAmmoSupplyStation.h"
#include "Player/ShootPlayerController.h"
#include "Weapons/ShootRangedWeaponInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Interaction_RefillAmmo)

UShootGA_Interaction_RefillAmmo::UShootGA_Interaction_RefillAmmo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	// 与 Collect 相同，客户端完成读条后通过 GameplayEvent 预测激活；服务器仍重新校验距离并唯一写弹药。
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UShootGA_Interaction_RefillAmmo::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	APawn* InstigatorPawn = TriggerEventData
		? const_cast<APawn*>(Cast<APawn>(TriggerEventData->Instigator.Get()))
		: nullptr;
	AShootAmmoSupplyStation* SupplyStation = TriggerEventData
		? const_cast<AShootAmmoSupplyStation*>(Cast<AShootAmmoSupplyStation>(TriggerEventData->Target.Get()))
		: nullptr;

	if (ActorInfo && ActorInfo->IsNetAuthority() && InstigatorPawn && SupplyStation &&
		SupplyStation->CanSupplyPawn(InstigatorPawn))
	{
		AShootPlayerController* PlayerController = Cast<AShootPlayerController>(InstigatorPawn->GetController());
		UShootQuickBarComponent* QuickBar = PlayerController
			? PlayerController->GetGameplayQuickBarComponent()
			: nullptr;
		UShootRangedWeaponInstance* WeaponInstance = QuickBar
			? Cast<UShootRangedWeaponInstance>(QuickBar->FindActiveWeaponInstance())
			: nullptr;
		if (WeaponInstance)
		{
			WeaponInstance->RefillAmmoToCapacity();
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
