# Lyra 关键代码摘录与蓝图位置

## 用途

- 给执行 AI 一个“全貌 + 抄代码入口”。
- 先看本文，知道 Lyra 的伤害链在哪个类、哪个蓝图、哪一段代码。
- 实施时可以直接参考下面的代码块，再打开对应 Lyra 文件确认上下文。
- 项目实现不需要逐字复制，但调用链和职责边界按 Lyra 对齐。

## 只看这几个文件就够

1. Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.cpp
2. Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility.cpp
3. Source/LyraGame/AbilitySystem/Executions/LyraDamageExecution.cpp
4. Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp
5. Source/LyraGame/AbilitySystem/LyraGameplayEffectContext.cpp
6. Source/LyraGame/AbilitySystem/Attributes/LyraCombatSet.h
7. Source/LyraGame/AbilitySystem/Attributes/LyraHealthSet.h

蓝图只要看这几个：

- /Game/Weapons/GA_Weapon_Fire
- /ShooterCore/Weapons/Pistol/GA_Weapon_Fire_Pistol
- /ShooterCore/Weapons/Rifle/GA_Weapon_Fire_Rifle_Auto
- /ShooterCore/Weapons/Shotgun/GA_Weapon_Fire_Shotgun
- /Game/Weapons/Pistol/GE_Damage_Pistol
- /ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto
- /ShooterCore/Weapons/Shotgun/GE_Damage_Shotgun
- /ShooterCore/Weapons/Pistol|Rifle|Shotgun/B_WeaponInstance_*
- /Game/Characters/Heroes/PhysMat_Player
- /Game/Characters/Heroes/PhysMat_Player_WeakSpot

## Lyra 射击能力 C++ 核心

### ULyraGameplayAbility_RangedWeapon::OnTargetDataReadyCallback

文件：Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.cpp

```cpp
void ULyraGameplayAbility_RangedWeapon::OnTargetDataReadyCallback(
    const FGameplayAbilityTargetDataHandle& InData,
    FGameplayTag ApplicationTag)
{
    UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
    check(MyAbilityComponent);

    if (const FGameplayAbilitySpec* AbilitySpec = MyAbilityComponent->FindAbilitySpecFromHandle(CurrentSpecHandle))
    {
        FScopedPredictionWindow ScopedPrediction(MyAbilityComponent);

        FGameplayAbilityTargetDataHandle LocalTargetDataHandle(
            MoveTemp(const_cast<FGameplayAbilityTargetDataHandle&>(InData)));

        const bool bShouldNotifyServer = CurrentActorInfo->IsLocallyControlled() && !CurrentActorInfo->IsNetAuthority();
        if (bShouldNotifyServer)
        {
            MyAbilityComponent->CallServerSetReplicatedTargetData(
                CurrentSpecHandle,
                CurrentActivationInfo.GetActivationPredictionKey(),
                LocalTargetDataHandle,
                ApplicationTag,
                MyAbilityComponent->ScopedPredictionKey);
        }

        // 服务器确认命中后，Commit 成功才调蓝图事件
        if (bIsTargetDataValid && CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
        {
            ULyraRangedWeaponInstance* WeaponData = GetWeaponInstance();
            check(WeaponData);
            WeaponData->AddSpread();
            OnRangedWeaponTargetDataReady(LocalTargetDataHandle);
        }
        else
        {
            K2_EndAbility();
        }
    }

    MyAbilityComponent->ConsumeClientReplicatedTargetData(
        CurrentSpecHandle,
        CurrentActivationInfo.GetActivationPredictionKey());
}
```

### ULyraGameplayAbility_RangedWeapon::StartRangedWeaponTargeting

```cpp
void ULyraGameplayAbility_RangedWeapon::StartRangedWeaponTargeting()
{
    check(CurrentActorInfo);
    AActor* AvatarActor = CurrentActorInfo->AvatarActor.Get();
    check(AvatarActor);

    UAbilitySystemComponent* MyAbilityComponent = CurrentActorInfo->AbilitySystemComponent.Get();
    check(MyAbilityComponent);

    AController* Controller = GetControllerFromActorInfo();
    check(Controller);
    ULyraWeaponStateComponent* WeaponStateComponent = Controller->FindComponentByClass<ULyraWeaponStateComponent>();

    TArray<FHitResult> FoundHits;
    PerformLocalTargeting(/*out*/ FoundHits);

    FGameplayAbilityTargetDataHandle TargetData;
    TargetData.UniqueId = WeaponStateComponent ? WeaponStateComponent->GetUnconfirmedServerSideHitMarkerCount() : 0;

    if (FoundHits.Num() > 0)
    {
        const int32 CartridgeID = FMath::Rand();
        for (const FHitResult& FoundHit : FoundHits)
        {
            FLyraGameplayAbilityTargetData_SingleTargetHit* NewTargetData =
                new FLyraGameplayAbilityTargetData_SingleTargetHit();
            NewTargetData->HitResult = FoundHit;
            NewTargetData->CartridgeID = CartridgeID;
            TargetData.Add(NewTargetData);
        }
    }

    if (WeaponStateComponent != nullptr)
    {
        WeaponStateComponent->AddUnconfirmedServerSideHitMarkers(TargetData, FoundHits);
    }

    OnTargetDataReadyCallback(TargetData, FGameplayTag());
}
```

项目可以不抄 WeaponStateComponent，但 TargetData 生成和 OnTargetDataReadyCallback 的结构要保留。

## Lyra 效果上下文核心

### ULyraGameplayAbility::MakeEffectContext

文件：Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility.cpp

```cpp
FGameplayEffectContextHandle ULyraGameplayAbility::MakeEffectContext(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo) const
{
    FGameplayEffectContextHandle ContextHandle = Super::MakeEffectContext(Handle, ActorInfo);

    FLyraGameplayEffectContext* EffectContext = FLyraGameplayEffectContext::ExtractEffectContext(ContextHandle);
    check(EffectContext);
    check(ActorInfo);

    AActor* EffectCauser = nullptr;
    const ILyraAbilitySourceInterface* AbilitySource = nullptr;
    float SourceLevel = 0.0f;
    GetAbilitySource(Handle, ActorInfo, /*out*/ SourceLevel, /*out*/ AbilitySource, /*out*/ EffectCauser);

    UObject* SourceObject = GetSourceObject(Handle, ActorInfo);
    AActor* Instigator = ActorInfo ? ActorInfo->OwnerActor.Get() : nullptr;

    EffectContext->SetAbilitySource(AbilitySource, SourceLevel);
    EffectContext->AddInstigator(Instigator, EffectCauser);
    EffectContext->AddSourceObject(SourceObject);

    return ContextHandle;
}
```

### ULyraGameplayAbility::GetAbilitySource

```cpp
void ULyraGameplayAbility::GetAbilitySource(
    FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    float& OutSourceLevel,
    const ILyraAbilitySourceInterface*& OutAbilitySource,
    AActor*& OutEffectCauser) const
{
    OutSourceLevel = 0.0f;
    OutAbilitySource = nullptr;
    OutEffectCauser = nullptr;

    OutEffectCauser = ActorInfo->AvatarActor.Get();

    UObject* SourceObject = GetSourceObject(Handle, ActorInfo);
    OutAbilitySource = Cast<ILyraAbilitySourceInterface>(SourceObject);
}
```

### ULyraGameplayAbility::ApplyAbilityTagsToGameplayEffectSpec

```cpp
void ULyraGameplayAbility::ApplyAbilityTagsToGameplayEffectSpec(
    FGameplayEffectSpec& Spec,
    FGameplayAbilitySpec* AbilitySpec) const
{
    Super::ApplyAbilityTagsToGameplayEffectSpec(Spec, AbilitySpec);

    if (const FHitResult* HitResult = Spec.GetContext().GetHitResult())
    {
        if (const UPhysicalMaterialWithTags* PhysMatWithTags =
            Cast<const UPhysicalMaterialWithTags>(HitResult->PhysMaterial.Get()))
        {
            Spec.CapturedTargetTags.GetSpecTags().AppendTags(PhysMatWithTags->Tags);
        }
    }
}
```

### FLyraGameplayEffectContext

文件：Source/LyraGame/AbilitySystem/LyraGameplayEffectContext.cpp

```cpp
void FLyraGameplayEffectContext::SetAbilitySource(
    const ILyraAbilitySourceInterface* InObject,
    float InSourceLevel)
{
    AbilitySourceObject = MakeWeakObjectPtr(Cast<const UObject>(InObject));
}

const ILyraAbilitySourceInterface* FLyraGameplayEffectContext::GetAbilitySource() const
{
    return Cast<ILyraAbilitySourceInterface>(AbilitySourceObject.Get());
}

const UPhysicalMaterial* FLyraGameplayEffectContext::GetPhysicalMaterial() const
{
    if (const FHitResult* HitResultPtr = GetHitResult())
    {
        return HitResultPtr->PhysMaterial.Get();
    }
    return nullptr;
}
```

## Lyra 伤害 Execution 核心

### ULyraDamageExecution::Execute_Implementation

文件：Source/LyraGame/AbilitySystem/Executions/LyraDamageExecution.cpp

```cpp
void ULyraDamageExecution::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    FLyraGameplayEffectContext* TypedContext = FLyraGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
    check(TypedContext);

    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluateParameters;
    EvaluateParameters.SourceTags = SourceTags;
    EvaluateParameters.TargetTags = TargetTags;

    float BaseDamage = 0.0f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
        DamageStatics().BaseDamageDef,
        EvaluateParameters,
        BaseDamage);

    const AActor* EffectCauser = TypedContext->GetEffectCauser();
    const FHitResult* HitActorResult = TypedContext->GetHitResult();

    AActor* HitActor = nullptr;
    FVector ImpactLocation = FVector::ZeroVector;
    FVector ImpactNormal = FVector::ZeroVector;
    FVector StartTrace = FVector::ZeroVector;
    FVector EndTrace = FVector::ZeroVector;

    if (HitActorResult)
    {
        const FHitResult& CurHitResult = *HitActorResult;
        HitActor = CurHitResult.HitObjectHandle.FetchActor();
        if (HitActor)
        {
            ImpactLocation = CurHitResult.ImpactPoint;
            ImpactNormal = CurHitResult.ImpactNormal;
            StartTrace = CurHitResult.TraceStart;
            EndTrace = CurHitResult.TraceEnd;
        }
    }

    UAbilitySystemComponent* TargetAbilitySystemComponent =
        ExecutionParams.GetTargetAbilitySystemComponent();
    if (!HitActor)
    {
        HitActor = TargetAbilitySystemComponent
            ? TargetAbilitySystemComponent->GetAvatarActor_Direct()
            : nullptr;
        if (HitActor)
        {
            ImpactLocation = HitActor->GetActorLocation();
        }
    }

    float DamageInteractionAllowedMultiplier = 0.0f;
    if (HitActor)
    {
        ULyraTeamSubsystem* TeamSubsystem = HitActor->GetWorld()->GetSubsystem<ULyraTeamSubsystem>();
        if (ensure(TeamSubsystem))
        {
            DamageInteractionAllowedMultiplier =
                TeamSubsystem->CanCauseDamage(EffectCauser, HitActor) ? 1.0 : 0.0;
        }
    }

    double Distance = WORLD_MAX;
    if (TypedContext->HasOrigin())
    {
        Distance = FVector::Dist(TypedContext->GetOrigin(), ImpactLocation);
    }
    else if (EffectCauser)
    {
        Distance = FVector::Dist(EffectCauser->GetActorLocation(), ImpactLocation);
    }
    else
    {
        UE_LOG(LogLyraAbilitySystem, Error,
            TEXT("Damage Calculation cannot deduce a source location for damage coming from %s; "
                 "Falling back to WORLD_MAX dist!"),
            *GetPathNameSafe(Spec.Def));
    }

    float PhysicalMaterialAttenuation = 1.0f;
    float DistanceAttenuation = 1.0f;
    if (const ILyraAbilitySourceInterface* AbilitySource = TypedContext->GetAbilitySource())
    {
        if (const UPhysicalMaterial* PhysMat = TypedContext->GetPhysicalMaterial())
        {
            PhysicalMaterialAttenuation =
                AbilitySource->GetPhysicalMaterialAttenuation(PhysMat, SourceTags, TargetTags);
        }
        DistanceAttenuation = AbilitySource->GetDistanceAttenuation(Distance, SourceTags, TargetTags);
    }
    DistanceAttenuation = FMath::Max(DistanceAttenuation, 0.0f);

    const float DamageDone = FMath::Max(
        BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation *
        DamageInteractionAllowedMultiplier,
        0.0f);

    if (DamageDone > 0.0f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            ULyraHealthSet::GetDamageAttribute(),
            EGameplayModOp::Additive,
            DamageDone));
    }
#endif
}
```

## Lyra WeaponInstance 衰减核心

### ULyraRangedWeaponInstance::GetDistanceAttenuation

文件：Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp

```cpp
float ULyraRangedWeaponInstance::GetDistanceAttenuation(
    float Distance,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags) const
{
    const FRichCurve* Curve = DistanceDamageFalloff.GetRichCurveConst();
    return Curve->HasAnyData() ? Curve->Eval(Distance) : 1.0f;
}
```

### ULyraRangedWeaponInstance::GetPhysicalMaterialAttenuation

```cpp
float ULyraRangedWeaponInstance::GetPhysicalMaterialAttenuation(
    const UPhysicalMaterial* PhysicalMaterial,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags) const
{
    float CombinedMultiplier = 1.0f;
    if (const UPhysicalMaterialWithTags* PhysMatWithTags =
        Cast<const UPhysicalMaterialWithTags>(PhysicalMaterial))
    {
        for (const FGameplayTag MaterialTag : PhysMatWithTags->Tags)
        {
            if (const float* pTagMultiplier = MaterialDamageMultiplier.Find(MaterialTag))
            {
                CombinedMultiplier *= *pTagMultiplier;
            }
        }
    }
    return CombinedMultiplier;
}
```

## Lyra 属性与 GE 资产事实

### ULyraCombatSet

文件：Source/LyraGame/AbilitySystem/Attributes/LyraCombatSet.h

- BaseDamage 属性是 Source 的 CombatSet 属性。
- 默认值 0。
- LyraDamageExecution 捕获 Source 的 BaseDamage。
- 本项目不复制该来源链路；项目方案把 BaseDamage 放在自己的 GE 类中。

### ULyraHealthSet

文件：Source/LyraGame/AbilitySystem/Attributes/LyraHealthSet.h

- Damage 是 Meta 属性。
- Health 被隐藏，不直接允许 Modifier 修改。
- DamageExecution 输出到 Damage，HealthSet 在 PostGameplayEffectExecute 把 Damage 转成 Health 扣减。
- 项目已有等价物：UShootAttributeSet::IncomingDamage 和 HandleIncomingDamage。

### GE_Damage_* 资产事实

- /Game/Weapons/Pistol/GE_Damage_Pistol
- /ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto
- /ShooterCore/Weapons/Shotgun/GE_Damage_Shotgun

MCP 已确认：

- DurationPolicy = Instant。
- Modifiers 为空。
- Executions 只有 1 个 ULyraDamageExecution。
- 只有 GameplayEffectTag 区分 DamageType.Pistol / Rifle / Shotgun。
- GameplayEffectParent_Damage_Basic 也是同样结构。

### GA_Weapon_Fire 蓝图事实

- /Game/Weapons/GA_Weapon_Fire 是父蓝图，父类 ULyraGameplayAbility_RangedWeapon。
- 有变量 GE_Damage，类型 TSubclassOf<GameplayEffect>。
- OnRangedWeaponTargetDataReady 事件图里：
  - 先执行 Fire Cue。
  - 遍历 TargetData 执行 Impact Cue。
  - Authority 时用 ApplyGameplayEffectToTarget，GameplayEffectClass 输入来自 Get GE_Damage。
- 子蓝图：
  - GA_Weapon_Fire_Pistol 的 GE_Damage = GE_Damage_Pistol。
  - GA_Weapon_Fire_Rifle_Auto 的 GE_Damage = GE_Damage_RifleAuto。
  - GA_Weapon_Fire_Shotgun 的 GE_Damage = GE_Damage_Shotgun。

### B_WeaponInstance_* 弱点倍率

MCP 已确认：

- Pistol：Gameplay.Zone.WeakSpot = 2.0
- Rifle：Gameplay.Zone.WeakSpot = 1.5
- Shotgun：Gameplay.Zone.WeakSpot = 1.75

### PhysMat

- PhysMat_Player：无 Tag。
- PhysMat_Player_WeakSpot：Gameplay.Zone.WeakSpot。

## 项目实现时怎么用这份摘录

- 项目不需要复制 FLyraGameplayEffectContext，但 Execution 要能拿到 SourceObject 和 HitResult。
- 项目已有 UShootRangedWeaponInstance 实现 ILyraAbilitySourceInterface，保留该接口即可。
- 项目已有 UShootAttributeSet::IncomingDamage 后处理，Execution 输出到 IncomingDamage 即可。
- 项目 Fire GA 的 ApplyDamageToTarget 目前手动计算伤害；应改成只做 EffectContext 装配 + MakeOutgoingSpec + Apply，把计算交给 Execution。
- 每枪 GE 的选择方式抄 Lyra：Fire GA 持有 DamageGameplayEffectClass 或 Blueprint 变量 GE_Damage，每把枪指向自己的 GE。

## 不建议抄的部分

- ULyraWeaponStateComponent。
- FLyraGameplayEffectContext 的 CartridgeID 与严格命中校验。
- ULyraCombatSet::BaseDamage 的来源链路，因为项目已改用 GE 内 BaseDamage。
- ULyraHealthSet 的完整复制，项目已有 ShootAttributeSet。
