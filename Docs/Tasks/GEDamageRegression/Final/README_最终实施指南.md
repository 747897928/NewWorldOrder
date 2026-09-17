# NewWorldOrder Lyra 武器射击 GAS 实施指南

## 阅读说明

- 本文是最终实施文档，不是过程笔记。
- 目标读者：执行代码的 AI 或开发者。
- 拿到本文，应该能直接开始改代码；遇到本文没展开的细节，按文末“去 Lyra 哪里看”打开对应文件。
- 本文假设项目已确认“基本对齐 Lyra，能抄就抄，不适配才改”。

## 已确认的边界

- 不修改 Unreal Engine 源码。
- 不修改 Lyra 与第三方插件源码。
- 项目 PlayerState 持有 ASC。
- Controller 持有 RuntimeOnly QuickBar。
- Pawn 只承担装备表现桥。
- 当前项目已明确不引入 WeaponStateComponent，但本指南会把该决策作为待确认项，而不是默认红线。

## 架构总览

### Lyra 官方链路

```text
Equipment -> AbilitySet -> GA_Weapon_Fire
GA_Weapon_Fire -> ULyraGameplayAbility_RangedWeapon
-> TargetData
-> OnRangedWeaponTargetDataReady（蓝图事件）
-> Fire GameplayCue
-> Impact GameplayCue
-> 每枪 Damage GE（GE_Damage_*）
-> ULyraDamageExecution
-> WeaponInstance（ILyraAbilitySourceInterface）
-> DistanceDamageFalloff / MaterialDamageMultiplier
-> ULyraHealthSet::Damage
-> Health
```

### 项目当前链路

```text
Equipment -> AbilitySet -> ShootGA_Weapon_Fire_*
-> UShootGameplayAbility_Weapon_Fire
-> TargetData
-> OnRangedWeaponTargetDataReady_Implementation
-> Fire GameplayCue / Impact GameplayCue
-> ApplyDamageToTarget（GA 内手算伤害）
-> UShootEffect_DamageSetByCaller
-> UShootAttributeSet::IncomingDamage
-> HandleIncomingDamage
-> Health
```

### 项目已有可复用资产

- UShootRangedWeaponInstance 已实现 ILyraAbilitySourceInterface。
- UShootRangedWeaponInstance 已有 GetDistanceAttenuation 和 GetPhysicalMaterialAttenuation。
- UShootRangedWeaponInstance 已有 GetProjectileDamageEffect，证明“每把武器配置 GE”的先例。
- UShootAttributeSet 已有 IncomingDamage 和 HandleIncomingDamage。
- UShootReticleWidgetBase / ShootCircumferenceMarkerWidget / ShootHitMarkerConfirmationWidget 已从 Lyra 迁移。
- UShootHUDReticleComponent + ShootReticleHostWidget 已接入 LocalPlayer HUD。
- PhysMat_Player_WeakSpot 已有 Gameplay.Zone.WeakSpot。

## Phase 0：装备技能授予与死字段清理

### 结论

- 武器能力不是通过 GrantedAttributes 授予的。
- 武器能力通过 EquipmentManager -> AbilitySet -> GrantedGameplayAbilities 授予。
- GrantedAttributes 为空是正常现象，不代表没给技能。

### 当前授予链

```text
QuickBar/Equipment
-> ShootEquipmentManagerComponent::EquipItem
-> FShootEquipmentList::AddEntry
-> EquipmentDefinition->AbilitySetsToGrant
-> UShootAbilitySet::GiveToAbilitySystem
-> GrantedGameplayAbilities
-> GiveAbility(Spec)
-> Spec.SourceObject = WeaponInstance
```

代码位置：

- Source/NewWorldOrder/Private/Equipment/ShootEquipmentManagerComponent.cpp
- Source/NewWorldOrder/Private/AbilitySystem/ShootAbilitySet.cpp
- Source/NewWorldOrder/Public/AbilitySystem/ShootAbilitySet.h

### 当前 AbilitySet 资产事实

- /Game/Weapons/Rifle/AS_Weapon_Rifle：
  - ShootGA_Weapon_Fire_Rifle，InputTag.LMB
  - GA_Weapon_Reload_Rifle_C，InputTag.R
  - GrantedGameplayEffects 空
  - GrantedAttributes 空
- /Game/Weapons/Pistol/AS_Weapon_Pistol：
  - ShootGA_Weapon_Fire_Pistol，InputTag.LMB
  - GA_Weapon_Reload_Pistol_C，InputTag.R
- /Game/Weapons/Shotgun/AS_Weapon_Shotgun：
  - ShootGA_Weapon_Fire_Shotgun，InputTag.LMB
  - GA_Weapon_Reload_Shotgun_C，InputTag.R

### 与 Lyra 的差异

Lyra AbilitySet_ShooterPistol / AbilitySet_ShooterRifle / AbilitySet_ShooterShotgun 都有：

- Fire GA
- Reload GA
- GA_Weapon_AutoReload

项目三把枪都缺少 GA_Weapon_AutoReload。

需要：

- 从 Lyra 抄 /Game/Weapons/GA_Weapon_AutoReload 的逻辑。
- 在项目创建 AutoReload GA（C++ 或蓝图）。
- 把 AutoReload GA 加进三个 AS_Weapon_* 的 GrantedGameplayAbilities。
- 确认 AutoReload 的输入触发方式：Lyra 是通过弹匣空时自动触发，不是手动按键。

### Socket 死字段

项目 ShootInventoryFragment_RangedWeaponConfig 有：

- AttachSocketHand
- AttachSocketHolster

当前 C++ 没有读取这两个字段。

真实挂点位置在：

- BP_Equipment_Rifle / BP_Equipment_Pistol / BP_Equipment_Shotgun 的 ActorsToSpawn[0].AttachSocket = weapon_r

建议：

- 从 ShootInventoryFragment_RangedWeaponConfig 删除 AttachSocketHand / AttachSocketHolster。
- 挂点继续以 EquipmentDefinition ActorsToSpawn 为准。
- 删除前用编辑器引用扫描确认没有蓝图读取。

### Phase 0 验收

- 拾取武器后，Fire 和 Reload GA 能激活。
- 弹匣打空后 AutoReload 能自动触发。
- 卸下武器后，这些 GA 被移除。
- 武器挂点仍为 weapon_r，不受 Fragment socket 字段删除影响。
## Phase 1：武器直射伤害 GE

### Lyra 位置

- /Game/Weapons/GA_Weapon_Fire 蓝图，变量 GE_Damage。
- /Game/Weapons/Pistol/GE_Damage_Pistol
- /ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto
- /ShooterCore/Weapons/Shotgun/GE_Damage_Shotgun
- Source/LyraGame/AbilitySystem/Executions/LyraDamageExecution.cpp
- Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp

### 项目现状

- 三把枪没有每枪 GE。
- 三把枪 Fire GA 默认 DamageGameplayEffectClass = UShootEffect_DamageSetByCaller。
- Sniper 的 DamageGameplayEffectClass 为 null。
- ApplyDamageToTarget 在 Rifle / Shotgun / Sniper 各复制一份手算伤害。

### 改动目标

- 每把枪一个 GE 资产。
- GE 持有 BaseDamage。
- Execution 从 GE 读取 BaseDamage，再调用 WeaponInstance 的衰减。
- Fire GA 不再手算伤害。

### 新增 C++ 文件

1. Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_DamageBase.h
2. Source/NewWorldOrder/Private/AbilitySystem/Effects/ShootEffect_DamageBase.cpp
3. Source/NewWorldOrder/Public/AbilitySystem/Executions/ShootDamageExecution.h
4. Source/NewWorldOrder/Private/AbilitySystem/Executions/ShootDamageExecution.cpp

### ShootEffect_DamageBase

```cpp
UCLASS(Blueprintable, BlueprintType)
class NEWWORLDORDER_API UShootEffect_DamageBase : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UShootEffect_DamageBase();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
    float BaseDamage = 10.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
    FGameplayTag DamageTypeTag;
};
```

构造函数：

```cpp
UShootEffect_DamageBase::UShootEffect_DamageBase()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayEffectExecutionDefinition Execution;
    Execution.CalculationClass = UShootDamageExecution::StaticClass();
    Executions.Add(Execution);
}
```

### ShootDamageExecution

```cpp
UCLASS()
class NEWWORLDORDER_API UShootDamageExecution : public UGameplayEffectExecutionCalculation
{
    GENERATED_BODY()

public:
    UShootDamageExecution();

protected:
    virtual void Execute_Implementation(
        const FGameplayEffectCustomExecutionParameters& ExecutionParams,
        FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
```

实现：

```cpp
void UShootDamageExecution::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    const UShootEffect_DamageBase* DamageGE = Cast<const UShootEffect_DamageBase>(Spec.Def);
    if (!DamageGE)
    {
        return;
    }

    float Damage = DamageGE->BaseDamage;

    const FGameplayEffectContextHandle& Context = Spec.GetContext();
    if (const UObject* SourceObject = Context.GetSourceObject())
    {
        if (const UShootRangedWeaponInstance* Weapon =
            Cast<UShootRangedWeaponInstance>(SourceObject))
        {
            if (const FHitResult* Hit = Context.GetHitResult())
            {
                const float Distance = FVector::Dist(Hit->TraceStart, Hit->ImpactPoint);
                Damage *= Weapon->GetDistanceAttenuation(Distance);

                if (Hit->PhysMaterial.IsValid())
                {
                    Damage *= Weapon->GetPhysicalMaterialAttenuation(Hit->PhysMaterial.Get());
                }
            }
        }
    }

    if (Damage > 0.f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            UShootAttributeSet::GetIncomingDamageAttribute(),
            EGameplayModOp::Additive,
            Damage));
    }
}
```

### 创建每枪 GE 资产

- /Game/Weapons/Pistol/GE_Damage_Pistol
- /Game/Weapons/Rifle/GE_Damage_Rifle
- /Game/Weapons/Shotgun/GE_Damage_Shotgun

父类：UShootEffect_DamageBase。

配置：

- BaseDamage：先沿用 10，后续按策划数值。
- DamageTypeTag：GameplayEffect.DamageType.Pistol / Rifle / Shotgun。
- DurationPolicy：继承父类 Instant。

### WeaponInstance 增加 DamageGameplayEffect

UShootRangedWeaponInstance.h：

```cpp
UFUNCTION(BlueprintPure, Category="Weapon|Damage")
TSubclassOf<UGameplayEffect> GetDamageGameplayEffect() const;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
TSubclassOf<UGameplayEffect> DamageGameplayEffect;
```

UShootRangedWeaponInstance.cpp：

```cpp
TSubclassOf<UGameplayEffect> UShootRangedWeaponInstance::GetDamageGameplayEffect() const
{
    return DamageGameplayEffect;
}
```

在三个 WeaponInstance 蓝图里把 DamageGameplayEffect 指向对应 GE。

### Fire GA 收敛 ApplyDamageToTarget

把三个子类里的 ApplyDamageToTarget 删掉，在基类实现一份：

```cpp
void UShootGameplayAbility_Weapon_Fire::ApplyDamageToTarget(const FHitResult& Hit)
{
    UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
    if (!Weapon)
    {
        return;
    }

    TSubclassOf<UGameplayEffect> DamageGE = Weapon->GetDamageGameplayEffect();
    if (!DamageGE)
    {
        return;
    }

    AActor* TargetActor = Hit.GetActor();
    if (!TargetActor)
    {
        return;
    }

    UAbilitySystemComponent* TargetASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
    if (!TargetASC || !SourceASC)
    {
        return;
    }

    FGameplayEffectContextHandle EffectContext = SourceASC->MakeEffectContext();
    EffectContext.AddSourceObject(Weapon);
    EffectContext.AddInstigator(GetAvatarActorFromActorInfo(), GetAvatarActorFromActorInfo());
    EffectContext.AddHitResult(Hit);

    FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
        DamageGE,
        GetAbilityLevel(),
        EffectContext);

    if (!SpecHandle.IsValid())
    {
        return;
    }

    SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}
```

### Phase 1 验收

- 冷编译通过。
- 三把枪各自 GE 生效。
- 打身体伤害等于 GE BaseDamage 乘距离衰减。
- Sniper 不再 null。

## Phase 2：距离衰减与弱点

### Lyra 位置

- Source/LyraGame/Weapons/LyraRangedWeaponInstance.h
- Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp
- B_WeaponInstance_Pistol / Rifle / Shotgun

### 项目现状

- 数据在 ShootInventoryFragment_RangedWeaponConfig。
- 三把枪 DistanceFalloff 为空。
- MaterialDamageMultiplier 为空。
- WeakSpot 只在 Status.Marked 时加 10%，写在 GA 里。

### 改动目标

- 数据迁到 UShootRangedWeaponInstance。
- GetDistanceAttenuation 读实例字段。
- GetPhysicalMaterialAttenuation 读实例字段。
- 删除 GA 内 WeakSpot + Marked 10% 硬编码。

### 代码

UShootRangedWeaponInstance.h：

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon Config")
FRuntimeFloatCurve DistanceDamageFalloff;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon Config")
TMap<FGameplayTag, float> MaterialDamageMultiplier;
```

UShootRangedWeaponInstance.cpp：

```cpp
float UShootRangedWeaponInstance::GetDistanceAttenuation(
    float Distance,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags) const
{
    const FRichCurve* Curve = DistanceDamageFalloff.GetRichCurveConst();
    return (Curve && Curve->HasAnyData()) ? Curve->Eval(Distance) : 1.0f;
}

float UShootRangedWeaponInstance::GetPhysicalMaterialAttenuation(
    const UPhysicalMaterial* PhysMat,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags) const
{
    float Multiplier = 1.0f;
    if (const UPhysicalMaterialWithTags* PhysMatWithTags =
        Cast<UPhysicalMaterialWithTags>(PhysMat))
    {
        for (const FGameplayTag& Tag : PhysMatWithTags->Tags)
        {
            if (const float* pTagMultiplier = MaterialDamageMultiplier.Find(Tag))
            {
                Multiplier *= *pTagMultiplier;
            }
        }
    }
    return Multiplier;
}
```

### WeaponInstance 蓝图配置

- Pistol：Gameplay.Zone.WeakSpot = 2.0
- Rifle：Gameplay.Zone.WeakSpot = 1.5
- Shotgun：Gameplay.Zone.WeakSpot = 1.75
- DistanceDamageFalloff：从 Lyra B_WeaponInstance_* 复制曲线。

### Phase 2 验收

- 打头伤害高于身体。
- 远距离伤害低于近距离。
- 三把枪弱点倍率不同。

## Phase 3：准星与命中提示

### Lyra 位置

- Source/LyraGame/UI/Weapons/LyraReticleWidgetBase.cpp
- Source/LyraGame/UI/Weapons/CircumferenceMarkerWidget.h
- Source/LyraGame/UI/Weapons/HitMarkerConfirmationWidget.h
- Source/LyraGame/UI/Weapons/SHitMarkerConfirmationWidget.cpp
- Source/LyraGame/Weapons/LyraWeaponStateComponent.h

### 项目现状

- ShootReticleWidgetBase 已有 ComputeSpreadAngle / ComputeMaxScreenspaceSpreadRadius / HasFirstShotAccuracy。
- ShootCircumferenceMarkerWidget 已有。
- ShootHitMarkerConfirmationWidget 已有。
- UShootHUDReticleComponent 已注册到 LocalPlayer HUD。
- BroadcastReticleHitNotify 使用项目自己的消息链。

### 建议

- 不重写准星体系。
- 对比项目与 Lyra 的方法签名和 HitZone 规则。
- 如果引入 WeaponStateComponent，命中提示需要改成 Lyra 的 AddUnconfirmedServerSideHitMarkers + ClientConfirmTargetData。
- 如果不引入，保留项目消息链。

### Phase 3 验收

- 分屏下每个玩家有自己的准星。
- 命中后 HitMarker 显示。
- WeakSpot 命中使用不同 HitZone 样式。

## Phase 4：WeaponStateComponent 决策

### Lyra 位置

- Source/LyraGame/Weapons/LyraWeaponStateComponent.h/cpp

### 项目现状

- 不引入该组件。
- 项目使用简化 TargetData UniqueId=0。
- 项目使用 BroadcastReticleHitNotify 和 ClientReceiveReticleHitNotify。

### 需要用户决定

- 是否引入 WeaponStateComponent。
- 如果引入，需要把它加到 AShootPlayerController。
- 需要把 Fire GA 的 TargetData 流程改成使用 UniqueId 和命中确认。
- 如果不引入，继续使用项目现有简化链。

## Phase 5：后坐力决策

### Lyra 现状

- Lyra C++ 没有独立 CameraRecoil 类。
- 后坐力表现主要在 B_Weapon Fire GameplayCue、动画、相机表现中。

### 项目现状

- UShootRangedWeaponInstance::GetLocalCameraRecoil。
- UShootGameplayAbility_Weapon_Fire::ApplyLocalCameraRecoil。
- 数据在 Fragment 的 LocalCameraPitch/YawRecoilMin/Max。

### 建议

- 如果项目手感需要保留，把后坐力数据迁到 WeaponInstance，保留 C++ 实现。
- 如果要完全对齐 Lyra，需要先读 B_Weapon Fire 蓝图，确认 Lyra 后坐力实现，再决定替换。
- 当前不删除项目后坐力。

## 去 Lyra 哪里看

- 射击 TargetData 与能力：Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.cpp
- EffectContext：Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility.cpp
- 伤害 Execution：Source/LyraGame/AbilitySystem/Executions/LyraDamageExecution.cpp
- WeaponInstance 衰减：Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp
- 准星：Source/LyraGame/UI/Weapons/LyraReticleWidgetBase.cpp
- 命中提示：Source/LyraGame/UI/Weapons/SHitMarkerConfirmationWidget.cpp
- WeaponState：Source/LyraGame/Weapons/LyraWeaponStateComponent.cpp
- GE 选择：/Game/Weapons/GA_Weapon_Fire 蓝图 EventGraph
- 每枪 GE：/Game/Weapons/Pistol/GE_Damage_Pistol、/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto、/ShooterCore/Weapons/Shotgun/GE_Damage_Shotgun
- WeaponInstance 数据：/ShooterCore/Weapons/Pistol|Rifle|Shotgun/B_WeaponInstance_*
- PhysMat：/Game/Characters/Heroes/PhysMat_Player、/Game/Characters/Heroes/PhysMat_Player_WeakSpot

## 本文未覆盖但已知存在

- Lyra B_Weapon Fire 蓝图内的完整表现链。
- Lyra 投射物/手雷完整表现链。
- Lyra 相机模式与 ADS 完整链路。
- Lyra 动画层与武器动画完整链路。

这些不在本文第一阶段，但实现对应系统时应先打开上述 Lyra 文件确认。
