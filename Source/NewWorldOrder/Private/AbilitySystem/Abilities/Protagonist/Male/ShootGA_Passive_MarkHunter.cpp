// 被动：标记猎手，默认监听击杀事件按最大生命值比例回血
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Passive_MarkHunter.h"

#include "ShootGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/Effects/ShootEffect_Marked.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "Character/ShootCharacter.h"
#include "GameModes/ShootGameModeBase.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"

UShootGA_Passive_MarkHunter::UShootGA_Passive_MarkHunter()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	ActivationPolicy = EShootAbilityActivationPolicy::OnSpawn;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Male.Passive.MarkHunter"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Passive1);
	SetAssetTags(AssetTags);
	// 被动由 AbilitySet 授予；OnSpawn 后保持激活，直到套件撤销时解绑全局击杀委托。

	EliteHealthThreshold = 200.f;
	HealPercentElite = 0.08f;
	HealPercentNormal = 0.04f;
	NormalHealMinLevel = 3;

	SpreadRadius = 800.f;
	SpreadDuration = 8.f;
	SpreadRadiusMultiplierLv3 = 1.5f;
	bSpreadRequiresVictimMarked = true;
	SpreadMarkEffectClass = UShootEffect_Marked::StaticClass();
}

void UShootGA_Passive_MarkHunter::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 服务器监听击杀事件
	if (AShootGameModeBase* ShootGameModeBase = GetShootGameMode())
	{
		ShootGameModeBase->OnCharacterKilled.AddUniqueDynamic(this, &ThisClass::HandleKill);
	}
}

void UShootGA_Passive_MarkHunter::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AShootGameModeBase* ShootGameModeBase = GetShootGameMode())
	{
		ShootGameModeBase->OnCharacterKilled.RemoveDynamic(this, &ThisClass::HandleKill);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_Passive_MarkHunter::HandleKill(AActor* Killer, AActor* Victim)
{
	AShootCharacter* OwnerCharacter = GetShootCharacterFromActorInfo();

	// 仅当击杀者为自身时触发
	if (!OwnerCharacter || !UShootAbilitySystemLibrary::IsKillAttributedToActor(Killer, OwnerCharacter))
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	const float MaxHealth = ASC ? ASC->GetNumericAttribute(UShootAttributeSet::GetMaxHealthAttribute()) : 0.f;
	if (MaxHealth <= 0.f)
	{
		return;
	}
	const float AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);

	// 按正史规则，>200HP 视为精英击杀；普通击杀仅在 Lv3 解锁回血
	float HealPercent = 0.f;
	float VictimMaxHealth = 0.f;
	if (Victim)
	{
		if (const UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim))
		{
			VictimMaxHealth = VictimASC->GetNumericAttribute(UShootAttributeSet::GetMaxHealthAttribute());
		}
	}
	if (VictimMaxHealth > EliteHealthThreshold)
	{
		HealPercent = HealPercentElite;
	}
	else if (AbilityLevel >= NormalHealMinLevel)
	{
		HealPercent = HealPercentNormal;
	}

	const float HealValue = MaxHealth * HealPercent;
	if (HealValue > 0.f && ASC)
	{
		ASC->ApplyModToAttribute(UShootAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, HealValue);
	}

	// Lv3 解锁标记传播，配合女主战术扫描（优先要求被击杀目标已有标记）
	if (AbilityLevel < 3.f || !Victim)
	{
		return;
	}

	if (!SpreadMarkEffectClass || SpreadRadius <= 0.f || SpreadDuration <= 0.f)
	{
		return;
	}

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	if (bSpreadRequiresVictimMarked)
	{
		const UAbilitySystemComponent* VictimASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Victim);
		if (!VictimASC || !Tags.Status_Marked.IsValid() || !VictimASC->HasMatchingGameplayTag(Tags.Status_Marked))
		{
			return;
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float EffectiveRadius = SpreadRadius * SpreadRadiusMultiplierLv3;
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MarkHunterSpread), false, OwnerCharacter);
	const FVector Center = Victim->GetActorLocation();
	const bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(EffectiveRadius),
		Params);

	if (!bHit)
	{
		return;
	}

	auto IsHostileTarget = [](AActor* Source, AActor* Target) -> bool
	{
		return UShootAbilitySystemLibrary::IsHostileActor(Source, Target);
	};

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* TargetActor = Result.GetActor();
		if (!TargetActor || TargetActor == OwnerCharacter || TargetActor == Victim)
		{
			continue;
		}

		if (!IsHostileTarget(OwnerCharacter, TargetActor))
		{
			continue;
		}

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
			Ctx.AddSourceObject(this);
			FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(SpreadMarkEffectClass, AbilityLevel, Ctx);
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetDuration(SpreadDuration, true);
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}
