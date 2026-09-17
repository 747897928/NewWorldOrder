// 男主Q：战术突击，默认 C++ 路径：短距冲刺 + 无敌帧 + 射速/移速 Buff
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_TacticalAssault.h"

#include "AbilitySystemComponent.h"
#include "ShootGameplayTags.h"
#include "AbilitySystem/Effects/ShootEffect_MoveSpeed.h"
#include "AbilitySystem/Effects/ShootEffect_FireRate.h"
#include "AbilitySystem/Effects/ShootEffect_MatchSkillCooldowns.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

UShootGA_Male_TacticalAssault::UShootGA_Male_TacticalAssault()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Male.SkillQ.TacticalAssault"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Primary);
	SetAssetTags(AssetTags);
	CooldownGameplayEffectClass = UShootEffect_TacticalAssaultCooldown::StaticClass();
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(
		FName("Cooldown.Skill.TacticalAssault"), false));
	ActivationGameplayCueTag = FGameplayTag::RequestGameplayTag(
		FName("GameplayCue.Skill.TacticalAssault.Activate"), false);

	MoveSpeedBuffClass = UShootEffect_MoveSpeed::StaticClass();
	FireRateBuffClass = UShootEffect_FireRate::StaticClass();
	DashDistance = 700.f; // ~7m，对应 Lv1 5m 可按蒙太奇/动画调（资产可覆盖）
	DashCapsuleRadius = 50.f;
	DashCapsuleHalfHeight = 88.f;
	DashInvulnerableTime = 0.3f;
	BuffDuration = 3.0f;
	ImpactDamage = 50.f;
	ImpactStunDuration = 1.0f;
}

void UShootGA_Male_TacticalAssault::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = ActorInfo ? Cast<ACharacter>(ActorInfo->AvatarActor.Get()) : nullptr;
	if (Character)
	{
		const FVector Forward = Character->GetActorForwardVector();
		const FVector Start = Character->GetActorLocation();
		const FVector End = Start + Forward * DashDistance;
		UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		if (ActorInfo->IsNetAuthority() && ActivationGameplayCueTag.IsValid())
		{
			FGameplayCueParameters CueParameters;
			CueParameters.Location = Start;
			CueParameters.Normal = Forward;
			CueParameters.Instigator = Character;
			CueParameters.EffectCauser = Character;
			K2_ExecuteGameplayCueWithParams(ActivationGameplayCueTag, CueParameters);
		}

		// 服务器侧进行一次胶囊 Sweep，命中目标后施加可配置的 GE（伤害/易伤）
		if (ActorInfo && ActorInfo->IsNetAuthority() && SourceASC)
		{
			FCollisionShape Capsule = FCollisionShape::MakeCapsule(DashCapsuleRadius, DashCapsuleHalfHeight);
			TArray<FHitResult> Hits;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(TacticalAssaultDash), false, Character);
			const ECollisionChannel Channel = ECC_Pawn;

			if (Character->GetWorld()->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, Channel, Capsule, Params))
			{
				TSet<AActor*> HitActors;
				FGameplayEffectContextHandle BaseCtx = SourceASC->MakeEffectContext();
				BaseCtx.AddSourceObject(this);
				for (const FHitResult& Hit : Hits)
				{
					AActor* HitActor = Hit.GetActor();
					if (!HitActor || HitActor == Character || HitActors.Contains(HitActor))
					{
						continue;
					}
					HitActors.Add(HitActor);

					if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor))
					{
						if (DashDamageEffect)
						{
							FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DashDamageEffect, GetAbilityLevel(Handle, ActorInfo), BaseCtx);
							if (Spec.IsValid())
							{
								SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
							}
						}

						if (DashVulnerableEffect)
						{
							FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DashVulnerableEffect, GetAbilityLevel(Handle, ActorInfo), BaseCtx);
							if (Spec.IsValid())
							{
								SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
							}
						}

						if (!DashDamageEffect && !DashVulnerableEffect)
						{
							UE_LOG(LogTemp, Verbose, TEXT("TacticalAssault hit %s but no damage/vulnerable effects configured."), *GetNameSafe(HitActor));
						}
					}
				}
			}
		}

		Character->LaunchCharacter(Forward * DashDistance, true, true);
		// TODO: TacticalAssault - 冲刺窗口内添加 i-frame / 硬直免疫 Tag，必要时改为 AbilityTask 控制时机。
	}

	// TODO: TacticalAssault - 添加冲刺路径碰撞检测，命中敌人时造成冲撞伤害/硬直并施加易伤；冲刺窗口内应用 i-frame/硬直免疫 Tag。

	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		ApplyEffectToOwner(MoveSpeedBuffClass, 1.f, BuffDuration);
		ApplyEffectToOwner(FireRateBuffClass, 1.f, BuffDuration);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
