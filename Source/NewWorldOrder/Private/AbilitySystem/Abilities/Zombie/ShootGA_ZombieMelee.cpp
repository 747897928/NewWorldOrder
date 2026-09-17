// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Abilities/Zombie/ShootGA_ZombieMelee.h"

#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/ShootEffect_ZombieMeleeCooldown.h"
#include "AbilitySystemGlobals.h"
#include "AI/EnemyBotCharacter.h"
#include "GameplayEffect.h"
#include "Interface/CombatInterface.h"
#include "ShootGameplayTags.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_ZombieMelee)

UShootGA_ZombieMelee::UShootGA_ZombieMelee()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;

	// 这两个僵尸专用 Tag 的唯一注册源是 DefaultGameplayTags.ini。配置 Tag 在 GameplayAbility
	// CDO 创建前已经由 GameplayTagsSettings 注册；这里直接请求，不能依赖 AssetManager 启动后
	// 才填充的 FShootGameplayTags 字段，否则 CDO 可能持有空 AbilityTags，运行时无法按标签找到它。
	FGameplayTagContainer AssetTags;
	const FGameplayTag ZombieMeleeTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Ability.Skill.Zombie.Melee")), false);
	if (ZombieMeleeTag.IsValid())
	{
		AssetTags.AddTag(ZombieMeleeTag);
	}
	SetAssetTags(AssetTags);
	CooldownGameplayEffectClass = UShootEffect_ZombieMeleeCooldown::StaticClass();
	const FGameplayTag ZombieMeleeCooldownTag = FGameplayTag::RequestGameplayTag(
		FName(TEXT("Cooldown.AI.ZombieMelee")), false);
	if (ZombieMeleeCooldownTag.IsValid())
	{
		ActivationBlockedTags.AddTag(ZombieMeleeCooldownTag);
	}
}

bool UShootGA_ZombieMelee::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AEnemyBotCharacter* Zombie = ActorInfo
		? Cast<AEnemyBotCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	const AActor* Target = Zombie ? Zombie->GetPendingMeleeTarget() : nullptr;
	return ActorInfo && ActorInfo->IsNetAuthority() && Zombie && Target
		&& Zombie->GetAttackMontage()
		&& Zombie->IsMeleeAttackTargetValid(*Target);
}

void UShootGA_ZombieMelee::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AEnemyBotCharacter* Zombie = ActorInfo
		? Cast<AEnemyBotCharacter>(ActorInfo->AvatarActor.Get())
		: nullptr;
	AActor* Target = Zombie ? Zombie->GetPendingMeleeTarget() : nullptr;
	if (!ActorInfo || !ActorInfo->IsNetAuthority() || !Zombie || !Target
		|| !Zombie->GetAttackMontage() || !Zombie->IsMeleeAttackTargetValid(*Target)
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		if (Zombie)
		{
			Zombie->ClearPendingMeleeTarget();
		}
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ActiveZombie = Zombie;
	ActiveTarget = Target;
	bHitApplied = false;
	Zombie->OnMeleeAnimationNotify().AddUObject(this, &ThisClass::HandleMeleeNotify);
	Zombie->SetArchetypeAnimationState(EShootEnemyTestAnimationState::Attacking);
	Zombie->PlayReplicatedMeleeMontage();

	// 没有命中 Notify 时只结束能力，不补发伤害；这样资产漏配会暴露为“没有命中”，
	// 不会重新引入旧的“激活即扣血”时序。
	Zombie->GetWorldTimerManager().SetTimer(
		FinishAttackTimerHandle, this, &ThisClass::FinishAttackWithoutHit,
		FMath::Max(Zombie->GetAttackMontage()->GetPlayLength() + 0.1f, 0.1f), false);
}

void UShootGA_ZombieMelee::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (AEnemyBotCharacter* Zombie = ActiveZombie.Get())
	{
		Zombie->OnMeleeAnimationNotify().RemoveAll(this);
		Zombie->GetWorldTimerManager().ClearTimer(FinishAttackTimerHandle);
		Zombie->ClearPendingMeleeTarget();
		if (!bWasCancelled && Zombie->GetDeathState() == EShootDeathState::NotDead
			&& Zombie->IsArchetypeAttackAnimationActive())
		{
			Zombie->SetArchetypeAnimationState(EShootEnemyTestAnimationState::Idle);
		}
	}
	ActiveZombie.Reset();
	ActiveTarget.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_ZombieMelee::HandleMeleeNotify()
{
	if (bHitApplied)
	{
		return;
	}

	bHitApplied = true;
	ExecuteMeleeHit();
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UShootGA_ZombieMelee::ExecuteMeleeHit()
{
	AEnemyBotCharacter* Zombie = ActiveZombie.Get();
	AActor* Target = ActiveTarget.Get();
	if (!Zombie || !Target || !Zombie->IsMeleeAttackTargetValid(*Target))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = Zombie->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	auto ApplyConfiguredEffect = [&](const TSubclassOf<UGameplayEffect> EffectClass, const float Magnitude)
	{
		if (!EffectClass || Magnitude <= 0.0f)
		{
			return;
		}

		FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
		EffectContext.AddInstigator(Zombie, Zombie);
		EffectContext.AddSourceObject(Zombie);
		const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
			EffectClass, Zombie->GetPlayerLevel_Implementation(), EffectContext);
		if (!SpecHandle.IsValid())
		{
			return;
		}

		// ResourceInventory 只管理数量型资源；Zombie 的攻击属于 ASC/GE 战斗链，
		// QuickBar/Equipment 不参与。直接伤害和 Bleeder 周期 GE 统一读取 SetByCaller.Damage。
		FShootGameplayTags::SetSetByCallerMagnitude(
			*SpecHandle.Data.Get(), FShootGameplayTags::Get().SetByCaller_Damage, Magnitude);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	};

	ApplyConfiguredEffect(Zombie->GetAttackEffectClass(), Zombie->GetAttackDamage());
	ApplyConfiguredEffect(Zombie->GetAttackStatusEffectClass(), Zombie->GetAttackStatusMagnitude());
}

void UShootGA_ZombieMelee::FinishAttackWithoutHit()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UShootGA_ZombieMelee::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (!ActorInfo || !CooldownGameplayEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	AActor* Avatar = ActorInfo->AvatarActor.Get();
	if (!ASC || !Avatar)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddInstigator(Avatar, Avatar);
	Context.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(CooldownGameplayEffectClass, 1.0f, Context);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	const AEnemyBotCharacter* Zombie = Cast<AEnemyBotCharacter>(Avatar);
	const float Duration = Zombie ? FMath::Max(Zombie->GetAttackInterval(), 0.1f) : 1.0f;
	SpecHandle.Data->SetDuration(Duration, true);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
