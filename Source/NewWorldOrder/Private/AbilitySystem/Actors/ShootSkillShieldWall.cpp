// 通用护盾墙：为拥有者提供护盾/减伤，可蓝图扩展反伤/破碎爆炸
#include "AbilitySystem/Actors/ShootSkillShieldWall.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/ShootEffect_ShieldWall_SetByCaller.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "ShootGameplayTags.h"

AShootSkillShieldWall::AShootSkillShieldWall()
{
	PrimaryActorTick.bCanEverTick = false;
	Duration = 5.f;
	DamageReduction = 0.8f; // 额外减伤 80%（可覆盖）
	ShieldBonus = 0.f;
	ShieldEffectClass = UShootEffect_ShieldWall_SetByCaller::StaticClass();
}

void AShootSkillShieldWall::InitShield(AActor* InOwnerActor)
{
	OwnerActor = InOwnerActor;
	OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InOwnerActor);
}

void AShootSkillShieldWall::BeginPlay()
{
	Super::BeginPlay();
	ApplyShield();
	OnShieldActivated();
}

void AShootSkillShieldWall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RemoveShield();
	Super::EndPlay(EndPlayReason);
}

void AShootSkillShieldWall::ApplyShield()
{
	if (!OwnerASC.IsValid())
	{
		return;
	}

	// ShieldEffectClass 可由外部覆盖，默认使用 SetByCaller 版本以注入动态数值
	TSubclassOf<UGameplayEffect> EffectClass = ShieldEffectClass;
	if (!EffectClass)
	{
		EffectClass = UShootEffect_ShieldWall_SetByCaller::StaticClass();
	}

	FGameplayEffectContextHandle Ctx = OwnerASC->MakeEffectContext();
	Ctx.AddSourceObject(this);
	FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(EffectClass, 1.f, Ctx);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetDuration(Duration, true);
		// 用 SetByCaller 注入护盾与减伤数值（单位为“加成”）
		FShootGameplayTags::SetSetByCallerMagnitude(
			*SpecHandle.Data.Get(), FShootGameplayTags::Get().SetByCaller_ShieldCapacityBonus, ShieldBonus);
		FShootGameplayTags::SetSetByCallerMagnitude(
			*SpecHandle.Data.Get(), FShootGameplayTags::Get().SetByCaller_DamageReductionBonus, DamageReduction);
		ActiveShieldHandle = OwnerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void AShootSkillShieldWall::RemoveShield()
{
	if (OwnerASC.IsValid() && ActiveShieldHandle.IsValid())
	{
		OwnerASC->RemoveActiveGameplayEffect(ActiveShieldHandle);
		ActiveShieldHandle.Invalidate();
		OnShieldBroken();
	}
}
