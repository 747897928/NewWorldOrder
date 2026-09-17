// 女主Q：战术扫描，默认范围标记敌人，施加 Status.Marked/弱点易伤
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_TacticalScan.h"

#include "ShootGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShootAbilitySystemLibrary.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "AbilitySystem/Effects/ShootEffect_Vulnerable.h"

UShootGA_Female_TacticalScan::UShootGA_Female_TacticalScan()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Female.SkillQ.TacticalScan"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Primary);
	SetAssetTags(AssetTags);

	Radius = 1500.f; // 15m
	Duration = 15.f;
	VulnerableBonus = 0.1f; // 弱点伤害 +10%
	MarkEffectClass = nullptr;
	VulnerableEffectClass = nullptr;
}

void UShootGA_Female_TacticalScan::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 仅服务器执行扫描与应用效果
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TacticalScan), false, GetAvatarActorFromActorInfo());
	const FVector Center = GetAvatarActorFromActorInfo() ? GetAvatarActorFromActorInfo()->GetActorLocation() : FVector::ZeroVector;
	bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius),
		Params);

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	AActor* SourceActor = GetAvatarActorFromActorInfo();

	auto IsHostileTarget = [](AActor* Source, AActor* Target) -> bool
	{
		return UShootAbilitySystemLibrary::IsHostileActor(Source, Target);
	};

	// 先统计敌对目标数量，用于“尸潮红利”持续时间翻倍
	TArray<AActor*> HostileTargets;
	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* TargetActor = Result.GetActor();
		if (!TargetActor) continue;

		if (!IsHostileTarget(SourceActor, TargetActor))
		{
			continue;
		}

		HostileTargets.Add(TargetActor);
	}

	const bool bIsHorde = HostileTargets.Num() >= 10;
	const float EffectiveDuration = bIsHorde ? Duration * 2.0f : Duration;

	if (UWorld* W = World)
	{
		W->GetTimerManager().ClearTimer(ScanTimerHandle);
		W->GetTimerManager().SetTimer(ScanTimerHandle, this, &ThisClass::HandleScanExpired, EffectiveDuration, false);
	}

	// TODO: TacticalScan - 群体命中时延长 Mark 持续；为被标记目标触发高亮/视野提示 Cue。
	for (AActor* TargetActor : HostileTargets)
	{
		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			CachedTargetASCs.AddUnique(TargetASC);

			// 机制：易伤（可选）
			if (VulnerableEffectClass)
			{
				FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
				Ctx.AddSourceObject(this);
				if (FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(VulnerableEffectClass, 1.f, Ctx); Spec.IsValid())
				{
					Spec.Data->SetDuration(EffectiveDuration, true);
					TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}

			// 状态：标记标签（便于 UI/其他系统查询；到期在 EndAbility 中移除）
			FGameplayTagContainer StatusToAdd;
			StatusToAdd.AddTag(Tags.Status_Marked);
			TargetASC->AddLooseGameplayTags(StatusToAdd);

			if (MarkEffectClass)
			{
				FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
				Ctx.AddSourceObject(this);
				if (FGameplayEffectSpecHandle Spec = TargetASC->MakeOutgoingSpec(MarkEffectClass, 1.f, Ctx); Spec.IsValid())
				{
					Spec.Data->SetDuration(EffectiveDuration, true);
					TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}
			else
			{
				// 默认：仅依赖 Status_Marked/Status_Vulnerable + VulnerableEffectClass 的机制效果
			}
		}
	}
}

void UShootGA_Female_TacticalScan::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScanTimerHandle);
	}

	// 仅服务器移除目标状态标签
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		const FShootGameplayTags& Tags = FShootGameplayTags::Get();
		FGameplayTagContainer StatusToRemove;
		StatusToRemove.AddTag(Tags.Status_Marked);

		for (const TWeakObjectPtr<UAbilitySystemComponent>& ASCWeak : CachedTargetASCs)
		{
			if (UAbilitySystemComponent* TargetASC = ASCWeak.Get())
			{
				TargetASC->RemoveLooseGameplayTags(StatusToRemove);
			}
		}
	}

	CachedTargetASCs.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UShootGA_Female_TacticalScan::HandleScanExpired()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
