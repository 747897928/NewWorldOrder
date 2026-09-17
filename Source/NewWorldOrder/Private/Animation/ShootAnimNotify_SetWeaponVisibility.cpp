// Copyright NewWorldOrder Game. All Rights Reserved.

#include "Animation/ShootAnimNotify_SetWeaponVisibility.h"

#include "Character/CombatComponent.h"
#include "GameFramework/Character.h"

UShootAnimNotify_SetWeaponVisibility::UShootAnimNotify_SetWeaponVisibility()
{
}

FString UShootAnimNotify_SetWeaponVisibility::GetNotifyName_Implementation() const
{
	switch (WeaponAction)
	{
	case EShootWeaponAnimAction::Show:
		return TEXT("WeaponShow");
	case EShootWeaponAnimAction::Hide:
		return TEXT("WeaponHide");
	case EShootWeaponAnimAction::Destroy:
		return TEXT("WeaponDestroy");
	default:
		return Super::GetNotifyName_Implementation();
	}
}

void UShootAnimNotify_SetWeaponVisibility::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(MeshComp->GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	if (UCombatComponent* CombatComponent = OwnerPawn->FindComponentByClass<UCombatComponent>())
	{
		CombatComponent->HandleWeaponAnimNotify(WeaponAction);
	}
}
