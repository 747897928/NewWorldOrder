---
note_id: GAS-002
title: TargetData预测流程
category: GAS
status: Active
created: 2025-11-06
updated: 2025-11-06
---

# TargetData预测流程

## 核心要点

- TargetData用于客户端预测和服务器验证
- 客户端生成TargetData并本地应用效果（预测）
- 服务器接收TargetData并验证后应用（权威）
- 遵循Lyra的预测窗口模式

## 预测流程

完整流程：
```
1. 客户端：Ability激活
2. 客户端：开启预测窗口
3. 客户端：执行逻辑（射线检测、计算伤害）
4. 客户端：生成TargetData
5. 客户端：本地应用效果（预测）
6. 客户端：发送TargetData到服务器
7. 服务器：接收TargetData
8. 服务器：验证合法性
9. 服务器：应用效果（权威）
10. 服务器：回传给所有客户端
11. 客户端：接收权威结果，纠正预测（如有偏差）
```

## 代码实现

Ability中使用AbilityTask：
```cpp
void UShootGA_Weapon_Fire::ActivateAbility(...)
{
    // 创建WaitTargetData任务
    UAbilityTask_WaitTargetData* Task = UAbilityTask_WaitTargetData::WaitTargetData(
        this,
        TEXT("WaitTargetData"),
        EGameplayTargetingConfirmation::Instant,
        TargetDataGenerator  // 自定义Target生成器
    );

    // 绑定回调
    Task->ValidData.AddDynamic(this, &ThisClass::OnTargetDataReady);
    Task->Cancelled.AddDynamic(this, &ThisClass::OnTargetDataCancelled);

    // 激活任务
    Task->ReadyForActivation();
}
```

TargetData生成：
```cpp
FGameplayAbilityTargetDataHandle UMyTargetGenerator::MakeTargetData(...)
{
    FGameplayAbilityTargetDataHandle DataHandle;

    // 执行射线检测
    TArray<FHitResult> HitResults;
    WeaponInstance->TraceBulletsInCartridge(HitResults);

    // 生成TargetData
    for (const FHitResult& Hit : HitResults)
    {
        FGameplayAbilityTargetData_SingleTargetHit* Data =
            new FGameplayAbilityTargetData_SingleTargetHit(Hit);

        DataHandle.Add(Data);
    }

    return DataHandle;
}
```

TargetData处理：
```cpp
void UShootGA_Weapon_Fire::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    if (HasAuthority())
    {
        // 服务器：验证并应用
        for (const FGameplayAbilityTargetData* TargetData : Data.Data)
        {
            // 验证命中合法性
            if (IsValidHit(TargetData))
            {
                // 应用伤害
                ApplyGameplayEffectToTarget(TargetData);
            }
        }
    }

    // 广播GameplayCue
    ExecuteGameplayCueFromTargetData(Data);

    // 结束Ability
    EndAbility(...);
}
```

## TargetData结构

常用类型：
```cpp
// 单目标命中
FGameplayAbilityTargetData_SingleTargetHit
- FHitResult HitResult
- bool bHitReplaced

// 位置目标
FGameplayAbilityTargetData_LocationInfo
- FGameplayAbilityTargetingLocationInfo SourceLocation
- FGameplayAbilityTargetingLocationInfo TargetLocation

// Actor目标
FGameplayAbilityTargetData_ActorArray
- TArray<TWeakObjectPtr<AActor>> TargetActorArray
```

自定义TargetData：
```cpp
USTRUCT()
struct FMyCustomTargetData : public FGameplayAbilityTargetData
{
    GENERATED_BODY()

    UPROPERTY()
    int32 UniqueId;

    UPROPERTY()
    float CustomDamage;

    // 实现必需方法
    virtual UScriptStruct* GetScriptStruct() const override
    {
        return FMyCustomTargetData::StaticStruct();
    }

    virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override
    {
        // 序列化逻辑
        return true;
    }
};
```

## 预测纠正

客户端预测：
```cpp
// 客户端本地应用效果（即时反馈）
if (IsLocallyControlled())
{
    // 播放开火动画
    PlayFireAnimation();

    // 扣除弹药（预测）
    PredictAmmoConsumption();

    // 显示准星反馈
    ShowHitMarker();
}
```

服务器纠正：
```cpp
// 服务器权威结果
if (HasAuthority())
{
    // 真实扣除弹药
    ConsumeAmmo();

    // 验证命中
    if (!IsValidHit(...))
    {
        // 回滚预测
        RevertPrediction();
    }
}
```

## 常见错误

错误：客户端直接应用伤害
```cpp
// 错误：绕过服务器验证
void OnTargetDataReady(...)
{
    ApplyDamage(Target);  // 客户端直接伤害
}
```

正确：
```cpp
void OnTargetDataReady(...)
{
    if (HasAuthority())  // 只在服务器
    {
        ApplyGameplayEffectToTarget(...);
    }
}
```

错误：不开启预测窗口
```cpp
// 错误：没有预测，体验延迟
void ActivateAbility(...)
{
    FHitResult Hit = DoRaycast();
    ServerApplyDamage(Hit);  // 等待服务器RTT
}
```

正确：
```cpp
void ActivateAbility(...)
{
    // 使用WaitTargetData开启预测
    UAbilityTask_WaitTargetData* Task = ...;
    Task->ReadyForActivation();
}
```

## 相关笔记

- [Weapon命中流程](./Weapon_命中流程.md)
- [Hitscan命中实现](../Weapons/Hitscan_命中实现.md)

## 来源

- DevelopmentLogs/2025-11/2025-11-02_GAS武器命中流程.md
- Lyra Ability System文档
- GAS官方文档
