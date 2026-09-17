// 男主E：震撼手雷，默认生成爆炸 Actor 造成伤害/控制/易伤
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_ShockGrenade.h"

#include "AbilitySystem/Actors/ShootSkillExplosionActor.h"
#include "AbilitySystem/Effects/ShootEffect_Stun.h"
#include "AbilitySystem/Effects/ShootEffect_Vulnerable.h"
#include "ShootGameplayTags.h"

UShootGA_Male_ShockGrenade::UShootGA_Male_ShockGrenade()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Male.SkillE.ShockGrenade"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Secondary);
	SetAssetTags(AssetTags);

	ExplosionClass = AShootSkillExplosionActor::StaticClass();
	Damage = 150.f;
	Radius = 300.f;
	FuseTime = 0.2f;
	VulnerableDuration = 5.f;
	StunDuration = 2.f; // 普通敌人满控制，可按目标调整
	KnockbackStrength = 800.f; // 击退力度，可按技能配置覆盖
	VulnerableEffectClass = UShootEffect_Vulnerable::StaticClass();
	ControlEffectClass = UShootEffect_Stun::StaticClass();
}

void UShootGA_Male_ShockGrenade::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (ExplosionClass)
		{
			FActorSpawnParameters Params;
			Params.Owner = GetAvatarActorFromActorInfo();
			FVector SpawnLoc = GetAvatarActorFromActorInfo() ? GetAvatarActorFromActorInfo()->GetActorLocation() : FVector::ZeroVector;
			FRotator SpawnRot = GetAvatarActorFromActorInfo() ? GetAvatarActorFromActorInfo()->GetActorRotation() : FRotator::ZeroRotator;
			// TODO: ShockGrenade - 使用投掷轨迹/落点选择，而非直接在脚下生成；支持每目标控制规则。
			if (AShootSkillExplosionActor* Explosion = World->SpawnActorDeferred<AShootSkillExplosionActor>(ExplosionClass, FTransform(SpawnRot, SpawnLoc), Params.Owner))
			{
				Explosion->ConfigureExplosion(Damage, Radius, FuseTime, VulnerableDuration, StunDuration, KnockbackStrength,
					nullptr, ControlEffectClass, VulnerableEffectClass);
				Explosion->FinishSpawning(FTransform(SpawnRot, SpawnLoc));
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
