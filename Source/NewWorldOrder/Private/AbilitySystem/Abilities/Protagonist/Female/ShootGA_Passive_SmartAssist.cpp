// 女主被动：智能辅助，默认占位：若有无人机 Buff GE 则应用，可扩展智能模式/爆炸等
#include "AbilitySystem/Abilities/Protagonist/Female/ShootGA_Passive_SmartAssist.h"

#include "ShootGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/ShootEffect_Passive_SmartAssist.h"

UShootGA_Passive_SmartAssist::UShootGA_Passive_SmartAssist()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	ActivationPolicy = EShootAbilityActivationPolicy::OnSpawn;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Female.Passive.SmartAssist"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Passive2);
	SetAssetTags(AssetTags);
	// 被动由 AbilitySet 授予；OnSpawn 策略在 ASC 绑定 Avatar 后自动激活。

	DroneBuffEffectClass = UShootEffect_Passive_SmartAssist::StaticClass();
}

void UShootGA_Passive_SmartAssist::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (DroneBuffEffectClass)
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			if (DroneBuffHandle.IsValid())
			{
				ASC->RemoveActiveGameplayEffect(DroneBuffHandle);
				DroneBuffHandle.Invalidate();
			}
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			Ctx.AddSourceObject(this);
			if (FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DroneBuffEffectClass, 1.f, Ctx); Spec.IsValid())
			{
				Spec.Data->DynamicGrantedTags.AddTag(FShootGameplayTags::Get().Abilities_Kit_Protagonist_Female);
				DroneBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
	// TODO(主角-SmartAssist-DroneAI): 生成无人机、跟随/输出/自爆逻辑，分 Assist/自爆模式，属性配置化。
}

void UShootGA_Passive_SmartAssist::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (DroneBuffHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->RemoveActiveGameplayEffect(DroneBuffHandle);
		}
		DroneBuffHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
