// Copyright ZhaoYiJie

#pragma once

#include "GameFramework/CharacterMovementComponent.h"
#include "ShootCharacterMovementComponent.generated.h"

/**
 * 主动画蓝图每帧需要的地面信息。
 *
 * 与 Lyra 的 FLyraCharacterGroundInfo 保持相同职责：落地时距离为 0，
 * 离地时按需向下 Trace。数据缓存在 MovementComponent，不让 AnimBP
 * 在并行动画更新中遍历 World 或重复发射射线。
 */
USTRUCT(BlueprintType)
struct FShootCharacterGroundInfo
{
	GENERATED_BODY()

	uint64 LastUpdateFrame = 0;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance = 0.0f;
};

/**
 * 项目层的 Lyra CharacterMovement 适配。
 *
 * 调用链：
 * AShootCharacter 构造时替换 ACharacter 默认 MovementComponent
 * -> UShootMannequinAnimInstance::NativeUpdateAnimation
 * -> GetGroundInfo
 * -> 正式 CC 主 AnimBP 的 Jump/Fall/Land 状态。
 *
 * 该类只承接项目需要的移动事实，不修改 Lyra 插件或 Unreal Engine 源码。
 */
UCLASS(Config=Game)
class NEWWORLDORDER_API UShootCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UShootCharacterMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void SimulateMovement(float DeltaTime) override;
	virtual bool CanAttemptJump() const override;
	virtual bool CanEverSwim() const override;
	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0) override;
	virtual void PhysicsVolumeChanged(class APhysicsVolume* NewVolume) override;

	UFUNCTION(BlueprintCallable, Category="Shoot|CharacterMovement")
	const FShootCharacterGroundInfo& GetGroundInfo();

	/** 模拟代理收到 AShootCharacter 压缩加速度后写回，供 Lyra locomotion 状态判断使用。 */
	void SetReplicatedAcceleration(const FVector& InAcceleration);

private:
	FShootCharacterGroundInfo CachedGroundInfo;

	UPROPERTY(Transient)
	bool bHasReplicatedAcceleration = false;
};
