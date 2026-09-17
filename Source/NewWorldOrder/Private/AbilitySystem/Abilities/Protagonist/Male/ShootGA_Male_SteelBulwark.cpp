// 男主C：钢铁壁垒，默认生成护盾墙 Actor，加额外减伤/护盾，可扩展反伤/爆炸
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Male_SteelBulwark.h"

#include "AbilitySystem/Actors/ShootSkillShieldWall.h"
#include "ShootGameplayTags.h"

UShootGA_Male_SteelBulwark::UShootGA_Male_SteelBulwark()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Male.SkillC.SteelBulwark"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Utility);
	SetAssetTags(AssetTags);

	ShieldWallClass = AShootSkillShieldWall::StaticClass();
	ShieldDuration = 5.f;
	ShieldBonus = 0.f;
	DamageReduction = 0.8f;
}

void UShootGA_Male_SteelBulwark::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
		if (ShieldWallClass)
		{
			FActorSpawnParameters Params;
			Params.Owner = GetAvatarActorFromActorInfo();
			const FVector SpawnLoc = GetAvatarActorFromActorInfo() ? GetAvatarActorFromActorInfo()->GetActorLocation() : FVector::ZeroVector;
			const FRotator SpawnRot = GetAvatarActorFromActorInfo() ? GetAvatarActorFromActorInfo()->GetActorRotation() : FRotator::ZeroRotator;
			// TODO: SteelBulwark - 调整盾墙朝向与覆盖范围、阻挡/反射逻辑；支持手动取消与破碎爆炸。
			if (AShootSkillShieldWall* Shield = World->SpawnActorDeferred<AShootSkillShieldWall>(ShieldWallClass, FTransform(SpawnRot, SpawnLoc), Params.Owner))
			{
				Shield->InitShield(GetAvatarActorFromActorInfo());
				// TODO(codex): ShieldWall 参数当前为 protected，Phase 1 先用 Actor 默认/资产配置。
				// Shield->Duration = ShieldDuration;
				// Shield->ShieldBonus = ShieldBonus;
				// Shield->DamageReduction = DamageReduction;
				Shield->FinishSpawning(FTransform(SpawnRot, SpawnLoc));
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
