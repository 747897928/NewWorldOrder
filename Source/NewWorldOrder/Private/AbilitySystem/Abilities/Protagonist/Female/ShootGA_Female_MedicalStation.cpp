// 女主X：医疗站，默认生成医疗站 Actor，周期治疗范围友军
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Female_MedicalStation.h"

#include "AbilitySystem/Actors/ShootSkillMedicalStation.h"
#include "AbilitySystem/Effects/ShootEffect_MatchSkillCooldowns.h"
#include "ShootGameplayTags.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"

UShootGA_Female_MedicalStation::UShootGA_Female_MedicalStation()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Female.Ultimate.MedicalStation"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Ultimate);
	SetAssetTags(AssetTags);
	CooldownGameplayEffectClass = UShootEffect_MedicalStationCooldown::StaticClass();
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(
		FName("Cooldown.Skill.MedicalStation"), false));
	DeployGameplayCueTag = FGameplayTag::RequestGameplayTag(
		FName("GameplayCue.Skill.MedicalStation.Deploy"), false);

	MedicalStationClass = AShootSkillMedicalStation::StaticClass();
}

void UShootGA_Female_MedicalStation::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 仅服务器 Spawn（客户端依赖复制/表现）
	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		if (UWorld* World = GetWorld())
		{
			if (MedicalStationClass)
			{
				FActorSpawnParameters Params;
				Params.Owner = GetAvatarActorFromActorInfo();
				Params.Instigator = Cast<APawn>(GetAvatarActorFromActorInfo());
				AActor* AvatarActor = GetAvatarActorFromActorInfo();
				const FTransform SpawnTransform = ResolveMedicalStationSpawnTransform(AvatarActor);
				const FVector SpawnLoc = SpawnTransform.GetLocation();
				const FRotator SpawnRot = SpawnTransform.Rotator();
				// TODO(主角-MedicalStation-Design): 治疗量缩放、死亡保护/额外交互按设计补齐。
				if (World->SpawnActor<AShootSkillMedicalStation>(MedicalStationClass, SpawnLoc, SpawnRot, Params)
					&& DeployGameplayCueTag.IsValid())
				{
					// 世界 Actor 负责持续治疗光环；GameplayCue 只表达本次部署成功，且由服务器广播一次。
					FGameplayCueParameters CueParameters;
					CueParameters.Location = SpawnLoc;
					CueParameters.Instigator = GetAvatarActorFromActorInfo();
					CueParameters.EffectCauser = GetAvatarActorFromActorInfo();
					K2_ExecuteGameplayCueWithParams(DeployGameplayCueTag, CueParameters);
				}
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

FTransform UShootGA_Female_MedicalStation::ResolveMedicalStationSpawnTransform_Implementation(
	AActor* AvatarActor) const
{
	if (!AvatarActor)
	{
		return FTransform::Identity;
	}

	const FVector AvatarLocation = AvatarActor->GetActorLocation();
	FVector GroundLocation = AvatarLocation;
	if (const ACharacter* Character = Cast<ACharacter>(AvatarActor))
	{
		// 当前玩法只要求部署在角色胶囊脚底。地面不阻挡 Visibility 时也不能把医疗站留在角色中心。
		const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
		GroundLocation.Z -= Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 0.f;
		GroundLocation.Z += GroundClearance;
	}

	/*
	 * 参考代码：以后若投掷部署物需要贴合复杂地形、斜坡或台阶，可恢复这段可见性地面检测。
	 * 当前医疗站按产品要求只取胶囊脚底，故保留注释供类似需求复用，不参与运行时路径。
	 *
	 * UWorld* World = AvatarActor->GetWorld();
	 * FHitResult GroundHit;
	 * FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MedicalStationPlacement), false, AvatarActor);
	 * constexpr float TraceStartHeight = 100.f;
	 * const FVector TraceStart = AvatarLocation + FVector::UpVector * TraceStartHeight;
	 * const FVector TraceEnd = AvatarLocation - FVector::UpVector * GroundTraceDistance;
	 * if (World && World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd,
	 *     ECC_Visibility, QueryParams))
	 * {
	 *     GroundLocation = GroundHit.ImpactPoint + GroundHit.ImpactNormal * GroundClearance;
	 * }
	 */

	return FTransform(FRotator::ZeroRotator, GroundLocation);
}
