// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/ShootCharacterBase.h"

#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameModes/ShootGameModeBase.h"
#include "Interaction/ShootReviveInteractableComponent.h"
#include "Net/UnrealNetwork.h"

namespace ShootCharacterCollision
{
	// 与 LyraCharacter 保持一致；对应 Profile 已迁移到 Config/DefaultEngine.ini。
	// 主 Mesh 接收精确武器命中，Capsule 负责近似命中与移动碰撞，服装/头发/鞋子组件不参与这条设置。
	static const FName PawnCapsuleProfile(TEXT("LyraPawnCapsule"));
	static const FName PawnMeshProfile(TEXT("LyraPawnMesh"));
}

// Sets default values
AShootCharacterBase::AShootCharacterBase(const FObjectInitializer& ObjectInitializer): Super(ObjectInitializer)
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 调用链：武器 GA 使用 Lyra_TraceChannel_Weapon -> 命中角色主 Mesh；
	// Capsule 使用 LyraPawnCapsule 保留移动碰撞，并为需要 Capsule Trace 的玩法提供独立通道。
	GetCapsuleComponent()->SetCollisionProfileName(ShootCharacterCollision::PawnCapsuleProfile);
	GetMesh()->SetCollisionProfileName(ShootCharacterCollision::PawnMeshProfile);

	// 倒地救援交互组件默认关闭；仅允许启用该玩法的模式 AbilitySet/GA 驱动，
	// 终结死亡链不会再根据 Health=0 自动开放救援。
	ReviveInteractableComponent = CreateDefaultSubobject<UShootReviveInteractableComponent>(TEXT("ReviveInteractableComponent"));
}

UAbilitySystemComponent* AShootCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAnimMontage* AShootCharacterBase::GetHitReactMontage_Implementation()
{
	return HitReactMontage;
}

void AShootCharacterBase::Die(const FVector& DeathImpulse)
{
	if (DeathState != EShootDeathState::NotDead)
	{
		return;
	}

	StartDeath();

	// 死亡后的清理、旁观和重生属于当前地图规则。未来 PVE 的“倒地救起”必须在调用 Die 前
	// 由模式 AbilitySet/GA 接管，不能让所有 GameMode 都隐式拥有倒地玩法。
	if (HasAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (AShootGameModeBase* ShootGameModeBase = World->GetAuthGameMode<AShootGameModeBase>())
			{
				ShootGameModeBase->PlayerDied(this);
			}
		}
	}
}

void AShootCharacterBase::StartDeath()
{
	if (DeathState != EShootDeathState::NotDead)
	{
		return;
	}

	DeathState = EShootDeathState::DeathStarted;
	OnDeathDelegate.Broadcast(this);

	// 停止移动/输入，关闭碰撞
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->DisableMovement();
	}
	StopAnimMontage();
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	
	// 取消/移除能力
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CancelAllAbilities();
		AbilitySystemComponent->RemoveAllGameplayCues();
	}

	ForceNetUpdate();
}

void AShootCharacterBase::FinishDeath()
{
	if (DeathState != EShootDeathState::DeathStarted)
	{
		return;
	}

	DeathState = EShootDeathState::DeathFinished;
	ForceNetUpdate();
}

void AShootCharacterBase::OnRep_DeathState(EShootDeathState OldDeathState)
{
	const EShootDeathState NewDeathState = DeathState;
	DeathState = OldDeathState;

	// 与 Lyra 相同，通过状态转换函数重放客户端表现，避免服务器只关碰撞而客户端仍能移动。
	if (OldDeathState == EShootDeathState::NotDead && NewDeathState >= EShootDeathState::DeathStarted)
	{
		StartDeath();
	}
	if (DeathState == EShootDeathState::DeathStarted && NewDeathState == EShootDeathState::DeathFinished)
	{
		FinishDeath();
	}
}

void AShootCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, DeathState);
}

bool AShootCharacterBase::IsDead_Implementation() const
{
	return DeathState != EShootDeathState::NotDead;
}

void AShootCharacterBase::SetReviveInteractableEnabled(bool bEnabled)
{
	if (!ReviveInteractableComponent)
	{
		return;
	}

	// 避免重复设置，减少交互刷新成本
	if (ReviveInteractableComponent->IsReviveEnabled() == bEnabled)
	{
		return;
	}

	ReviveInteractableComponent->SetReviveEnabled(bEnabled);
}

// Called when the game starts or when spawned
void AShootCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AShootCharacterBase::InitAbilityActorInfo()
{
}

void AShootCharacterBase::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const
{
	check(IsValid(GetAbilitySystemComponent()));
	check(GameplayEffectClass);
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(
		GameplayEffectClass, Level, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), GetAbilitySystemComponent());
}

void AShootCharacterBase::InitializeDefaultAttributes() const
{
	if (DefaultPrimaryAttributes)
	{
		ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);
	}
	if (DefaultSecondaryAttributes)
	{
		ApplyEffectToSelf(DefaultSecondaryAttributes, 1.f);
	}

	if (DefaultVitalAttributes)
	{
		ApplyEffectToSelf(DefaultVitalAttributes, 1.f);
	}
}


void AShootCharacterBase::ResetAndInitializeDefaultAttributes() const
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		// InitializeDefaultAttributes 通过 ApplyEffectToSelf 应用 GE，其 SourceObject 均为本角色；
		// 移除后重新应用，保证多次进副本/回合重开属性从初始状态开始，不叠加无限 GE。
		FGameplayEffectQuery Query;
		Query.EffectSource = this;   // 匹配 SourceObject=本角色的 GE(ApplyEffectToSelf 均以本角色为源)
		for (const FActiveGameplayEffectHandle& Handle : ASC->GetActiveEffects(Query))
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}
	InitializeDefaultAttributes();
}

void AShootCharacterBase::AddCharacterAbilities()
{
	UShootAbilitySystemComponent* ShootASC = CastChecked<UShootAbilitySystemComponent>(AbilitySystemComponent);
	if (!HasAuthority()) return;

	ShootASC->AddCharacterAbilities(StartupAbilities);
	ShootASC->AddCharacterPassiveAbilities(StartupPassiveAbilities);
}
