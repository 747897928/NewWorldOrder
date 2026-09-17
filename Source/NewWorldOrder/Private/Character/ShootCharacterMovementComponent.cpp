// Copyright ZhaoYiJie

#include "Character/ShootCharacterMovementComponent.h"

#include "Character/ShootCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PhysicsVolume.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootCharacterMovementComponent)

namespace ShootCharacterMovement
{
	static float GroundTraceDistance = 100000.0f;
	static FAutoConsoleVariableRef CVarGroundTraceDistance(
		TEXT("ShootCharacter.GroundTraceDistance"),
		GroundTraceDistance,
		TEXT("Distance to trace down when generating character animation ground information."),
		ECVF_Cheat);
}

UShootCharacterMovementComponent::UShootCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootCharacterMovementComponent::SimulateMovement(float DeltaTime)
{
	if (!bHasReplicatedAcceleration)
	{
		Super::SimulateMovement(DeltaTime);
		return;
	}

	// ACharacter 默认模拟会覆盖远端加速度；Lyra locomotion 的 Start/Stop/Pivot
	// 需要服务器复制来的值，因此模拟完成后恢复它。
	const FVector OriginalAcceleration = Acceleration;
	Super::SimulateMovement(DeltaTime);
	Acceleration = OriginalAcceleration;
}

bool UShootCharacterMovementComponent::CanAttemptJump() const
{
	// 与 Lyra 一致：蹲伏不在这里阻止跳跃。Jump GA 会先 UnCrouch，
	// 最终是否允许跳跃仍由 ACharacter::CanJumpInternal 校验。
	return IsJumpAllowed() && (IsMovingOnGround() || IsFalling());
}

bool UShootCharacterMovementComponent::CanEverSwim() const
{
	if (!Super::CanEverSwim())
	{
		return false;
	}

	// PhysicsVolume 是引擎移动模式的权威来源；Experience 只负责决定本局是否开放这条能力。
	// 非项目角色不受本规则影响，保留 CharacterMovement 的默认水体行为。
	const AShootCharacter* ShootCharacter = Cast<AShootCharacter>(CharacterOwner);
	return !ShootCharacter || ShootCharacter->IsSwimmingAllowedByExperience();
}

void UShootCharacterMovementComponent::SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode)
{
	if (NewMovementMode == MOVE_Swimming && !CanEverSwim())
	{
		// Experience 卸载或关闭游泳时，不能让外部调用把角色重新推回水中。
		// 在地面上回到当前 GroundMovementMode，否则先进入 Falling，交给引擎重新找地面。
		const EMovementMode FallbackMode = IsMovingOnGround() ? GetGroundMovementMode() : MOVE_Falling;
		Super::SetMovementMode(FallbackMode);
		return;
	}

	Super::SetMovementMode(NewMovementMode, NewCustomMode);
}

void UShootCharacterMovementComponent::PhysicsVolumeChanged(APhysicsVolume* NewVolume)
{
	if (NewVolume && NewVolume->bWaterVolume && !CanEverSwim())
	{
		// 不调用基类的入水分支，角色会继续按楼梯/池底的陆地碰撞处理。
		// 这使水体几何可以复用在多个 Experience，而不把游泳能力写死在地图 Actor 上。
		if (IsSwimming())
		{
			Super::SetMovementMode(MOVE_Falling);
		}
		return;
	}

	Super::PhysicsVolumeChanged(NewVolume);
}

const FShootCharacterGroundInfo& UShootCharacterMovementComponent::GetGroundInfo()
{
	if (!CharacterOwner || CachedGroundInfo.LastUpdateFrame == GFrameCounter)
	{
		return CachedGroundInfo;
	}

	if (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	else
	{
		const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
		check(Capsule);

		const float CapsuleHalfHeight = Capsule->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel =
			UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn;

		// 起点是胶囊中心；终点额外减去半高，得到“胶囊底到地面”的距离。
		const FVector TraceStart = GetActorLocation();
		const FVector TraceEnd(
			TraceStart.X,
			TraceStart.Y,
			TraceStart.Z - ShootCharacterMovement::GroundTraceDistance - CapsuleHalfHeight);

		FCollisionQueryParams QueryParams(
			SCENE_QUERY_STAT(ShootCharacterMovement_GetGroundInfo),
			false,
			CharacterOwner);
		FCollisionResponseParams ResponseParams;
		InitCollisionParams(QueryParams, ResponseParams);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(
			HitResult,
			TraceStart,
			TraceEnd,
			CollisionChannel,
			QueryParams,
			ResponseParams);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = ShootCharacterMovement::GroundTraceDistance;
		if (HitResult.bBlockingHit)
		{
			CachedGroundInfo.GroundDistance =
				FMath::Max(HitResult.Distance - CapsuleHalfHeight, 0.0f);
		}
	}

	CachedGroundInfo.LastUpdateFrame = GFrameCounter;
	return CachedGroundInfo;
}

void UShootCharacterMovementComponent::SetReplicatedAcceleration(const FVector& InAcceleration)
{
	bHasReplicatedAcceleration = true;
	Acceleration = InAcceleration;
}
