// 治疗包执行能力：拾取即用，不占 QuickBar 槽位、不生成武器实例（对齐 Lyra B_HealPickup + GE_InstantHeal 的即时治疗模式）。
#include "Interaction/Abilities/ShootGA_Interaction_HealthPack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/Effects/ShootEffect_HealInstant.h"
#include "GameFramework/Pawn.h"
#include "Interaction/ShootHealthpackPickup.h"
#include "ShootGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootGA_Interaction_HealthPack)

UShootGA_Interaction_HealthPack::UShootGA_Interaction_HealthPack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	// 与 RefillAmmo/Collect 相同：客户端预测激活，服务器重新校验并唯一写入治疗与销毁。
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UShootGA_Interaction_HealthPack::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 对齐 Lyra LyraGameplayAbility_Interact：施法者直接用能力 Avatar，不从事件 Payload 提取；
	// 只有交互目标需要从 const Payload 中取出（Lyra 官方同样使用 const_cast 处理）。
	APawn* InstigatorPawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	AShootHealthpackPickup* Pickup = TriggerEventData
		? const_cast<AShootHealthpackPickup*>(Cast<AShootHealthpackPickup>(TriggerEventData->Target.Get()))
		: nullptr;

	// 服务器权威：客户端预测只负责让本地感知即时，这里必须再次校验（玩家、范围、未满血）。
	if (ActorInfo && ActorInfo->IsNetAuthority() && InstigatorPawn && Pickup &&
		Pickup->CanUsePawn(InstigatorPawn))
	{
		UAbilitySystemComponent* InstigatorASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorPawn);
		if (InstigatorASC)
		{
			const float HealthBefore = InstigatorASC->GetNumericAttribute(UShootAttributeSet::GetHealthAttribute());
			FGameplayEffectContextHandle EffectContext = InstigatorASC->MakeEffectContext();
			EffectContext.AddSourceObject(this);
			EffectContext.AddInstigator(InstigatorPawn, InstigatorPawn);
			FGameplayEffectSpecHandle HealSpec = InstigatorASC->MakeOutgoingSpec(
				UShootEffect_HealInstant::StaticClass(), GetAbilityLevel(), EffectContext);
			if (HealSpec.IsValid() && HealSpec.Data.IsValid())
			{
				FShootGameplayTags::SetSetByCallerMagnitude(
					*HealSpec.Data.Get(), FShootGameplayTags::Get().SetByCaller_Heal, HealAmount);
				InstigatorASC->ApplyGameplayEffectSpecToSelf(*HealSpec.Data.Get());

				const float HealthAfter = InstigatorASC->GetNumericAttribute(UShootAttributeSet::GetHealthAttribute());
				const float ActualHeal = FMath::Max(HealthAfter - HealthBefore, 0.0f);
				if (ActualHeal > KINDA_SMALL_NUMBER)
				{
					FGameplayCueParameters CueParameters;
					CueParameters.Location = InstigatorPawn->GetActorLocation();
					CueParameters.Instigator = InstigatorPawn;
					CueParameters.EffectCauser = Pickup;
					CueParameters.SourceObject = this;
					CueParameters.RawMagnitude = ActualHeal;
					CueParameters.NormalizedMagnitude = InstigatorASC->GetNumericAttribute(
						UShootAttributeSet::GetMaxHealthAttribute()) > KINDA_SMALL_NUMBER
						? FMath::Clamp(ActualHeal / InstigatorASC->GetNumericAttribute(
							UShootAttributeSet::GetMaxHealthAttribute()), 0.0f, 1.0f)
						: 0.0f;
					InstigatorASC->ExecuteGameplayCue(
						FGameplayTag::RequestGameplayTag(FName("GameplayCue.Character.Heal"), false), CueParameters);
					Pickup->ConsumeByPawn(InstigatorPawn);
				}
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
