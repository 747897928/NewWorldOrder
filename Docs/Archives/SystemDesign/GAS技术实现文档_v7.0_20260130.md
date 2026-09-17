# 新秩序 - GAS技术实现文档 v7.0

**文档类型**: 技术实现文档（纯C++） **最后更新**: 2025-11-13 **基于**: Lyra示例项目实现思路 **目标**: 提供完整可用的C++代码

**前置条件**:

- 已有AShootCharacterBase基类
- 已有ASC在AShootPlayerState和AEnemyBotCharacter
- 武器系统基于Lyra的装备赋予GA/GE思路

------

## 第一章：AttributeSet实现

### 1.1 ShootAttributeSet.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "ShootAttributeSet.generated.h"

// 使用宏简化属性定义（参考Lyra）
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 角色基础属性集
 * 包含：生命值、护盾、四大属性、派生属性
 */
UCLASS()
class NEWWORLDORDER_API UShootAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UShootAttributeSet();

    // AttributeSet核心函数
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    //~=============================================================================
    // 生命值与护盾
    //~=============================================================================

    UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, Health)

    UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, MaxHealth)

    UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_Shield)
    FGameplayAttributeData Shield;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, Shield)

    UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_MaxShield)
    FGameplayAttributeData MaxShield;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, MaxShield)

    //~=============================================================================
    // 四大核心属性
    //~=============================================================================

    // 体力（影响MaxHP、护盾容量、减伤）
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Vitality)
    FGameplayAttributeData Vitality;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, Vitality)

    // 力量（影响伤害、护甲穿透）
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Strength)
    FGameplayAttributeData Strength;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, Strength)

    // 敏捷（影响移速、换弹速度、射击移速惩罚、翻滚）
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Agility)
    FGameplayAttributeData Agility;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, Agility)

    // 感知（影响爆头倍率、暴击率）
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Perception)
    FGameplayAttributeData Perception;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, Perception)

    //~=============================================================================
    // 派生属性（只读，通过GE计算）
    //~=============================================================================

    // 伤害加成（由力量计算）
    UPROPERTY(BlueprintReadOnly, Category = "Derived", ReplicatedUsing = OnRep_DamageMultiplier)
    FGameplayAttributeData DamageMultiplier;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, DamageMultiplier)

    // 护甲穿透（由力量计算）
    UPROPERTY(BlueprintReadOnly, Category = "Derived", ReplicatedUsing = OnRep_ArmorPenetration)
    FGameplayAttributeData ArmorPenetration;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, ArmorPenetration)

    // 移速倍率（由敏捷计算）
    UPROPERTY(BlueprintReadOnly, Category = "Derived", ReplicatedUsing = OnRep_MoveSpeedMultiplier)
    FGameplayAttributeData MoveSpeedMultiplier;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, MoveSpeedMultiplier)

    // 换弹速度倍率（由敏捷计算）
    UPROPERTY(BlueprintReadOnly, Category = "Derived", ReplicatedUsing = OnRep_ReloadSpeedMultiplier)
    FGameplayAttributeData ReloadSpeedMultiplier;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, ReloadSpeedMultiplier)

    // 爆头伤害倍率（由感知计算）
    UPROPERTY(BlueprintReadOnly, Category = "Derived", ReplicatedUsing = OnRep_HeadshotMultiplier)
    FGameplayAttributeData HeadshotMultiplier;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, HeadshotMultiplier)

    // 暴击率（由感知计算）
    UPROPERTY(BlueprintReadOnly, Category = "Derived", ReplicatedUsing = OnRep_CriticalChance)
    FGameplayAttributeData CriticalChance;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, CriticalChance)

    // 减伤百分比（由体力计算）
    UPROPERTY(BlueprintReadOnly, Category = "Derived", ReplicatedUsing = OnRep_DamageReduction)
    FGameplayAttributeData DamageReduction;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, DamageReduction)

    //~=============================================================================
    // Meta属性（临时，不复制）
    //~=============================================================================

    // 受到的伤害（Meta，用于伤害计算）
    UPROPERTY(BlueprintReadOnly, Category = "Meta")
    FGameplayAttributeData IncomingDamage;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, IncomingDamage)

    // 受到的治疗（Meta，用于治疗计算）
    UPROPERTY(BlueprintReadOnly, Category = "Meta")
    FGameplayAttributeData IncomingHealing;
    ATTRIBUTE_ACCESSORS(UShootAttributeSet, IncomingHealing)

protected:
    //~=============================================================================
    // 复制通知函数
    //~=============================================================================

    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

    UFUNCTION()
    virtual void OnRep_Shield(const FGameplayAttributeData& OldShield);

    UFUNCTION()
    virtual void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);

    UFUNCTION()
    virtual void OnRep_Vitality(const FGameplayAttributeData& OldVitality);

    UFUNCTION()
    virtual void OnRep_Strength(const FGameplayAttributeData& OldStrength);

    UFUNCTION()
    virtual void OnRep_Agility(const FGameplayAttributeData& OldAgility);

    UFUNCTION()
    virtual void OnRep_Perception(const FGameplayAttributeData& OldPerception);

    UFUNCTION()
    virtual void OnRep_DamageMultiplier(const FGameplayAttributeData& OldDamageMultiplier);

    UFUNCTION()
    virtual void OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration);

    UFUNCTION()
    virtual void OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldMoveSpeedMultiplier);

    UFUNCTION()
    virtual void OnRep_ReloadSpeedMultiplier(const FGameplayAttributeData& OldReloadSpeedMultiplier);

    UFUNCTION()
    virtual void OnRep_HeadshotMultiplier(const FGameplayAttributeData& OldHeadshotMultiplier);

    UFUNCTION()
    virtual void OnRep_CriticalChance(const FGameplayAttributeData& OldCriticalChance);

    UFUNCTION()
    virtual void OnRep_DamageReduction(const FGameplayAttributeData& OldDamageReduction);

private:
    // Clamping辅助函数
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
```

### 1.2 ShootAttributeSet.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/ShootAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UShootAttributeSet::UShootAttributeSet()
{
    // 初始化默认值（Lv1，0点属性）
    InitHealth(100.0f);
    InitMaxHealth(100.0f);
    InitShield(0.0f);
    InitMaxShield(20.0f); // 20% of MaxHealth
    
    InitVitality(0.0f);
    InitStrength(0.0f);
    InitAgility(0.0f);
    InitPerception(0.0f);
    
    InitDamageMultiplier(1.0f);
    InitArmorPenetration(0.0f);
    InitMoveSpeedMultiplier(1.0f);
    InitReloadSpeedMultiplier(1.0f);
    InitHeadshotMultiplier(2.0f); // 基础爆头倍率
    InitCriticalChance(0.0f);
    InitDamageReduction(0.0f);
}

void UShootAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    
    // Clamp属性值（参考Lyra的ClampAttribute）
    ClampAttribute(Attribute, NewValue);
}

void UShootAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    // 获取Source和Target
    FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
    UAbilitySystemComponent* SourceASC = Context.GetOriginalInstigatorAbilitySystemComponent();
    const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer& TargetTags = *Data.EffectSpec.CapturedTargetTags.GetAggregatedTags();

    // 获取修改的幅度
    float DeltaValue = 0.0f;
    if (Data.EvaluatedData.ModifierOp == EGameplayModOp::Type::Additive)
    {
        DeltaValue = Data.EvaluatedData.Magnitude;
    }

    // 获取Target Actor
    AActor* TargetActor = nullptr;
    AController* TargetController = nullptr;
    if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
    {
        TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
        TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
    }

    //~=============================================================================
    // 处理伤害（IncomingDamage）
    //~=============================================================================
    if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
    {
        const float LocalIncomingDamage = GetIncomingDamage();
        SetIncomingDamage(0.0f); // 清空Meta属性

        if (LocalIncomingDamage > 0.0f)
        {
            // 先扣除护盾
            const float OldShield = GetShield();
            const float NewShield = FMath::Max(0.0f, OldShield - LocalIncomingDamage);
            SetShield(NewShield);

            // 剩余伤害扣除生命值
            const float RemainingDamage = FMath::Max(0.0f, LocalIncomingDamage - OldShield);
            if (RemainingDamage > 0.0f)
            {
                const float NewHealth = FMath::Max(0.0f, GetHealth() - RemainingDamage);
                SetHealth(NewHealth);

                // 检查死亡
                if (NewHealth <= 0.0f)
                {
                    // TODO: 触发死亡逻辑
                    if (TargetActor)
                    {
                        // 可以在这里广播死亡事件
                        // OnDeathDelegate.Broadcast(...);
                    }
                }
            }
        }
    }

    //~=============================================================================
    // 处理治疗（IncomingHealing）
    //~=============================================================================
    else if (Data.EvaluatedData.Attribute == GetIncomingHealingAttribute())
    {
        const float LocalIncomingHealing = GetIncomingHealing();
        SetIncomingHealing(0.0f); // 清空Meta属性

        if (LocalIncomingHealing > 0.0f)
        {
            const float NewHealth = FMath::Clamp(GetHealth() + LocalIncomingHealing, 0.0f, GetMaxHealth());
            SetHealth(NewHealth);
        }
    }

    //~=============================================================================
    // 处理MaxHealth变化（重新计算MaxShield）
    //~=============================================================================
    else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute())
    {
        // 护盾上限 = (20% + Vitality × 1%) × MaxHP
        const float ShieldCapacityPercent = 0.2f + (GetVitality() * 0.01f);
        const float NewMaxShield = GetMaxHealth() * ShieldCapacityPercent;
        SetMaxShield(NewMaxShield);

        // Clamp当前护盾
        if (GetShield() > NewMaxShield)
        {
            SetShield(NewMaxShield);
        }

        // Clamp当前生命值
        if (GetHealth() > GetMaxHealth())
        {
            SetHealth(GetMaxHealth());
        }
    }

    //~=============================================================================
    // 处理体力变化（影响MaxHealth、DamageReduction）
    //~=============================================================================
    else if (Data.EvaluatedData.Attribute == GetVitalityAttribute())
    {
        // 体力影响的属性会通过GE自动更新，这里只需要确保触发重新计算
        // MaxHealth的GE会自动触发
        // DamageReduction的GE会自动触发
    }

    //~=============================================================================
    // 处理敏捷变化（影响移速）
    //~=============================================================================
    //在我之前的ShootAttributeSet.cpp中，处理移速变化时直接修改了CharacterMovement->MaxWalkSpeed，但这在客户端的OnRep通知中可能不会触发。
	//正确做法（参考Lyra）：移速应该通过GameplayTag来管理，或者在角色Tick中持续同步。
}

void UShootAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 复制所有属性（参考Lyra的REPLIFETIME_CONDITION_NOTIFY）
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, Shield, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
    
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, Vitality, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, Strength, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, Agility, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, Perception, COND_None, REPNOTIFY_Always);
    
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, DamageMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, ArmorPenetration, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, MoveSpeedMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, ReloadSpeedMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, HeadshotMultiplier, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, CriticalChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UShootAttributeSet, DamageReduction, COND_None, REPNOTIFY_Always);
}

//~=============================================================================
// 复制通知函数实现（参考Lyra的GAMEPLAYATTRIBUTE_REPNOTIFY）
//~=============================================================================

void UShootAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, Health, OldHealth);
}

void UShootAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, MaxHealth, OldMaxHealth);
}

void UShootAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, Shield, OldShield);
}

void UShootAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, MaxShield, OldMaxShield);
}

void UShootAttributeSet::OnRep_Vitality(const FGameplayAttributeData& OldVitality)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, Vitality, OldVitality);
}

void UShootAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, Strength, OldStrength);
}

void UShootAttributeSet::OnRep_Agility(const FGameplayAttributeData& OldAgility)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, Agility, OldAgility);
}

void UShootAttributeSet::OnRep_Perception(const FGameplayAttributeData& OldPerception)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, Perception, OldPerception);
}

void UShootAttributeSet::OnRep_DamageMultiplier(const FGameplayAttributeData& OldDamageMultiplier)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, DamageMultiplier, OldDamageMultiplier);
}

void UShootAttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, ArmorPenetration, OldArmorPenetration);
}

void UShootAttributeSet::OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldMoveSpeedMultiplier)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, MoveSpeedMultiplier, OldMoveSpeedMultiplier);
}

void UShootAttributeSet::OnRep_ReloadSpeedMultiplier(const FGameplayAttributeData& OldReloadSpeedMultiplier)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, ReloadSpeedMultiplier, OldReloadSpeedMultiplier);
}

void UShootAttributeSet::OnRep_HeadshotMultiplier(const FGameplayAttributeData& OldHeadshotMultiplier)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, HeadshotMultiplier, OldHeadshotMultiplier);
}

void UShootAttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldCriticalChance)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, CriticalChance, OldCriticalChance);
}

void UShootAttributeSet::OnRep_DamageReduction(const FGameplayAttributeData& OldDamageReduction)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UShootAttributeSet, DamageReduction, OldDamageReduction);
}

//~=============================================================================
// Clamping辅助函数
//~=============================================================================

void UShootAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetShieldAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxShield());
    }
    else if (Attribute == GetMaxHealthAttribute())
    {
        NewValue = FMath::Max(1.0f, NewValue); // 至少1点血
    }
    else if (Attribute == GetMaxShieldAttribute())
    {
        NewValue = FMath::Max(0.0f, NewValue);
    }
    else if (Attribute == GetVitalityAttribute() || 
             Attribute == GetStrengthAttribute() || 
             Attribute == GetAgilityAttribute() || 
             Attribute == GetPerceptionAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 30.0f); // 属性0-30
    }
    else if (Attribute == GetCriticalChanceAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f); // 暴击率0-100%
    }
    else if (Attribute == GetDamageReductionAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, 0.75f); // 减伤最多75%
    }
}
```

在AShootCharacterBase中添加移速同步
```cpp
// AShootCharacterBase.h
protected:
    virtual void UpdateMovementSpeed();

// AShootCharacterBase.cpp
void AShootCharacterBase::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    
    // 每帧更新移速（基于ASC的MoveSpeedMultiplier属性）
    UpdateMovementSpeed();
}

void AShootCharacterBase::UpdateMovementSpeed()
{
    if (AbilitySystemComponent && AttributeSet)
    {
        const float MoveSpeedMultiplier = AttributeSet->GetMoveSpeedMultiplier();
        
        if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
        {
            // 基础移速600
            const float BaseMoveSpeed = 600.0f;
            const float DesiredSpeed = BaseMoveSpeed * MoveSpeedMultiplier;
            
            // 只在数值变化时更新（避免每帧都设置）
            if (!FMath::IsNearlyEqual(MovementComp->MaxWalkSpeed, DesiredSpeed, 1.0f))
            {
                MovementComp->MaxWalkSpeed = DesiredSpeed;
            }
        }
    }
}
```


------



## 第二章：属性计算GameplayEffect（C++实现）

### 2.1 自定义MMC（Modifier Magnitude Calculation）

由于属性计算涉及**三段递减**，需要自定义MMC类来实现复杂的计算逻辑。

#### 2.1.1 MMC_MaxHealthFromVitality.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_MaxHealthFromVitality.generated.h"

/**
 * 计算MaxHealth：基于Vitality的三段递减
 * 0-10点: 每点+12 HP
 * 11-20点: 每点+8 HP
 * 21-30点: 每点+6 HP
 * 
 * 基础HP = 100
 */
UCLASS()
class NEWWORLDORDER_API UMMC_MaxHealthFromVitality : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UMMC_MaxHealthFromVitality();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition VitalityDef;
};
```

#### 2.1.2 MMC_MaxHealthFromVitality.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/MMC/MMC_MaxHealthFromVitality.h"
#include "AbilitySystem/ShootAttributeSet.h"

UMMC_MaxHealthFromVitality::UMMC_MaxHealthFromVitality()
{
    // 定义要捕获的属性（Vitality）
    VitalityDef.AttributeToCapture = UShootAttributeSet::GetVitalityAttribute();
    VitalityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    VitalityDef.bSnapshot = false;

    // 注册捕获定义
    RelevantAttributesToCapture.Add(VitalityDef);
}

float UMMC_MaxHealthFromVitality::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float Vitality = 0.0f;
    GetCapturedAttributeMagnitude(VitalityDef, Spec, EvaluationParameters, Vitality);
    Vitality = FMath::Max(0.0f, Vitality);

    // 计算MaxHealth
    float MaxHealth = 100.0f; // 基础值

    // 三段递减计算
    if (Vitality <= 10.0f)
    {
        // 0-10点: 每点+12
        MaxHealth += Vitality * 12.0f;
    }
    else if (Vitality <= 20.0f)
    {
        // 0-10点: 120
        // 11-20点: 每点+8
        MaxHealth += 120.0f + (Vitality - 10.0f) * 8.0f;
    }
    else // Vitality > 20
    {
        // 0-10点: 120
        // 11-20点: 80
        // 21-30点: 每点+6
        MaxHealth += 120.0f + 80.0f + (Vitality - 20.0f) * 6.0f;
    }

    return MaxHealth;
}
```

#### 2.1.3 MMC_DamageFromStrength.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_DamageFromStrength.generated.h"

/**
 * 计算伤害加成倍率：基于Strength的三段递减
 * 0-15点: 每点+2%
 * 16-25点: 每点+1.5%
 * 26-30点: 每点+1%
 * 
 * 返回值：伤害倍率（1.0 = 100%基础伤害）
 */
UCLASS()
class NEWWORLDORDER_API UMMC_DamageFromStrength : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UMMC_DamageFromStrength();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition StrengthDef;
};
```

#### 2.1.4 MMC_DamageFromStrength.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/MMC/MMC_DamageFromStrength.h"
#include "AbilitySystem/ShootAttributeSet.h"

UMMC_DamageFromStrength::UMMC_DamageFromStrength()
{
    StrengthDef.AttributeToCapture = UShootAttributeSet::GetStrengthAttribute();
    StrengthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    StrengthDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(StrengthDef);
}

float UMMC_DamageFromStrength::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float Strength = 0.0f;
    GetCapturedAttributeMagnitude(StrengthDef, Spec, EvaluationParameters, Strength);
    Strength = FMath::Max(0.0f, Strength);

    // 计算伤害加成百分比
    float BonusPercent = 0.0f;

    if (Strength <= 15.0f)
    {
        // 0-15点: 每点+2%
        BonusPercent = Strength * 2.0f;
    }
    else if (Strength <= 25.0f)
    {
        // 0-15点: 30%
        // 16-25点: 每点+1.5%
        BonusPercent = 30.0f + (Strength - 15.0f) * 1.5f;
    }
    else // Strength > 25
    {
        // 0-15点: 30%
        // 16-25点: 15%
        // 26-30点: 每点+1%
        BonusPercent = 30.0f + 15.0f + (Strength - 25.0f) * 1.0f;
    }

    // 返回伤害倍率（1.0 + 加成百分比）
    return 1.0f + (BonusPercent / 100.0f);
}
```

#### 2.1.5 MMC_ArmorPenetrationFromStrength.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ArmorPenetrationFromStrength.generated.h"

/**
 * 计算护甲穿透：Strength × 0.8%
 * 返回值：穿透百分比（0.24 = 24%穿透）
 */
UCLASS()
class NEWWORLDORDER_API UMMC_ArmorPenetrationFromStrength : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UMMC_ArmorPenetrationFromStrength();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition StrengthDef;
};
```

#### 2.1.6 MMC_ArmorPenetrationFromStrength.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/MMC/MMC_ArmorPenetrationFromStrength.h"
#include "AbilitySystem/ShootAttributeSet.h"

UMMC_ArmorPenetrationFromStrength::UMMC_ArmorPenetrationFromStrength()
{
    StrengthDef.AttributeToCapture = UShootAttributeSet::GetStrengthAttribute();
    StrengthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    StrengthDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(StrengthDef);
}

float UMMC_ArmorPenetrationFromStrength::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float Strength = 0.0f;
    GetCapturedAttributeMagnitude(StrengthDef, Spec, EvaluationParameters, Strength);
    Strength = FMath::Max(0.0f, Strength);

    // 护甲穿透 = Strength × 0.8%
    return Strength * 0.008f; // 0.008 = 0.8%
}
```

#### 2.1.7 MMC_MoveSpeedFromAgility.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_MoveSpeedFromAgility.generated.h"

/**
 * 计算移速倍率：基于Agility的三段递减
 * 0-15点: 每点+1%
 * 16-25点: 每点+0.75%
 * 26-30点: 每点+0.5%
 * 
 * 返回值：移速倍率（1.25 = 125%移速）
 */
UCLASS()
class NEWWORLDORDER_API UMMC_MoveSpeedFromAgility : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UMMC_MoveSpeedFromAgility();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition AgilityDef;
};
```

#### 2.1.8 MMC_MoveSpeedFromAgility.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/MMC/MMC_MoveSpeedFromAgility.h"
#include "AbilitySystem/ShootAttributeSet.h"

UMMC_MoveSpeedFromAgility::UMMC_MoveSpeedFromAgility()
{
    AgilityDef.AttributeToCapture = UShootAttributeSet::GetAgilityAttribute();
    AgilityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    AgilityDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(AgilityDef);
}

float UMMC_MoveSpeedFromAgility::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float Agility = 0.0f;
    GetCapturedAttributeMagnitude(AgilityDef, Spec, EvaluationParameters, Agility);
    Agility = FMath::Max(0.0f, Agility);

    // 计算移速加成百分比
    float BonusPercent = 0.0f;

    if (Agility <= 15.0f)
    {
        // 0-15点: 每点+1%
        BonusPercent = Agility * 1.0f;
    }
    else if (Agility <= 25.0f)
    {
        // 0-15点: 15%
        // 16-25点: 每点+0.75%
        BonusPercent = 15.0f + (Agility - 15.0f) * 0.75f;
    }
    else // Agility > 25
    {
        // 0-15点: 15%
        // 16-25点: 7.5%
        // 26-30点: 每点+0.5%
        BonusPercent = 15.0f + 7.5f + (Agility - 25.0f) * 0.5f;
    }

    // 返回移速倍率
    return 1.0f + (BonusPercent / 100.0f);
}
```

#### 2.1.9 MMC_ReloadSpeedFromAgility.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ReloadSpeedFromAgility.generated.h"

/**
 * 计算换弹速度倍率：Agility × 1.5%
 * 换弹时间 = 基础时间 × (1 - ReloadSpeedBonus)
 * 
 * 返回值：换弹速度加成（0.45 = -45%换弹时间）
 */
UCLASS()
class NEWWORLDORDER_API UMMC_ReloadSpeedFromAgility : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UMMC_ReloadSpeedFromAgility();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition AgilityDef;
};
```

#### 2.1.10 MMC_ReloadSpeedFromAgility.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/MMC/MMC_ReloadSpeedFromAgility.h"
#include "AbilitySystem/ShootAttributeSet.h"

UMMC_ReloadSpeedFromAgility::UMMC_ReloadSpeedFromAgility()
{
    AgilityDef.AttributeToCapture = UShootAttributeSet::GetAgilityAttribute();
    AgilityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    AgilityDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(AgilityDef);
}

float UMMC_ReloadSpeedFromAgility::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float Agility = 0.0f;
    GetCapturedAttributeMagnitude(AgilityDef, Spec, EvaluationParameters, Agility);
    Agility = FMath::Max(0.0f, Agility);

    // 换弹速度加成 = Agility × 1.5%
    float ReloadSpeedBonus = Agility * 0.015f; // 0.015 = 1.5%

    // 返回换弹时间缩减倍率（用于计算 FinalReloadTime = BaseReloadTime × (1 - ReloadSpeedBonus)）
    return ReloadSpeedBonus;
}
```

#### 2.1.11 MMC_HeadshotMultiplierFromPerception.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_HeadshotMultiplierFromPerception.generated.h"

/**
 * 计算爆头倍率：基础倍率 + Perception × 1.5%
 * 基础倍率：步枪2.0，狙击枪2.5
 * 
 * 返回值：爆头伤害倍率（2.45 = 245%伤害）
 */
UCLASS()
class NEWWORLDORDER_API UMMC_HeadshotMultiplierFromPerception : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UMMC_HeadshotMultiplierFromPerception();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition PerceptionDef;
};
```

#### 2.1.12 MMC_HeadshotMultiplierFromPerception.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/MMC/MMC_HeadshotMultiplierFromPerception.h"
#include "AbilitySystem/ShootAttributeSet.h"

UMMC_HeadshotMultiplierFromPerception::UMMC_HeadshotMultiplierFromPerception()
{
    PerceptionDef.AttributeToCapture = UShootAttributeSet::GetPerceptionAttribute();
    PerceptionDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    PerceptionDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(PerceptionDef);
}

float UMMC_HeadshotMultiplierFromPerception::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float Perception = 0.0f;
    GetCapturedAttributeMagnitude(PerceptionDef, Spec, EvaluationParameters, Perception);
    Perception = FMath::Max(0.0f, Perception);

    // 基础爆头倍率（2.0，狙击枪会额外加0.5）
    float BaseMultiplier = 2.0f;

    // 感知加成：Perception × 1.5%
    float BonusMultiplier = Perception * 0.015f; // 0.015 = 1.5%

    return BaseMultiplier + BonusMultiplier;
}
```

#### 2.1.13 MMC_CriticalChanceFromPerception.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_CriticalChanceFromPerception.generated.h"

/**
 * 计算暴击率：Perception × 0.75%
 * 
 * 返回值：暴击概率（0.225 = 22.5%暴击率）
 */
UCLASS()
class NEWWORLDORDER_API UMMC_CriticalChanceFromPerception : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UMMC_CriticalChanceFromPerception();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition PerceptionDef;
};
```

#### 2.1.14 MMC_CriticalChanceFromPerception.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/MMC/MMC_CriticalChanceFromPerception.h"
#include "AbilitySystem/ShootAttributeSet.h"

UMMC_CriticalChanceFromPerception::UMMC_CriticalChanceFromPerception()
{
    PerceptionDef.AttributeToCapture = UShootAttributeSet::GetPerceptionAttribute();
    PerceptionDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    PerceptionDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(PerceptionDef);
}

float UMMC_CriticalChanceFromPerception::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float Perception = 0.0f;
    GetCapturedAttributeMagnitude(PerceptionDef, Spec, EvaluationParameters, Perception);
    Perception = FMath::Max(0.0f, Perception);

    // 暴击率 = Perception × 0.75%
    return Perception * 0.0075f; // 0.0075 = 0.75%
}
```

#### 2.1.15 MMC_DamageReductionFromVitality.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_DamageReductionFromVitality.generated.h"

/**
 * 计算减伤：Vitality × 0.2%
 * 
 * 返回值：减伤百分比（0.06 = 6%减伤）
 */
UCLASS()
class NEWWORLDORDER_API UMMC_DamageReductionFromVitality : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UMMC_DamageReductionFromVitality();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition VitalityDef;
};
```

#### 2.1.16 MMC_DamageReductionFromVitality.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/MMC/MMC_DamageReductionFromVitality.h"
#include "AbilitySystem/ShootAttributeSet.h"

UMMC_DamageReductionFromVitality::UMMC_DamageReductionFromVitality()
{
    VitalityDef.AttributeToCapture = UShootAttributeSet::GetVitalityAttribute();
    VitalityDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    VitalityDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(VitalityDef);
}

float UMMC_DamageReductionFromVitality::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float Vitality = 0.0f;
    GetCapturedAttributeMagnitude(VitalityDef, Spec, EvaluationParameters, Vitality);
    Vitality = FMath::Max(0.0f, Vitality);

    // 减伤 = Vitality × 0.2%
    return Vitality * 0.002f; // 0.002 = 0.2%
}
```

### 2.2 GameplayEffect C++类定义

#### 2.2.1 GE_AttributeCalculation.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_AttributeCalculation.generated.h"

/**
 * 属性计算GE基类
 * 所有派生属性的计算GE都继承自这个类
 */
UCLASS()
class NEWWORLDORDER_API UGE_AttributeCalculation : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_AttributeCalculation();
};
```

#### 2.2.2 GE_AttributeCalculation.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/GameplayEffects/GE_AttributeCalculation.h"

UGE_AttributeCalculation::UGE_AttributeCalculation()
{
    // 配置为Infinite（持续生效）
    DurationPolicy = EGameplayEffectDurationType::Infinite;

    // 不堆叠
    StackingType = EGameplayEffectStackingType::None;

    // 定期更新（每0.5秒重新计算一次，确保属性变化后及时更新派生属性）
    Period = 0.5f;
    bExecutePeriodicEffectOnApplication = true;
}
```

#### 2.2.3 GE_MaxHealthCalculation.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GE_AttributeCalculation.h"
#include "GE_MaxHealthCalculation.generated.h"

/**
 * MaxHealth计算GE
 * 使用MMC_MaxHealthFromVitality计算MaxHealth
 */
UCLASS()
class NEWWORLDORDER_API UGE_MaxHealthCalculation : public UGE_AttributeCalculation
{
    GENERATED_BODY()

public:
    UGE_MaxHealthCalculation();
};
```

#### 2.2.4 GE_MaxHealthCalculation.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/GameplayEffects/GE_MaxHealthCalculation.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/MMC/MMC_MaxHealthFromVitality.h"

UGE_MaxHealthCalculation::UGE_MaxHealthCalculation()
{
    // 添加Modifier
    FGameplayModifierInfo ModifierInfo;
    ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(UMMC_MaxHealthFromVitality::StaticClass());
    ModifierInfo.ModifierOp = EGameplayModOp::Override; // 覆盖而非相加
    ModifierInfo.Attribute = UShootAttributeSet::GetMaxHealthAttribute();

    Modifiers.Add(ModifierInfo);
}
```

#### 2.2.5 其他GE类（按相同模式）

```cpp
// GE_DamageMultiplierCalculation.h/cpp
// GE_ArmorPenetrationCalculation.h/cpp
// GE_MoveSpeedCalculation.h/cpp
// GE_ReloadSpeedCalculation.h/cpp
// GE_HeadshotMultiplierCalculation.h/cpp
// GE_CriticalChanceCalculation.h/cpp
// GE_DamageReductionCalculation.h/cpp

// 实现方式完全相同，只是使用不同的MMC类和目标属性
```

为了简洁，我给出一个通用模板：

#### 2.2.6 通用GE模板

```cpp
// GE_[AttributeName]Calculation.h

#pragma once

#include "CoreMinimal.h"
#include "GE_AttributeCalculation.h"
#include "GE_[AttributeName]Calculation.generated.h"

UCLASS()
class NEWWORLDORDER_API UGE_[AttributeName]Calculation : public UGE_AttributeCalculation
{
    GENERATED_BODY()

public:
    UGE_[AttributeName]Calculation();
};

// GE_[AttributeName]Calculation.cpp

#include "AbilitySystem/GameplayEffects/GE_[AttributeName]Calculation.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/MMC/MMC_[AttributeName].h"

UGE_[AttributeName]Calculation::UGE_[AttributeName]Calculation()
{
    FGameplayModifierInfo ModifierInfo;
    ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(UMMC_[AttributeName]::StaticClass());
    ModifierInfo.ModifierOp = EGameplayModOp::Override;
    ModifierInfo.Attribute = UShootAttributeSet::Get[AttributeName]Attribute();

    Modifiers.Add(ModifierInfo);
}
```

### 2.3 应用这些GE（在角色初始化时）

#### 2.3.1 AShootCharacterBase::InitializeAttributes()

```cpp
// 在AShootCharacterBase.h中添加
protected:
    // 属性计算GE（在编辑器中配置）
    UPROPERTY(EditDefaultsOnly, Category = "Abilities|AttributeCalculation")
    TArray<TSubclassOf<UGameplayEffect>> AttributeCalculationEffects;

    virtual void InitializeAttributeCalculation();

// 在AShootCharacterBase.cpp中实现

void AShootCharacterBase::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority() && AbilitySystemComponent)
    {
        // 应用所有属性计算GE
        InitializeAttributeCalculation();
    }
}

void AShootCharacterBase::InitializeAttributeCalculation()
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    // 应用所有属性计算GE（Infinite Duration，持续生效）
    for (const TSubclassOf<UGameplayEffect>& GEClass : AttributeCalculationEffects)
    {
        if (GEClass)
        {
            ApplyEffectToSelf(GEClass, 1.0f);
        }
    }
}
```

#### 2.3.2 在编辑器中配置

在`BP_ShootCharacter`和`BP_EnemyBotCharacter`的蓝图中：

```
AttributeCalculationEffects数组添加：
[0] GE_MaxHealthCalculation
[1] GE_DamageMultiplierCalculation
[2] GE_ArmorPenetrationCalculation
[3] GE_MoveSpeedCalculation
[4] GE_ReloadSpeedCalculation
[5] GE_HeadshotMultiplierCalculation
[6] GE_CriticalChanceCalculation
[7] GE_DamageReductionCalculation
```

### 2.4 角色固定特性GE

#### 2.4.1 男主固定特性：GE_MaleCharacterTraits.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_MaleCharacterTraits.generated.h"

/**
 * 男主角色固定特性
 * - MaxHealth +15%
 * - MoveSpeed -5%
 */
UCLASS()
class NEWWORLDORDER_API UGE_MaleCharacterTraits : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_MaleCharacterTraits();
};
```

#### 2.4.2 GE_MaleCharacterTraits.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/GameplayEffects/GE_MaleCharacterTraits.h"
#include "AbilitySystem/ShootAttributeSet.h"

UGE_MaleCharacterTraits::UGE_MaleCharacterTraits()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;

    // MaxHealth +15%
    FGameplayModifierInfo MaxHealthModifier;
    MaxHealthModifier.ModifierMagnitude = FScalableFloat(1.15f);
    MaxHealthModifier.ModifierOp = EGameplayModOp::Multiply;
    MaxHealthModifier.Attribute = UShootAttributeSet::GetMaxHealthAttribute();
    Modifiers.Add(MaxHealthModifier);

    // MoveSpeed -5%
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.ModifierMagnitude = FScalableFloat(0.95f);
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Multiply;
    MoveSpeedModifier.Attribute = UShootAttributeSet::GetMoveSpeedMultiplierAttribute();
    Modifiers.Add(MoveSpeedModifier);
}
```

#### 2.4.3 女主固定特性：GE_FemaleCharacterTraits.h/cpp

```cpp
// GE_FemaleCharacterTraits.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_FemaleCharacterTraits.generated.h"

/**
 * 女主角色固定特性
 * - MaxHealth -10%
 * - MoveSpeed +15%
 */
UCLASS()
class NEWWORLDORDER_API UGE_FemaleCharacterTraits : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_FemaleCharacterTraits();
};

// GE_FemaleCharacterTraits.cpp

#include "AbilitySystem/GameplayEffects/GE_FemaleCharacterTraits.h"
#include "AbilitySystem/ShootAttributeSet.h"

UGE_FemaleCharacterTraits::UGE_FemaleCharacterTraits()
{
    DurationPolicy = EGameplayEffectDurationType::Infinite;

    // MaxHealth -10%
    FGameplayModifierInfo MaxHealthModifier;
    MaxHealthModifier.ModifierMagnitude = FScalableFloat(0.9f);
    MaxHealthModifier.ModifierOp = EGameplayModOp::Multiply;
    MaxHealthModifier.Attribute = UShootAttributeSet::GetMaxHealthAttribute();
    Modifiers.Add(MaxHealthModifier);

    // MoveSpeed +15%
    FGameplayModifierInfo MoveSpeedModifier;
    MoveSpeedModifier.ModifierMagnitude = FScalableFloat(1.15f);
    MoveSpeedModifier.ModifierOp = EGameplayModOp::Multiply;
    MoveSpeedModifier.Attribute = UShootAttributeSet::GetMoveSpeedMultiplierAttribute();
    Modifiers.Add(MoveSpeedModifier);
}
```

#### 2.4.4 应用角色特性（在男主/女主Character类中）

```cpp
// 在AShootCharacter.h中添加
protected:
    UPROPERTY(EditDefaultsOnly, Category = "Abilities")
    TSubclassOf<UGameplayEffect> CharacterTraitsEffect;

// 在AShootCharacter.cpp的BeginPlay中
void AShootCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority() && AbilitySystemComponent && CharacterTraitsEffect)
    {
        ApplyEffectToSelf(CharacterTraitsEffect, 1.0f);
    }
}

// 在BP_MaleCharacter中配置：CharacterTraitsEffect = GE_MaleCharacterTraits
// 在BP_FemaleCharacter中配置：CharacterTraitsEffect = GE_FemaleCharacterTraits
```

### 2.5 男主"钢铁意志"被动触发GE

#### 2.5.1 GE_IronWill.h

```cpp
// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_IronWill.generated.h"

/**
 * 男主固定特性：钢铁意志
 * 受到伤害时10%概率触发：护盾+20% MaxHP，持续3秒
 * 
 * 注意：触发概率由Cue或代码处理，这个GE只负责护盾效果
 */
UCLASS()
class NEWWORLDORDER_API UGE_IronWill : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_IronWill();
};
```

#### 2.5.2 GE_IronWill.cpp

```cpp
// Copyright ZhaoYiJie

#include "AbilitySystem/GameplayEffects/GE_IronWill.h"
#include "AbilitySystem/ShootAttributeSet.h"

UGE_IronWill::UGE_IronWill()
{
    // 持续3秒
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FScalableFloat(3.0f);

    // 护盾 = 20% MaxHP（Instant，一次性赋予）
    FGameplayModifierInfo ShieldModifier;
    ShieldModifier.ModifierMagnitude = FScalableFloat(0.2f);
    ShieldModifier.ModifierOp = EGameplayModOp::Multiply; // 基于MaxHealth的20%
    ShieldModifier.Attribute = UShootAttributeSet::GetShieldAttribute();

    // 使用SetByCaller（需要在应用时传入MaxHealth的值）
    // 或者使用MMC捕获MaxHealth

    // 为了简化，我们使用一个自定义MMC
    // 这里先占位，稍后实现MMC_ShieldFromMaxHealth
    
    Modifiers.Add(ShieldModifier);
}
```

#### 2.5.3 MMC_ShieldFromMaxHealth（计算20% MaxHP的护盾）

```cpp
// MMC_ShieldFromMaxHealth.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_ShieldFromMaxHealth.generated.h"

UCLASS()
class NEWWORLDORDER_API UMMC_ShieldFromMaxHealth : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UMMC_ShieldFromMaxHealth();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
};

// MMC_ShieldFromMaxHealth.cpp

#include "AbilitySystem/MMC/MMC_ShieldFromMaxHealth.h"
#include "AbilitySystem/ShootAttributeSet.h"

UMMC_ShieldFromMaxHealth::UMMC_ShieldFromMaxHealth()
{
    MaxHealthDef.AttributeToCapture = UShootAttributeSet::GetMaxHealthAttribute();
    MaxHealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
    MaxHealthDef.bSnapshot = false;

    RelevantAttributesToCapture.Add(MaxHealthDef);
}

float UMMC_ShieldFromMaxHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    float MaxHealth = 0.0f;
    GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvaluationParameters, MaxHealth);
    MaxHealth = FMath::Max(0.0f, MaxHealth);

    // 护盾 = 20% MaxHP
    return MaxHealth * 0.2f;
}
```

#### 2.5.4 更新GE_IronWill使用MMC

```cpp
// 更新GE_IronWill.cpp

UGE_IronWill::UGE_IronWill()
{
    DurationPolicy = EGameplayEffectDurationType::HasDuration;
    DurationMagnitude = FScalableFloat(3.0f);

    // 护盾 = 20% MaxHP（使用MMC计算）
    FGameplayModifierInfo ShieldModifier;
    ShieldModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(UMMC_ShieldFromMaxHealth::StaticClass());
    ShieldModifier.ModifierOp = EGameplayModOp::Additive; // 直接加到Shield上
    ShieldModifier.Attribute = UShootAttributeSet::GetShieldAttribute();

    Modifiers.Add(ShieldModifier);
}
```

#### 2.5.5 触发"钢铁意志"的逻辑（在AttributeSet中）

```cpp
// 在ShootAttributeSet.cpp的PostGameplayEffectExecute中添加

// 处理伤害（IncomingDamage）
if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
{
    const float LocalIncomingDamage = GetIncomingDamage();
    SetIncomingDamage(0.0f);

    if (LocalIncomingDamage > 0.0f)
    {
        // 先扣除护盾...（之前的代码）

        // 检查是否触发"钢铁意志"（男主专属）
        if (TargetActor && TargetActor->ActorHasTag(FName("Male"))) // 或者用其他方式判断是男主
        {
            // 10%概率触发
            if (FMath::FRand() < 0.1f)
            {
                // 应用钢铁意志GE
                if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
                {
                    // 检查CD（20秒CD）
                    // 可以用GameplayTag标记CD状态，或用GE的Cooldown机制
                    
                    // 简化实现：直接应用GE
                    FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
                    ContextHandle.AddSourceObject(TargetActor);
                    
                    // 假设已经定义了GE_IronWill的类
                    TSubclassOf<UGameplayEffect> IronWillGE = UGE_IronWill::StaticClass();
                    FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(IronWillGE, 1.0f, ContextHandle);
                    
                    if (SpecHandle.IsValid())
                    {
                        TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
                    }
                }
            }
        }
    }
}
```

**注意**：实际项目中，触发逻辑应该放在一个独立的被动技能GA中，而不是直接写在AttributeSet里。这里只是演示概念。