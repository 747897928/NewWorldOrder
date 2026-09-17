// 女主被动：医疗专精，默认提高治疗输出/受益
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Passive_MedicalExpertise.h"

#include "ShootGameplayTags.h"
#include "AbilitySystem/Effects/ShootEffect_Passive_MedicalExpertise.h"
#include "AbilitySystemComponent.h"

UShootGA_Passive_MedicalExpertise::UShootGA_Passive_MedicalExpertise()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	ActivationPolicy = EShootAbilityActivationPolicy::OnSpawn;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Female.Passive.MedicalExpertise"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Passive1);
	SetAssetTags(AssetTags);
	// 被动由 AbilitySet 授予；OnSpawn 策略在 ASC 绑定 Avatar 后自动激活。

	HealingBuffEffectClass = UShootEffect_Passive_MedicalExpertise::StaticClass();
	HealingDoneBonus = 0.2f;
	HealingReceivedBonus = 0.f;
}

void UShootGA_Passive_MedicalExpertise::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (HealingBuffHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(HealingBuffHandle);
			HealingBuffHandle.Invalidate();
		}

		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		Ctx.AddSourceObject(this);

		if (HealingBuffEffectClass)
		{
			const float AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
			if (FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(HealingBuffEffectClass, AbilityLevel, Ctx); Spec.IsValid())
			{
				// 按等级注入治疗倍率与等级标记，供医疗站/救援逻辑判定使用
				float DoneMultiplier = 1.2f;
				float ReceivedMultiplier = 1.0f;
				if (AbilityLevel >= 2.f)
				{
					DoneMultiplier = 1.4f;
				}
				if (AbilityLevel >= 3.f)
				{
					DoneMultiplier = 1.6f;
				}

				const FGameplayTag DoneTag = FGameplayTag::RequestGameplayTag(FName("SetByCaller.HealingDoneMultiplier"), false);
				const FGameplayTag ReceivedTag = FGameplayTag::RequestGameplayTag(FName("SetByCaller.HealingReceivedMultiplier"), false);
				if (DoneTag.IsValid())
				{
					FShootGameplayTags::SetSetByCallerMagnitude(*Spec.Data.Get(), DoneTag, DoneMultiplier);
				}
				if (ReceivedTag.IsValid())
				{
					FShootGameplayTags::SetSetByCallerMagnitude(*Spec.Data.Get(), ReceivedTag, ReceivedMultiplier);
				}

				if (AbilityLevel >= 2.f)
				{
					const FGameplayTag Lv2Tag = FGameplayTag::RequestGameplayTag(FName("Status.MedicalExpertise.Lv2"), false);
					if (Lv2Tag.IsValid())
					{
						Spec.Data->DynamicGrantedTags.AddTag(Lv2Tag);
					}
				}
				if (AbilityLevel >= 3.f)
				{
					const FGameplayTag Lv3Tag = FGameplayTag::RequestGameplayTag(FName("Status.MedicalExpertise.Lv3"), false);
					if (Lv3Tag.IsValid())
					{
						Spec.Data->DynamicGrantedTags.AddTag(Lv3Tag);
					}
				}

				// 标记套件 Tag，便于在性别切换时移除该套件相关的长期效果
				Spec.Data->DynamicGrantedTags.AddTag(FShootGameplayTags::Get().Abilities_Kit_Protagonist_Female);
				HealingBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
	// TODO(主角-MedicalExpertise-Design): 仍需接入其他治疗来源（非医疗站）时的免疫致死与范围加成策略。
}

void UShootGA_Passive_MedicalExpertise::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (HealingBuffHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->RemoveActiveGameplayEffect(HealingBuffHandle);
		}
		HealingBuffHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
