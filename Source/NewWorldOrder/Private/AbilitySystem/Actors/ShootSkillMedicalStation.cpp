// 通用医疗站：周期治疗范围内角色，默认每秒 5% MaxHP，可蓝图扩展视觉
#include "AbilitySystem/Actors/ShootSkillMedicalStation.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/Effects/ShootEffect_HealInstant.h"
#include "AbilitySystem/Effects/ShootEffect_ImmuneDeath.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "GameplayTagContainer.h"
#include "Components/SphereComponent.h"
#include "ShootGameplayTags.h"
#include "Teams/LyraTeamAgentInterface.h"
#include "GameFramework/Pawn.h"

AShootSkillMedicalStation::AShootSkillMedicalStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	Radius = 500.f;
	EffectiveRadius = Radius;
	Duration = 15.f;
	TickInterval = 1.f;
	HealPercentPerSecond = 0.05f;
	HealEffectClass = UShootEffect_HealInstant::StaticClass();

	SetRootComponent(CreateDefaultSubobject<USphereComponent>(TEXT("Sphere")));
	if (USphereComponent* Sphere = Cast<USphereComponent>(GetRootComponent()))
	{
		Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sphere->SetSphereRadius(EffectiveRadius);
	}
}

void AShootSkillMedicalStation::BeginPlay()
{
	Super::BeginPlay();
	LifeTimeElapsed = 0.f;

	if (!HasAuthority())
	{
		// 仅服务器进行治疗判定；表现层可在蓝图中通过 Tick/Cue/组件补完。
		return;
	}

	// 根据施法者的被动标签动态调整医疗站范围
	if (APawn* SourcePawn = GetInstigator())
	{
		if (UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourcePawn))
		{
			const FGameplayTag Lv2Tag = FGameplayTag::RequestGameplayTag(FName("Status.MedicalExpertise.Lv2"), false);
			const FGameplayTag Lv3Tag = FGameplayTag::RequestGameplayTag(FName("Status.MedicalExpertise.Lv3"), false);
			if (Lv3Tag.IsValid() && SourceASC->HasMatchingGameplayTag(Lv3Tag))
			{
				EffectiveRadius = Radius * 1.5f;
			}
			else if (Lv2Tag.IsValid() && SourceASC->HasMatchingGameplayTag(Lv2Tag))
			{
				EffectiveRadius = Radius * 1.3f;
			}
		}
	}

	if (USphereComponent* Sphere = Cast<USphereComponent>(GetRootComponent()))
	{
		Sphere->SetSphereRadius(EffectiveRadius);
	}

	// 生命周期由服务器 Actor 自身精确控制；Destroy 会复制到客户端并连同蓝图粒子组件一起回收。
	// HealTick 的累计时间仅保留为防御性兜底，不能再作为唯一销毁来源。
	if (Duration > 0.f)
	{
		SetLifeSpan(Duration);
	}
	GetWorldTimerManager().SetTimer(TickTimerHandle, this, &ThisClass::HealTick, TickInterval, true, 0.f);
}

void AShootSkillMedicalStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(TickTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AShootSkillMedicalStation::HealTick()
{
	if (!HasAuthority())
	{
		return;
	}

	LifeTimeElapsed += TickInterval;
	if (Duration <= 0.f || LifeTimeElapsed >= Duration)
	{
		GetWorldTimerManager().ClearTimer(TickTimerHandle);
		Destroy();
		return;
	}

	OnHealTickVisual();

	UWorld* World = GetWorld();
	if (!World) return;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MedicalStation), false, this);
	const FVector Center = GetActorLocation();
	bool bHit = World->OverlapMultiByObjectType(
		Overlaps,
		Center,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(EffectiveRadius),
		Params);

	if (!bHit) return;

	// SetByCaller.Heal（直接请求 Tag，避免依赖全局标签单例初始化时机）
	const FGameplayTag HealTag = FGameplayTag::RequestGameplayTag(FName("SetByCaller.Heal"), /*ErrorIfNotFound=*/false);

	auto GetTeamIdFromPawn = [](const APawn* Pawn) -> FGenericTeamId
	{
		if (!Pawn)
		{
			return FGenericTeamId::NoTeam;
		}
		if (const ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(Pawn->GetController()))
		{
			return TeamAgent->GetGenericTeamId();
		}
		return FGenericTeamId::NoTeam;
	};

	APawn* SourcePawn = GetInstigator();
	if (!SourcePawn)
	{
		SourcePawn = Cast<APawn>(GetOwner());
	}
	const FGenericTeamId SourceTeamID = GetTeamIdFromPawn(SourcePawn);
	UAbilitySystemComponent* SourceASC = SourcePawn ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourcePawn) : nullptr;
	const FGameplayTag Lv3Tag = FGameplayTag::RequestGameplayTag(FName("Status.MedicalExpertise.Lv3"), false);

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* TargetActor = Result.GetActor();
		if (!TargetActor) continue;

		const APawn* TargetPawn = Cast<APawn>(TargetActor);
		const FGenericTeamId TargetTeamID = GetTeamIdFromPawn(TargetPawn);

		// 只治疗友军：阵营必须有效且一致
		if (SourceTeamID == FGenericTeamId::NoTeam || TargetTeamID == FGenericTeamId::NoTeam || SourceTeamID.GetId() != TargetTeamID.GetId())
		{
			continue;
		}

		if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor))
		{
			// 若 HealEffectClass 为空，回退到通用即时治疗 GE，避免无治疗效果
			TSubclassOf<UGameplayEffect> EffectiveHealClass = HealEffectClass;
			if (!EffectiveHealClass)
			{
				EffectiveHealClass = UShootEffect_HealInstant::StaticClass();
			}

			FGameplayEffectContextHandle Ctx = TargetASC->MakeEffectContext();
			Ctx.AddSourceObject(this);
			FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(EffectiveHealClass, 1.f, Ctx);
			if (SpecHandle.IsValid())
			{
				const float TargetMaxHealth = TargetASC->GetNumericAttribute(UShootAttributeSet::GetMaxHealthAttribute());
				const float TargetHealingReceived = TargetASC->GetNumericAttribute(UShootAttributeSet::GetHealingReceivedMultiplierAttribute());
				const float SourceHealingDone = SourceASC ? SourceASC->GetNumericAttribute(UShootAttributeSet::GetHealingDoneMultiplierAttribute()) : 1.0f;

				float HealAmount = TargetMaxHealth * HealPercentPerSecond * TickInterval;
				HealAmount *= FMath::Max(TargetHealingReceived, 0.f);
				HealAmount *= FMath::Max(SourceHealingDone, 0.f);

				FShootGameplayTags::SetSetByCallerMagnitude(*SpecHandle.Data.Get(), HealTag, HealAmount);
				TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}

			// 医疗专精满级时，为被治疗者附加 10 秒免疫致死
			if (SourceASC && Lv3Tag.IsValid() && SourceASC->HasMatchingGameplayTag(Lv3Tag))
			{
				FGameplayEffectContextHandle ProtectCtx = TargetASC->MakeEffectContext();
				ProtectCtx.AddSourceObject(this);
				FGameplayEffectSpecHandle ProtectSpec = TargetASC->MakeOutgoingSpec(UShootEffect_ImmuneDeath::StaticClass(), 1.f, ProtectCtx);
				if (ProtectSpec.IsValid())
				{
					ProtectSpec.Data->SetDuration(10.0f, true);
					TargetASC->ApplyGameplayEffectSpecToSelf(*ProtectSpec.Data.Get());
				}
			}
		}
	}
}
