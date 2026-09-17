# GE 伤害系统迁移实施指南

## 文档目标

- 这份文档按“执行 AI 直接照着写代码”的标准写。
- 不需要再读 Lyra 源码也能动手；需要确认上下文时，按文档末尾的 Lyra 文件路径打开对应代码。
- 2026-08-22：第一至第七步已按项目化方案落地；本文同时作为后续清理与验收指南。

## 我读过的 Lyra 代码范围

已读核心链：

- LyraGameplayAbility_RangedWeapon.h / cpp
- LyraGameplayAbility.h / cpp（MakeEffectContext、GetAbilitySource、ApplyAbilityTagsToGameplayEffectSpec）
- LyraGameplayEffectContext.h / cpp
- LyraDamageExecution.h / cpp
- LyraRangedWeaponInstance.h / cpp
- LyraCombatSet.h / cpp
- LyraHealthSet.h
- LyraAbilitySourceInterface.h
- PhysicalMaterialWithTags.h / cpp
- LyraGameplayAbility_FromEquipment.h / cpp

未读全：

- Lyra 蓝图内部每一个连线节点。
- 所有 GameplayEffect 资产内部细节。

已通过 Lyra 11000 MCP 确认的关键蓝图事实：

- /Game/Weapons/GA_Weapon_Fire 有变量 GE_Damage。
- GA_Weapon_Fire_Pistol / Rifle_Auto / Shotgun 分别指向 GE_Damage_Pistol / GE_Damage_RifleAuto / GE_Damage_Shotgun。
- 三个 GE_Damage 都是 Instant，顶层 Modifiers 为空，只有 ULyraDamageExecution。
- 每个 Execution 内有一个 Scoped Modifier，把捕获的 CombatSet.BaseDamage 覆盖为 Pistol 18、Rifle 12、Shotgun 12；不能因为顶层 Modifiers 为空就误判 Lyra 没有每枪基础值。
- B_WeaponInstance_Pistol 的 WeakSpot 倍率 2.0。
- B_WeaponInstance_Rifle 的 WeakSpot 倍率 1.5。
- B_WeaponInstance_Shotgun 的 WeakSpot 倍率 1.75。
- PhysMat_Player_WeakSpot 的 Tag 是 Gameplay.Zone.WeakSpot。

## 现有代码复用结论

项目已经有大量可复用代码，不需要另起炉灶。

可复用：

- UShootRangedWeaponInstance 已实现 ILyraAbilitySourceInterface。
- UShootRangedWeaponInstance 已有 GetDistanceAttenuation 和 GetPhysicalMaterialAttenuation。
- UShootRangedWeaponInstance 已有 GetProjectileDamageEffect，证明“每把武器配置 GE”的项目内先例。
- UShootAttributeSet 已有 IncomingDamage 和 HandleIncomingDamage。
- UShootEffect_DamageSetByCaller 可继续留给投射物、技能等旧调用方。

需要新增的很少：

- 1 个 C++ GameplayEffect 基类。
- 1 个 C++ Execution Calculation。
- 3 个 Blueprint GameplayEffect 资产。
- 3 个 WeaponInstance 蓝图字段配置。

不需要新增：

- 不需要新增每枪 C++ GE 子类。
- 不需要新增 Blueprint GA 子类。
- 不需要改 AbilitySet。
- 不需要复制 Lyra 的 ULyraWeaponStateComponent。
- 不需要复制 Lyra 的 FLyraGameplayEffectContext。

## 最终改动清单

新增文件：

- Source/NewWorldOrder/Public/AbilitySystem/Effects/ShootEffect_DamageBase.h
- Source/NewWorldOrder/Private/AbilitySystem/Effects/ShootEffect_DamageBase.cpp
- Source/NewWorldOrder/Public/AbilitySystem/Executions/ShootDamageExecution.h
- Source/NewWorldOrder/Private/AbilitySystem/Executions/ShootDamageExecution.cpp

修改文件：

- Source/NewWorldOrder/Public/Weapons/ShootRangedWeaponInstance.h
- Source/NewWorldOrder/Private/Weapons/ShootRangedWeaponInstance.cpp
- Source/NewWorldOrder/Public/AbilitySystem/Abilities/ShootGameplayAbility_Weapon_Fire.h
- Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGameplayAbility_Weapon_Fire.cpp
- Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_Rifle.cpp
- Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_Shotgun.cpp
- Source/NewWorldOrder/Private/AbilitySystem/Abilities/ShootGA_Weapon_Fire_Sniper.cpp
- Source/NewWorldOrder/Public/Inventory/Fragments/ShootInventoryFragment_RangedWeaponConfig.h

修改资产：

- /Game/Weapons/Pistol/B_WeaponInstance_Pistol
- /Game/Weapons/Rifle/B_WeaponInstance_Rifle
- /Game/Weapons/Shotgun/B_WeaponInstance_Shotgun
- 三个 ItemDefinition 资产（如删除 Fragment 死字段）

## 第一步：给 WeaponInstance 增加 DamageGameplayEffect

修改 ShootRangedWeaponInstance.h：

```cpp
public:
    UFUNCTION(BlueprintPure, Category="Weapon|Damage")
    TSubclassOf<UGameplayEffect> GetDamageGameplayEffect() const;

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
    TSubclassOf<UGameplayEffect> DamageGameplayEffect;
```

修改 ShootRangedWeaponInstance.cpp：

```cpp
TSubclassOf<UGameplayEffect> UShootRangedWeaponInstance::GetDamageGameplayEffect() const
{
    return DamageGameplayEffect;
}
```

理由：

- 和现有 GetProjectileDamageEffect 一致。
- Fire GA 不再需要每把枪一个 C++ GA 子类。
- 每把枪的 GE 在 WeaponInstance 蓝图里配置，改枪不改代码。

## 第二步：把衰减字段迁到 WeaponInstance

修改 ShootRangedWeaponInstance.h：

```cpp
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon Config")
FRuntimeFloatCurve DistanceDamageFalloff;

UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon Config")
TMap<FGameplayTag, float> MaterialDamageMultiplier;
```

修改 ShootRangedWeaponInstance.cpp：

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

注意：

- 改完这段后，GetDistanceAttenuation 和 GetPhysicalMaterialAttenuation 不再读 Fragment。
- 如果暂时不想动 Fragment，可以保留旧函数体，但最终要切到实例字段。

## 第三步：新增 UShootEffect_DamageBase

ShootEffect_DamageBase.h：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "ShootEffect_DamageBase.generated.h"

UCLASS(Blueprintable, BlueprintType)
class NEWWORLDORDER_API UShootEffect_DamageBase : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UShootEffect_DamageBase();

    float GetBaseDamage() const { return BaseDamage; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Damage")
    float BaseDamage = 10.f;

};
```

ShootEffect_DamageBase.cpp：

```cpp
#include "AbilitySystem/Effects/ShootEffect_DamageBase.h"
#include "AbilitySystem/Executions/ShootDamageExecution.h"

UShootEffect_DamageBase::UShootEffect_DamageBase()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    FGameplayEffectExecutionDefinition Execution;
    Execution.CalculationClass = UShootDamageExecution::StaticClass();
    Executions.Add(Execution);
}
```

## 第四步：新增 UShootDamageExecution

ShootDamageExecution.h：

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ShootDamageExecution.generated.h"

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

ShootDamageExecution.cpp：

```cpp
#include "AbilitySystem/Executions/ShootDamageExecution.h"
#include "AbilitySystem/Effects/ShootEffect_DamageBase.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "Weapons/ShootRangedWeaponInstance.h"
#include "GameplayEffect.h"

UShootDamageExecution::UShootDamageExecution()
{
}

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

    float Damage = DamageGE->GetBaseDamage();

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

说明：

- 不捕获属性，所以不需要 RelevantAttributesToCapture。
- 队伍规则、免疫死亡、伤害数字仍由 ShootAttributeSet 处理。
- 后续 RPG 属性接入时，在这个函数里加属性捕获或 SetByCaller。

## 第五步：创建 3 个 Blueprint GE

在编辑器用 MCP 或手动创建：

- /Game/Weapons/Pistol/GE_Damage_Pistol
- /Game/Weapons/Rifle/GE_Damage_Rifle
- /Game/Weapons/Shotgun/GE_Damage_Shotgun

父类：

- UShootEffect_DamageBase

配置：

- BaseDamage：Pistol 18、Rifle 12、Shotgun 12，与 Lyra Execution Scoped Modifier 一致。
- 伤害类型 Tag 尚未接入项目消费链，本阶段没有为了形式对齐增加无人读取的 `DamageTypeTag` 属性。
- DurationPolicy：继承父类 Instant，不需要手动改。

## 第六步：Fire GA 不再手算伤害

目标：三个 ApplyDamageToTarget 统一收敛到基类，并且只装配 Context，不计算伤害。

在 UShootGameplayAbility_Weapon_Fire.h 增加一个 protected 函数：

```cpp
protected:
    bool ApplyWeaponDamageToTarget(const FHitResult& Hit);
```

在 UShootGameplayAbility_Weapon_Fire.cpp 实现：

```cpp
bool UShootGameplayAbility_Weapon_Fire::ApplyWeaponDamageToTarget(const FHitResult& Hit)
{
    UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
    if (!Weapon)
    {
        return false;
    }

    TSubclassOf<UGameplayEffect> DamageGE = Weapon->GetDamageGameplayEffect();
    if (!DamageGE)
    {
        return false;
    }

    AActor* TargetActor = Hit.GetActor();
    if (!TargetActor)
    {
        return false;
    }

    UAbilitySystemComponent* TargetASC =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
    UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
    if (!TargetASC || !SourceASC)
    {
        return false;
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
        return false;
    }

    ApplyAbilityTagsToGameplayEffectSpec(*SpecHandle.Data.Get(), GetCurrentAbilitySpec());
    SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
    return true;
}
```

然后：

- ShootGA_Weapon_Fire_Rifle.cpp 删除自己的 ApplyDamageToTarget 实现。
- ShootGA_Weapon_Fire_Shotgun.cpp 删除自己的 ApplyDamageToTarget 实现。
- Sniper 尚未配置每枪 GE，本阶段保留自己的旧实现，避免启用时直接变成零伤害。
- Rifle 与 Shotgun 不再需要各自的 DamageGameplayEffectClass；Pistol 继承 Rifle 的共享入口。

注意：

- 不要删除 AddHitResult。
- 不要删除 AddSourceObject。
- 自定义 Context 通过 ASC 创建 Spec 后，要调用 `ApplyAbilityTagsToGameplayEffectSpec`，补齐 UGameplayAbility 原生应用路径会写入的动态标签。
- 基类的 OnRangedWeaponTargetDataReady 仍是 BlueprintNativeEvent，子类可以继续重写表现逻辑，但伤害统一走基类 ApplyWeaponDamageToTarget。

## 第七步：WeaponInstance 蓝图配置

- B_WeaponInstance_Pistol：
  - DamageGameplayEffect = /Game/Weapons/Pistol/GE_Damage_Pistol
  - MaterialDamageMultiplier：Gameplay.Zone.WeakSpot = 2.0
  - DistanceDamageFalloff：`(0,1 Linear) (2000,1 Constant) (2001,0.5 Constant) (25000,0.5 Linear)`。
- B_WeaponInstance_Rifle：
  - DamageGameplayEffect = /Game/Weapons/Rifle/GE_Damage_Rifle
  - MaterialDamageMultiplier：Gameplay.Zone.WeakSpot = 1.5
  - DistanceDamageFalloff：`(0,1 Linear) (2800,1 Linear) (2801,0.5 Linear)`。
- B_WeaponInstance_Shotgun：
  - DamageGameplayEffect = /Game/Weapons/Shotgun/GE_Damage_Shotgun
  - MaterialDamageMultiplier：Gameplay.Zone.WeakSpot = 1.75
  - DistanceDamageFalloff：`(0,1) (600,1) (640,0.7) (2000,0.7) (2001,0.5)`，全部 Linear。

## 第八步：清理 Fragment 死字段

ShootInventoryFragment_RangedWeaponConfig.h 建议删除：

- BaseDamage
- HeadshotMultiplier
- FalloffStart
- FalloffEnd
- FireInterval
- BurstCount
- bAutomaticFire
- AttachSocketHand
- AttachSocketHolster

保留：

- MaxRange
- MagazineSize
- MaxReserve
- AmmoPerShot
- 散布、后坐力、Socket、Montage 等仍有读取的字段

删除前用编辑器引用扫描确认无蓝图引用。

## 第九步：编译与验收

编译：

- 使用 Scripts/Build_Windows.ps1 冷编译。
- 确认 UShootDamageExecution 和 UShootEffect_DamageBase 编译通过。

PIE：

- 单人 PIE 打靶，伤害应等于 GE BaseDamage 乘衰减。
- 打头伤害应高于身体。
- 远距离伤害应低于近距离。
- 切枪后伤害随当前 WeaponInstance 的 GE 变化。
- Listen Server 下服务器结算，客户端血量同步。

## 如果遇到 Lyra 上下文不够时去哪里看

- 射击 TargetData：Source/LyraGame/Weapons/LyraGameplayAbility_RangedWeapon.cpp
- EffectContext：Source/LyraGame/AbilitySystem/Abilities/LyraGameplayAbility.cpp
- 伤害 Execution：Source/LyraGame/AbilitySystem/Executions/LyraDamageExecution.cpp
- WeaponInstance 衰减：Source/LyraGame/Weapons/LyraRangedWeaponInstance.cpp
- GE 选择：/Game/Weapons/GA_Weapon_Fire 蓝图 EventGraph，变量 GE_Damage
- 每枪 GE：/Game/Weapons/Pistol/GE_Damage_Pistol、/ShooterCore/Weapons/Rifle/GE_Damage_RifleAuto、/ShooterCore/Weapons/Shotgun/GE_Damage_Shotgun
