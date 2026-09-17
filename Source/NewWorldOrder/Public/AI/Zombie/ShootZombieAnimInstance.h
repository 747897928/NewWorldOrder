// Copyright NewWorldOrder Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

#include "ShootZombieAnimInstance.generated.h"

class AEnemyBotCharacter;

/**
 * Zombie 正式 AnimBP 的稳定状态契约。
 *
 * 这里故意不暴露具体骨架、动画序列或 BlendSpace。五个 archetype 共用同一套状态语义，
 * 具体视觉资产由各自 AnimBP 配置；这样换动画不会把资产路径和动画调优写进 C++。
 */
UCLASS(Blueprintable, BlueprintType)
class NEWWORLDORDER_API UShootZombieAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** 当前速度大小，供 AnimBP 的 Idle/Move 状态转换或 BlendSpace 使用。 */
	UPROPERTY(BlueprintReadOnly, Category="Zombie|Animation")
	float GroundSpeed = 0.0f;

	/** 由 GroundSpeed 派生的稳定移动状态；不携带输入设备或阵营判断。 */
	UPROPERTY(BlueprintReadOnly, Category="Zombie|Animation")
	bool bIsMoving = false;

	/** CharacterMovement 当前是否处于空中。 */
	UPROPERTY(BlueprintReadOnly, Category="Zombie|Animation")
	bool bIsFalling = false;

	/** 通过 Combat Interface 的死亡状态驱动死亡表现。 */
	UPROPERTY(BlueprintReadOnly, Category="Zombie|Animation")
	bool bIsDead = false;

	/** 由 Character 复制的服务器近战阶段提供；正式 AnimBP 用它切入 Attack，Montage 负责具体动作。 */
	UPROPERTY(BlueprintReadOnly, Category="Zombie|Animation")
	bool bIsAttacking = false;

	TWeakObjectPtr<AEnemyBotCharacter> ZombieCharacter;
};
