// Copyright ZhaoYiJie

#include "Character/ShootMannequinAnimInstance.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/ShootCharacter.h"
#include "Character/ShootCharacterMovementComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootMannequinAnimInstance)

UShootMannequinAnimInstance::UShootMannequinAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootMannequinAnimInstance::InitializeWithAbilitySystem(UAbilitySystemComponent* AbilitySystem)
{
	if (!AbilitySystem || BoundAbilitySystem.Get() == AbilitySystem)
	{
		return;
	}

	// PlayerState ASC 可能晚于 Mutable 重建 AnimInstance 到达。每个实例只在 ASC
	// 真正变化时重绑，避免 Tick 中重复注册 GameplayTag 委托。
	GameplayTagPropertyMap.Initialize(this, AbilitySystem);
	BoundAbilitySystem = AbilitySystem;
}

#if WITH_EDITOR
EDataValidationResult UShootMannequinAnimInstance::IsDataValid(FDataValidationContext& Context) const
{
	Super::IsDataValid(Context);
	GameplayTagPropertyMap.IsDataValid(this, Context);
	return Context.GetNumErrors() > 0
		? EDataValidationResult::Invalid
		: EDataValidationResult::Valid;
}
#endif

void UShootMannequinAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (AActor* OwningActor = GetOwningActor())
	{
		InitializeWithAbilitySystem(
			UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
	}
}

void UShootMannequinAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	AShootCharacter* Character = Cast<AShootCharacter>(GetOwningActor());
	if (!Character)
	{
		return;
	}

	// Mutable 或 PlayerState 初始化顺序变化时在安全的原生更新入口补绑 ASC。
	InitializeWithAbilitySystem(Character->GetAbilitySystemComponent());

	if (UShootCharacterMovementComponent* Movement =
		Cast<UShootCharacterMovementComponent>(Character->GetCharacterMovement()))
	{
		GroundDistance = Movement->GetGroundInfo().GroundDistance;
	}
}
