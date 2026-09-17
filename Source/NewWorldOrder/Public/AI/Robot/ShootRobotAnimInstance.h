// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"

#include "ShootRobotAnimInstance.generated.h"

/** Robot AnimBP 的线程安全数据源；状态机和 BlendSpace 仍由蓝图资产维护。 */
UCLASS(Transient, Blueprintable)
class NEWWORLDORDER_API UShootRobotAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category="Robot|Animation")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Robot|Animation")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category="Robot|Animation")
	bool bIsFalling = false;

	UPROPERTY(BlueprintReadOnly, Category="Robot|Animation")
	bool bIsDead = false;

private:
	TWeakObjectPtr<class AShootRobotCompanionCharacter> RobotCharacter;
};
