// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"

#include "ShootAnimNotify_RobotFire.generated.h"

/** 放在机器人 Fire Montage 的枪口帧；服务器上的 Robot Fire GA 收到后执行唯一一次伤害。 */
UCLASS(meta=(DisplayName="Robot Fire"))
class NEWWORLDORDER_API UShootAnimNotify_RobotFire : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
};
