// 通用爆炸 Actor：延时 → SphereOverlap → 应用伤害/控制/易伤效果，默认使用 C++ SetByCaller 伤害
#include "AbilitySystem/Actors/ShootSkillExplosionActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "ShootGameplayTags.h"
#include "Components/SphereComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"

AShootSkillExplosionActor::AShootSkillExplosionActor()
{
	PrimaryActorTick.bCanEverTick = false;
	FuseTime = 0.3f;
	Radius = 300.f;
	Damage = 150.f;
	VulnerableDuration = 5.f;
	StunDuration = 0.f;
	KnockbackStrength = 0.f;
	SetRootComponent(CreateDefaultSubobject<USphereComponent>(TEXT("Sphere")));
	if (USphereComponent* Sphere = Cast<USphereComponent>(GetRootComponent()))
	{
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sphere->SetSphereRadius(Radius);
	}
}

void AShootSkillExplosionActor::ConfigureExplosion(float InDamage, float InRadius, float InFuseTime, float InVulnerableDuration, float InStunDuration,
	float InKnockbackStrength,
	TSubclassOf<UGameplayEffect> InDamageEffect, TSubclassOf<UGameplayEffect> InControlEffect, TSubclassOf<UGameplayEffect> InVulnerableEffect)
{
	Damage = InDamage;
	Radius = InRadius;
	FuseTime = InFuseTime;
	VulnerableDuration = InVulnerableDuration;
	StunDuration = InStunDuration;
	KnockbackStrength = InKnockbackStrength;
	if (USphereComponent* Sphere = Cast<USphereComponent>(GetRootComponent()))
	{
		Sphere->SetSphereRadius(Radius);
	}
	if (InDamageEffect)
	{
		DamageEffectClass = InDamageEffect;
	}
	if (InControlEffect)
	{
		ControlEffectClass = InControlEffect;
	}
	if (InVulnerableEffect)
	{
		VulnerableEffectClass = InVulnerableEffect;
	}
}

void AShootSkillExplosionActor::BeginPlay()
{
	Super::BeginPlay();
	if (FuseTime > 0.f)
	{
		GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &ThisClass::TriggerExplosion, FuseTime, false);
	}
	else
	{
		TriggerExplosion();
	}
}

void AShootSkillExplosionActor::TriggerExplosion()
{
	Explode();
	Destroy();
}

void AShootSkillExplosionActor::Explode()
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkillExplosion), false, this);
	const FVector Center = GetActorLocation();
	bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Radius),
		Params);

	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* TargetActor = Result.GetActor();
		if (!TargetActor) continue;

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			// 伤害
			if (DamageEffectClass)
			{
				FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
				Ctx.AddSourceObject(this);
				FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Ctx);
				if (SpecHandle.IsValid())
				{
					FShootGameplayTags::SetSetByCallerMagnitude(
						*SpecHandle.Data.Get(), GameplayTags.SetByCaller_Damage, Damage);
					TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}

			// 控制：用 Vulnerable/Status 标签作为示例，实际可替换为外部 GE
			if (VulnerableEffectClass && VulnerableDuration > 0.f)
			{
				FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
				Ctx.AddSourceObject(this);
				FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(VulnerableEffectClass, 1.f, Ctx);
				if (SpecHandle.IsValid())
				{
					SpecHandle.Data->SetDuration(VulnerableDuration, true);
					TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}

			if (ControlEffectClass && StunDuration > 0.f)
			{
				FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
				Ctx.AddSourceObject(this);
				FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(ControlEffectClass, 1.f, Ctx);
				if (SpecHandle.IsValid())
				{
					SpecHandle.Data->SetDuration(StunDuration, true);
					TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}
		}

		// 击退逻辑：优先对 Character 使用 LaunchCharacter，其次对物理组件施加径向冲量
		if (KnockbackStrength > 0.f)
		{
			const FVector ToTarget = (TargetActor->GetActorLocation() - Center);
			const float Distance = FMath::Max(ToTarget.Size(), 1.f);
			const float Falloff = FMath::Clamp(1.f - (Distance / FMath::Max(Radius, 1.f)), 0.f, 1.f);
			const FVector KnockDir = ToTarget.GetSafeNormal();
			const float FinalStrength = KnockbackStrength * Falloff;

			if (ACharacter* Character = Cast<ACharacter>(TargetActor))
			{
				// LaunchCharacter 由服务器触发，客户端收到位移复制
				Character->LaunchCharacter(KnockDir * FinalStrength, true, true);
			}
			else if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
			{
				if (RootPrimitive->IsSimulatingPhysics())
				{
					RootPrimitive->AddRadialImpulse(Center, Radius, FinalStrength, ERadialImpulseFalloff::RIF_Linear, true);
				}
			}
		}
	}
}
