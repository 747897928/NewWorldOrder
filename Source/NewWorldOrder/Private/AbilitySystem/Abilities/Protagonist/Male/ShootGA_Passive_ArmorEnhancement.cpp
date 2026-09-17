// 被动：装甲强化，默认施加护盾容量/减伤加成（可覆盖 GE，持续或永久）
#include "AbilitySystem/Abilities/Protagonist/Male/ShootGA_Passive_ArmorEnhancement.h"

#include "AbilitySystem/Effects/ShootEffect_Passive_ArmorEnhancement.h"
#include "AbilitySystemComponent.h"
#include "ShootGameplayTags.h"

UShootGA_Passive_ArmorEnhancement::UShootGA_Passive_ArmorEnhancement()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
	ActivationPolicy = EShootAbilityActivationPolicy::OnSpawn;

	const FShootGameplayTags& Tags = FShootGameplayTags::Get();
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Male.Passive.ArmorEnhancement"), false));
	AssetTags.AddTag(Tags.Ability_Type_Skill_Passive2);
	SetAssetTags(AssetTags);
	// 被动由 AbilitySet 授予；OnSpawn 策略在 ASC 绑定 Avatar 后自动激活。

	ArmorEnhancementBuffEffectClass = UShootEffect_Passive_ArmorEnhancement::StaticClass();
	ShieldBonusValue = 0.2f; // +20% 护盾
	DamageReductionValue = 0.f; // 如需额外减伤可设 >0
	BuffDuration = -1.f; // 负值视为永久
}

void UShootGA_Passive_ArmorEnhancement::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
	{
		if (ArmorBuffHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ArmorBuffHandle);
			ArmorBuffHandle.Invalidate();
		}

		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		Ctx.AddSourceObject(this);

		if (ArmorEnhancementBuffEffectClass)
		{
			if (FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ArmorEnhancementBuffEffectClass, 1.f, Ctx); Spec.IsValid())
			{
				const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
				Spec.Data->DynamicGrantedTags.AddTag(GameplayTags.Abilities_Kit_Protagonist_Male);

				if (BuffDuration > 0.f)
				{
					Spec.Data->SetDuration(BuffDuration, true);
				}

				FShootGameplayTags::SetSetByCallerMagnitude(
					*Spec.Data.Get(), GameplayTags.SetByCaller_ShieldCapacityBonus, ShieldBonusValue);
				FShootGameplayTags::SetSetByCallerMagnitude(
					*Spec.Data.Get(), GameplayTags.SetByCaller_DamageReductionBonus, DamageReductionValue);

				ArmorBuffHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
	// TODO: ArmorEnhancement - 监听护盾破碎事件并触发爆炸/击退/减速效果，按设计文档补齐。
}

void UShootGA_Passive_ArmorEnhancement::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ArmorBuffHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->RemoveActiveGameplayEffect(ArmorBuffHandle);
		}
		ArmorBuffHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
