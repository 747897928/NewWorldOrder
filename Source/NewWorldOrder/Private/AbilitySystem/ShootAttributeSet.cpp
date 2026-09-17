

// Copyright ZhaoYiJie


#include "AbilitySystem/ShootAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "ShootGameplayTags.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameplayCueFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameModes/ShootGameModeBase.h"
#include "Interface/CombatInterface.h"
#include "Physics/PhysicalMaterialWithTags.h"
#include "Player/ShootPlayerController.h"

UShootAttributeSet::UShootAttributeSet()
{
	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();

	/* Primary Attributes */
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Strength, GetStrengthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Vitality, GetVitalityAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Agility, GetAgilityAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Primary_Perception, GetPerceptionAttribute);

	/* Secondary Attributes */
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Armor, GetArmorAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ArmorPenetration, GetArmorPenetrationAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_CriticalHitChance, GetCriticalHitChanceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_CriticalHitDamage, GetCriticalHitDamageAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_CriticalHitResistance, GetCriticalHitResistanceAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxHealth, GetMaxHealthAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ShieldCapacity, GetShieldCapacityAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Secondary_DamageReduction, GetDamageReductionAttribute);

	/* Combat Buff Attributes */
	TagsToAttributes.Add(GameplayTags.Attributes_Combat_MoveSpeedMultiplier, GetMoveSpeedMultiplierAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Combat_FireRateMultiplier, GetFireRateMultiplierAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Combat_ReloadSpeedMultiplier, GetReloadSpeedMultiplierAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Combat_HealingDoneMultiplier, GetHealingDoneMultiplierAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Combat_HealingReceivedMultiplier, GetHealingReceivedMultiplierAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Combat_ShieldCapacityBonus, GetShieldCapacityBonusAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Combat_DamageReductionBonus, GetDamageReductionBonusAttribute);

	/* Ultimate Attributes */
	TagsToAttributes.Add(GameplayTags.Attributes_Combat_UltimateCharge, GetUltimateChargeAttribute);
	TagsToAttributes.Add(GameplayTags.Attributes_Combat_UltimateChargeMax, GetUltimateChargeMaxAttribute);

	// 默认基值：Multiplier 系列 1.0，Bonus 系列 0.0，大招上限 100
	MoveSpeedMultiplier = 1.0f;
	FireRateMultiplier = 1.0f;
	ReloadSpeedMultiplier = 1.0f;
	HealingDoneMultiplier = 1.0f;
	HealingReceivedMultiplier = 1.0f;
	ShieldCapacityBonus = 0.0f;
	DamageReductionBonus = 0.0f;
	UltimateCharge = 0.0f;
	UltimateChargeMax = 100.0f;
}

void UShootAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Primary Attributes

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Vitality, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Agility, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Perception, COND_None, REPNOTIFY_Always);

	// Secondary Attributes

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CriticalHitChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CriticalHitDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, CriticalHitResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, ShieldCapacity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, DamageReduction, COND_None, REPNOTIFY_Always);

	// Combat Buff Attributes
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, MoveSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, FireRateMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, ReloadSpeedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, HealingDoneMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, HealingReceivedMultiplier, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, ShieldCapacityBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, DamageReductionBonus, COND_None, REPNOTIFY_Always);

	// Ultimate Attributes
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, UltimateCharge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, UltimateChargeMax, COND_None, REPNOTIFY_Always);

	// Vital Attributes

	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass, Health, COND_None, REPNOTIFY_Always);
}

void UShootAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	UE_LOG(LogTemp, Warning, TEXT("PreChange: Attribute '%s'"), *Attribute.AttributeName);
	// Clamp Health to [0, MaxHealth]
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetShieldCapacity());
	}
	else if (Attribute == GetUltimateChargeAttribute())
	{
		const float Max = FMath::Max(GetUltimateChargeMax(), 0.0f);
		NewValue = FMath::Clamp(NewValue, 0.0f, Max);
	}
	else if (Attribute == GetUltimateChargeMaxAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetMoveSpeedMultiplierAttribute()
		|| Attribute == GetFireRateMultiplierAttribute()
		|| Attribute == GetReloadSpeedMultiplierAttribute()
		|| Attribute == GetHealingDoneMultiplierAttribute()
		|| Attribute == GetHealingReceivedMultiplierAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	Super::PreAttributeChange(Attribute, NewValue);
}

void UShootAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FShootEffectContext Context;
	SetEffectContext(Data, Context);

	//if(Props.TargetCharacter->Implements<UCombatInterface>() && ICombatInterface::Execute_IsDead(Props.TargetCharacter)) return;

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp(GetShield(), 0.0f, GetShieldCapacity()));
	}

	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		HandleIncomingDamage(Context);
	}
}

void UShootAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	//在单人游戏中，您可以通过覆盖 PostAttributeChange 来检测属性集类中的所有属性值变化。
	//在联网游戏中，属性值的更改也可以通过网络复制实现：服务器更改值，客户端接收新值。PostAttributeChange 仅在本地调用，因此如果服务器应用了 GE，
	//客户端不会执行 PostAttributeChange。客户端可以通过指定 OnRep 回调来处理这种情况，该回调用于值通过复制更改的情况。
	if (Attribute == GetMaxHealthAttribute() && bTopOffHealth)
	{
		SetHealth(GetMaxHealth());
		bTopOffHealth = false;
	}
	else if (Attribute == GetShieldCapacityAttribute())
	{
		// 初始属性链 Primary -> Secondary -> Vital 中，Secondary 先给出容量。
		// 仅当当前护盾尚未初始化时补满，后续 Potion/伤害对 Shield 的改动不会被容量刷新覆盖。
		if (OldValue <= 0.f && GetShield() <= 0.f && NewValue > 0.f)
		{
			SetShield(NewValue);
		}
		else if (GetShield() > NewValue)
		{
			SetShield(NewValue);
		}
	}
}

void UShootAttributeSet::SetEffectContext(const FGameplayEffectModCallbackData& Data, FShootEffectContext& Context) const
{
	const FGameplayEffectContextHandle& EffectContext = Data.EffectSpec.GetContext();
	Context.SourceASC = EffectContext.GetOriginalInstigatorAbilitySystemComponent();
	Context.SourceActor = EffectContext.GetOriginalInstigator();
	if (const APawn* Pawn = Cast<APawn>(Context.SourceActor.Get()))
	{
		Context.SourceController = Pawn->GetController();
	}
	if (const FHitResult* HitResult = EffectContext.GetHitResult())
	{
		Context.HitResult = *HitResult;
		Context.bHasHitResult = true;
	}

	if (const FGameplayTagContainer* SourceTags = Data.EffectSpec.CapturedSourceTags.GetAggregatedTags())
	{
		Context.SourceTags = *SourceTags;
	}
	if (const FGameplayTagContainer* TargetTags = Data.EffectSpec.CapturedTargetTags.GetAggregatedTags())
	{
		Context.TargetTags = *TargetTags;
	}

	if (IsValid(Context.SourceASC) && Context.SourceASC->AbilityActorInfo.IsValid())
	{
		Context.SourceActor = Context.SourceASC->AbilityActorInfo->AvatarActor.Get();
		Context.SourceController = Context.SourceASC->AbilityActorInfo->PlayerController.Get();
		if (!Context.SourceController && Context.SourceActor)
		{
			if (const APawn* Pawn = Cast<APawn>(Context.SourceActor))
			{
				Context.SourceController = Pawn->GetController();
			}
		}
		if (Context.SourceController)
		{
			Context.SourceCharacter = Cast<ACharacter>(Context.SourceController->GetPawn());
		}
	}

	// 属性集回调中的 Target 本身就是最终接收效果的 ASC，不能只依赖 AbilityActorInfo。
	// 环境伤害没有玩家施法者时，ActorInfo 可能尚未完整建立，但护盾/生命仍必须走同一条权威结算链。
	Context.TargetASC = &Data.Target;
	if (Data.Target.AbilityActorInfo.IsValid())
	{
		Context.TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Context.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
	}
	else
	{
		Context.TargetActor = Data.Target.GetAvatarActor();
	}

	Context.TargetCharacter = Cast<ACharacter>(Context.TargetActor);
	if (!Context.TargetController && Context.TargetActor)
	{
		if (const APawn* Pawn = Cast<APawn>(Context.TargetActor))
		{
			Context.TargetController = Pawn->GetController();
		}
	}
}

void UShootAttributeSet::HandleIncomingDamage(const FShootEffectContext& Context)
{
	const float LocalIncomingDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);

	const bool bWasAlive = GetHealth() > 0.f;
	if (LocalIncomingDamage <= 0.f || !Context.TargetASC || !bWasAlive)
	{
		return;
	}

	float DamageToApply = LocalIncomingDamage;

	if (UWorld* World = Context.TargetActor ? Context.TargetActor->GetWorld() : nullptr)
	{
		if (AGameModeBase* GameModeBase = World->GetAuthGameMode())
		{
			if (AShootGameModeBase* ShootGM = Cast<AShootGameModeBase>(GameModeBase))
			{
				const float FriendlyScalar = ShootGM->GetFriendlyFireScalarForActors(Context.SourceActor.Get(), Context.TargetActor.Get());
				DamageToApply *= FriendlyScalar;
			}
		}
	}

	// 伤害结算顺序固定为 Shield -> Health：护盾不足时，剩余伤害才会继续扣生命。
	// 这里直接消费当前 Shield，而不是修改 ShieldCapacity；后者是最大容量，不能作为受击资源使用。
	const float PreviousShield = FMath::Max(GetShield(), 0.f);
	const float ShieldDamage = FMath::Min(PreviousShield, DamageToApply);
	SetShield(PreviousShield - ShieldDamage);

	const float RemainingHealthDamage = FMath::Max(DamageToApply - ShieldDamage, 0.f);
	const float PreviousHealth = GetHealth();
	const float NewHealth = FMath::Clamp(PreviousHealth - RemainingHealthDamage, 0.f, GetMaxHealth());
	float FinalHealth = NewHealth;

	// 若目标拥有免疫致死标签，则血量最低保持 1，避免被直接打死
	if (FinalHealth <= 0.f && Context.TargetASC)
	{
		const FGameplayTag ImmuneDeathTag = FGameplayTag::RequestGameplayTag(FName("Status.ImmuneDeath"), false);
		if (ImmuneDeathTag.IsValid() && Context.TargetASC->HasMatchingGameplayTag(ImmuneDeathTag))
		{
			FinalHealth = 1.f;
		}
	}

	SetHealth(FinalHealth);
	const float ActualHealthDamage = FMath::Max(PreviousHealth - FinalHealth, 0.f);
	SendDamageNumberFeedback(Context, ActualHealthDamage);
	SendDamageTakenCue(Context, ActualHealthDamage);

	// 只在 Health 从正数跨到 0 的这一刻进入死亡管线。
	// Shotgun 同帧可能带来多个 pellet 命中；bWasAlive 可避免重复击杀广播和重复死亡。
	if (FinalHealth <= 0.f && Context.TargetCharacter)
	{
		if (ICombatInterface* CombatTarget = Cast<ICombatInterface>(Context.TargetCharacter))
		{
			// AShootCharacterBase::Die 已有幂等保护，并负责停移动、关碰撞和取消能力。
			CombatTarget->Die(FVector::ZeroVector);
		}
		// 通知 GameMode（便于积分/充能）
		if (UWorld* World = Context.TargetCharacter->GetWorld())
		{
			if (AGameModeBase* GameModeBase = World->GetAuthGameMode())
			{
				if (AShootGameModeBase* ShootGM = Cast<AShootGameModeBase>(GameModeBase))
				{
					ShootGM->NotifyCharacterKilled(Context.SourceActor.Get(), Context.TargetCharacter);
				}
			}
		}
	}
}

void UShootAttributeSet::SendDamageTakenCue(const FShootEffectContext& Context, float ActualHealthDamage) const
{
	if (ActualHealthDamage <= KINDA_SMALL_NUMBER || !Context.TargetASC || !Context.TargetActor
		|| !Context.TargetActor->HasAuthority())
	{
		return;
	}

	FGameplayCueParameters CueParameters = Context.bHasHitResult
		? UGameplayCueFunctionLibrary::MakeGameplayCueParametersFromHitResult(Context.HitResult)
		: FGameplayCueParameters();
	CueParameters.Instigator = Context.SourceActor.Get();
	CueParameters.EffectCauser = Context.SourceActor.Get();
	CueParameters.RawMagnitude = ActualHealthDamage;
	CueParameters.NormalizedMagnitude = GetMaxHealth() > KINDA_SMALL_NUMBER
		? FMath::Clamp(ActualHealthDamage / GetMaxHealth(), 0.f, 1.f)
		: 0.f;
	CueParameters.AggregatedSourceTags = Context.SourceTags;
	CueParameters.AggregatedTargetTags = Context.TargetTags;
	if (!Context.bHasHitResult)
	{
		CueParameters.Location = Context.TargetActor->GetActorLocation();
	}

	// 在服务器最终扣血后才广播，友伤归零、免死与过量伤害都使用实际 Health 损失。
	// GCN 不生成 NumberPop；伤害数字仍只走 SourcePlayerController 的玩家私有 RPC，避免双份显示。
	Context.TargetASC->ExecuteGameplayCue(FShootGameplayTags::Get().GameplayCue_Character_DamageTaken, CueParameters);
}

void UShootAttributeSet::SendDamageNumberFeedback(const FShootEffectContext& Context, float ActualHealthDamage) const
{
	if (ActualHealthDamage <= KINDA_SMALL_NUMBER || !Context.TargetActor || !Context.TargetActor->HasAuthority())
	{
		return;
	}

	AShootPlayerController* SourcePlayerController = Cast<AShootPlayerController>(Context.SourceController);
	if (!SourcePlayerController)
	{
		// AI、环境伤害和无玩家来源的 GE 不应把数字发送到任意本地玩家。
		return;
	}

	FShootNumberPopRequest Request;
	Request.WorldLocation = Context.bHasHitResult
		? FVector(Context.HitResult.ImpactPoint)
		: Context.TargetActor->GetActorLocation() + FVector(0.f, 0.f, 50.f);
	Request.SourceTags = Context.SourceTags;
	Request.TargetTags = Context.TargetTags;
	Request.NumberToDisplay = FMath::Max(1, FMath::RoundToInt(ActualHealthDamage));

	if (Context.bHasHitResult)
	{
		if (const UPhysicalMaterialWithTags* PhysicalMaterial = Cast<UPhysicalMaterialWithTags>(Context.HitResult.PhysMaterial.Get()))
		{
			Request.bIsCriticalDamage = PhysicalMaterial->Tags.HasTagExact(FShootGameplayTags::Get().Gameplay_Zone_WeakSpot);
		}
	}

	// 服务器只向伤害来源 Controller 的拥有客户端发送表现；分屏与 Listen Server 不会互相串线。
	SourcePlayerController->ClientAddDamageNumber(Request);
}

bool UShootAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	return Super::PreGameplayEffectExecute(Data);
}

void UShootAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Health, OldHealth);
}

void UShootAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Strength, OldStrength);
}

void UShootAttributeSet::OnRep_Vitality(const FGameplayAttributeData& OldVitality) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Vitality, OldVitality);
}

void UShootAttributeSet::OnRep_Agility(const FGameplayAttributeData& OldAgility) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Agility, OldAgility);
}

void UShootAttributeSet::OnRep_Perception(const FGameplayAttributeData& OldPerception) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Perception, OldPerception);
}

void UShootAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Armor, OldArmor);
}

void UShootAttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, ArmorPenetration, OldArmorPenetration);
}

void UShootAttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, CriticalHitChance, OldCriticalHitChance);
}

void UShootAttributeSet::OnRep_CriticalHitDamage(const FGameplayAttributeData& OldCriticalHitDamage) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, CriticalHitDamage, OldCriticalHitDamage);
}

void UShootAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MaxHealth, OldMaxHealth);
}

void UShootAttributeSet::OnRep_ShieldCapacity(const FGameplayAttributeData& OldShieldCapacity) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, ShieldCapacity, OldShieldCapacity);
}

void UShootAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, Shield, OldShield);
}

void UShootAttributeSet::OnRep_DamageReduction(const FGameplayAttributeData& OldDamageReduction) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, DamageReduction, OldDamageReduction);
}

void UShootAttributeSet::OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, MoveSpeedMultiplier, OldValue);
}

void UShootAttributeSet::OnRep_FireRateMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, FireRateMultiplier, OldValue);
}

void UShootAttributeSet::OnRep_ReloadSpeedMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, ReloadSpeedMultiplier, OldValue);
}

void UShootAttributeSet::OnRep_HealingDoneMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, HealingDoneMultiplier, OldValue);
}

void UShootAttributeSet::OnRep_HealingReceivedMultiplier(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, HealingReceivedMultiplier, OldValue);
}

void UShootAttributeSet::OnRep_ShieldCapacityBonus(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, ShieldCapacityBonus, OldValue);
}

void UShootAttributeSet::OnRep_DamageReductionBonus(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, DamageReductionBonus, OldValue);
}

void UShootAttributeSet::OnRep_UltimateCharge(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, UltimateCharge, OldValue);
}

void UShootAttributeSet::OnRep_UltimateChargeMax(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, UltimateChargeMax, OldValue);
}

void UShootAttributeSet::OnRep_CriticalHitResistance(const FGameplayAttributeData& OldCriticalHitResistance) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass, CriticalHitResistance, OldCriticalHitResistance);
}
