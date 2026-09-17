// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"

#include "ShootAnimNotify_RobotMelee.generated.h"

/** 放在机器人 Claw/Chomp Montage 的接触帧；服务器上的 Robot Melee GA 收到后结算一次伤害。 */
UCLASS(meta=(DisplayName="Robot Melee Impact"))
class NEWWORLDORDER_API UShootAnimNotify_RobotMelee : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
