// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "Weapons/ShootWeaponInstance.h"
#include "ShootAnimNotify_SetWeaponVisibility.generated.h"

/**
 * AnimNotify：驱动武器显隐（与 CombatComponent/WeaponInstance 协调）
 */
UCLASS(meta=(DisplayName="Shoot Weapon Visibility"))
class NEWWORLDORDER_API UShootAnimNotify_SetWeaponVisibility : public UAnimNotify
{
	GENERATED_BODY()

public:
	UShootAnimNotify_SetWeaponVisibility();

	/** 要执行的动作（显示/隐藏/销毁） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Weapon")
	EShootWeaponAnimAction WeaponAction = EShootWeaponAnimAction::Show;

	virtual FString GetNotifyName_Implementation() const override;
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
