// 仅用于 TestMap 的可见持续伤害区域：踩入自动施加，离开自动移除。
#include "Testing/ShootTestDamageHazardVolume.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Effects/ShootEffect_TestHazardDamagePeriodic.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootTestDamageHazardVolume)

AShootTestDamageHazardVolume::AShootTestDamageHazardVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DamageVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageVolume"));
	DamageVolume->SetupAttachment(SceneRoot);
	DamageVolume->SetBoxExtent(FVector(180.0f, 180.0f, 60.0f));
	DamageVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageVolume->SetCollisionResponseToAllChannels(ECR_Overlap);
	DamageVolume->SetGenerateOverlapEvents(true);

	HazardVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HazardVFX"));
	HazardVFX->SetupAttachment(SceneRoot);
	HazardVFX->SetAutoActivate(false);
	HazardVFX->SetRelativeScale3D(HazardVFXScale);

	// 默认使用测试 GE；蓝图仍可覆盖为其他测试伤害 GE，正式伤害系统不依赖本类。
	DamageGameplayEffectClass = UShootEffect_TestHazardDamagePeriodic::StaticClass();
}

void AShootTestDamageHazardVolume::BeginPlay()
{
	Super::BeginPlay();

	if (DamageVolume)
	{
		DamageVolume->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnHazardBeginOverlap);
		DamageVolume->OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnHazardEndOverlap);
	}

	if (HazardVFX)
	{
		HazardVFX->SetAsset(HazardVFXSystem);
		HazardVFX->SetRelativeScale3D(HazardVFXScale);
		if (HazardVFXSystem)
		{
			HazardVFX->Activate(true);
		}
	}
}

void AShootTestDamageHazardVolume::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
	{
		RemoveAllDamageEffects();
	}

	Super::EndPlay(EndPlayReason);
}

void AShootTestDamageHazardVolume::OnHazardBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComponent;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;

	if (!HasAuthority())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || (bOnlyPlayerControlled && !Pawn->IsPlayerControlled()))
	{
		return;
	}

	ApplyDamageEffectToPawn(Pawn);
}

void AShootTestDamageHazardVolume::OnHazardEndOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex)
{
	(void)OverlappedComponent;
	(void)OtherComponent;
	(void)OtherBodyIndex;

	if (HasAuthority())
	{
		RemoveDamageEffectFromPawn(Cast<APawn>(OtherActor));
	}
}

void AShootTestDamageHazardVolume::ApplyDamageEffectToPawn(APawn* Pawn)
{
	if (!Pawn || !DamageGameplayEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!TargetASC || ActiveDamageEffects.Contains(TargetASC))
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = TargetASC->MakeEffectContext();
	EffectContext.AddSourceObject(this);
	// 环境火焰不是玩家对自己造成伤害；使用火焰 Actor 作为来源，避免被友伤规则判定为同队自伤 0 倍。
	EffectContext.AddInstigator(this, this);

	const FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(
		DamageGameplayEffectClass, DamageGameplayEffectLevel, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	const FActiveGameplayEffectHandle ActiveHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (ActiveHandle.IsValid())
	{
		ActiveDamageEffects.Add(TargetASC, ActiveHandle);
	}
}

void AShootTestDamageHazardVolume::RemoveDamageEffectFromPawn(APawn* Pawn)
{
	if (!Pawn)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
	if (!TargetASC)
	{
		return;
	}

	if (FActiveGameplayEffectHandle* ActiveHandle = ActiveDamageEffects.Find(TargetASC))
	{
		TargetASC->RemoveActiveGameplayEffect(*ActiveHandle);
		ActiveDamageEffects.Remove(TargetASC);
	}
}

void AShootTestDamageHazardVolume::RemoveAllDamageEffects()
{
	for (const TPair<TWeakObjectPtr<UAbilitySystemComponent>, FActiveGameplayEffectHandle>& Entry : ActiveDamageEffects)
	{
		if (UAbilitySystemComponent* TargetASC = Entry.Key.Get())
		{
			TargetASC->RemoveActiveGameplayEffect(Entry.Value);
		}
	}

	ActiveDamageEffects.Empty();
}
