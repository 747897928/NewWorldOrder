// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"

#include "ShootAnimNotify_ZombieMeleeHit.generated.h"

/** 放在 Zombie Attack Montage 实际接触帧；只向服务器 Zombie GA 发出命中窗口信号。 */
UCLASS(meta=(DisplayName="Zombie Melee Impact"))
class NEWWORLDORDER_API UShootAnimNotify_ZombieMeleeHit : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
