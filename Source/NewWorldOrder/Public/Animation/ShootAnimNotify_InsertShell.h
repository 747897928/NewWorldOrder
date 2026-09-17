// Copyright ZhaoYiJie

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "ShootAnimNotify_InsertShell.generated.h"

/**
 * 逐发装填动画在弹壳真正进入弹仓后发送 GameplayEvent.Reload.InsertShell。
 *
 * 这里只标记动画提交点，不直接修改弹药；活动的逐发装填 GA 在服务器收到事件后
 * 才把一发从备弹转入弹匣。因此在此 Notify 之前被开火取消时，本轮弹药不会增加。
 */
UCLASS(meta=(DisplayName="Shoot Insert Shell"))
class NEWWORLDORDER_API UShootAnimNotify_InsertShell : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
