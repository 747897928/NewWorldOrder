---
note_id: WEAPON-001
title: Hitscan命中实现
category: Weapons
status: Active
created: 2025-11-06
updated: 2025-11-06
---

# Hitscan命中实现

## 核心要点

- Hitscan武器使用射线检测实现即时命中
- 支持散布、扩散恢复、多弹丸
- 使用AHitscanWeaponInstance + UHitscanWeaponDefinition
- 遵循GAS的TargetData预测流程

## 武器类型

适用武器：
- 步枪：中等散布、扩散恢复快
- 狙击枪：极低散布、扩散恢复慢
- 霰弹枪：多弹丸、大散布、无扩散恢复
- 手枪：中等散布、中等扩散恢复

## 类架构

Definition定义：
```cpp
UCLASS()
class UHitscanWeaponDefinition : public UWeaponDefinition
{
    GENERATED_BODY()

public:
    // 散布参数
    UPROPERTY(EditDefaultsOnly)
    float SpreadAngle = 1.0f;  // 基础散布角度

    UPROPERTY(EditDefaultsOnly)
    float SpreadAngleMultiplier_Hip = 1.0f;  // 腰射倍率

    UPROPERTY(EditDefaultsOnly)
    float SpreadAngleMultiplier_Ads = 0.5f;  // 瞄准倍率

    // 扩散恢复
    UPROPERTY(EditDefaultsOnly)
    float HeatToSpreadCurve;  // 热度->散布曲线

    UPROPERTY(EditDefaultsOnly)
    float HeatToCoolDownRate;  // 冷却速率

    // 多弹丸（霰弹枪）
    UPROPERTY(EditDefaultsOnly)
    int32 BulletsPerCartridge = 1;  // 每发子弹数量
};
```

Instance实现：
```cpp
UCLASS()
class AHitscanWeaponInstance : public ARangedWeaponInstance
{
    GENERATED_BODY()

public:
    // 射线检测
    void TraceBulletsInCartridge(
        TArray<FHitResult>& OutHits,
        const FVector& StartTrace,
        const FVector& EndTrace
    );

    // 散布计算
    FVector CalculateSpreadDirection(
        const FVector& AimDirection
    ) const;

    // 扩散更新
    void UpdateHeat(float DeltaTime);

private:
    float CurrentHeat = 0.0f;
    float CurrentSpreadAngle = 0.0f;
};
```

### FireSequenceId（2025-11-08 新增）
- `AHitscanWeaponInstance` 维护 `int32 FireSequenceId` 并在 `UpdateFiringTime()` 内自增，已复制给客户端。
- 若后续需要恢复 Lyra 的 `UniqueId` 语义，只需在 TargetData 里附带 `GetFireSequenceId()`，服务器即可比对回放顺序，无需改动 Ability 主体。

## 射线检测实现

TraceBulletsInCartridge：
```cpp
void AHitscanWeaponInstance::TraceBulletsInCartridge(
    TArray<FHitResult>& OutHits,
    const FVector& StartTrace,
    const FVector& AimDirection
)
{
    UHitscanWeaponDefinition* HitscanDef = GetHitscanDefinition();
    int32 BulletsToTrace = HitscanDef->BulletsPerCartridge;

    for (int32 i = 0; i < BulletsToTrace; ++i)
    {
        // 计算散布方向
        FVector SpreadDirection = CalculateSpreadDirection(AimDirection);

        // 计算终点
        FVector EndTrace = StartTrace + (SpreadDirection * MaxRange);

        // 射线检测
        FHitResult Hit;
        FCollisionQueryParams Params;
        Params.AddIgnoredActor(GetOwner());

        GetWorld()->LineTraceSingleByChannel(
            Hit,
            StartTrace,
            EndTrace,
            ECC_GameTraceChannel1,  // Weapon trace channel
            Params
        );

        OutHits.Add(Hit);
    }

    // 增加热度（扩散）
    CurrentHeat += HitscanDef->HeatPerShot;
}
```

散布计算：
```cpp
FVector AHitscanWeaponInstance::CalculateSpreadDirection(const FVector& AimDirection) const
{
    UHitscanWeaponDefinition* HitscanDef = GetHitscanDefinition();

    // 获取当前散布角度
    float SpreadAngle = CurrentSpreadAngle;

    // 应用瞄准状态倍率
    if (bIsAiming)
    {
        SpreadAngle *= HitscanDef->SpreadAngleMultiplier_Ads;
    }
    else
    {
        SpreadAngle *= HitscanDef->SpreadAngleMultiplier_Hip;
    }

    // 生成随机偏移
    FVector2D RandomCircle = FMath::RandPointInCircle(1.0f);
    float AngleRad = FMath::DegreesToRadians(SpreadAngle);

    // 计算偏移方向
    FVector Right = FVector::CrossProduct(AimDirection, FVector::UpVector).GetSafeNormal();
    FVector Up = FVector::CrossProduct(Right, AimDirection).GetSafeNormal();

    FVector Offset = (Right * RandomCircle.X + Up * RandomCircle.Y) * FMath::Tan(AngleRad);

    return (AimDirection + Offset).GetSafeNormal();
}
```

## 扩散系统

热度更新：
```cpp
void AHitscanWeaponInstance::UpdateHeat(float DeltaTime)
{
    UHitscanWeaponDefinition* HitscanDef = GetHitscanDefinition();

    // 自然冷却
    CurrentHeat = FMath::Max(0.0f, CurrentHeat - HitscanDef->HeatCoolDownRate * DeltaTime);

    // 热度->散布
    CurrentSpreadAngle = HitscanDef->SpreadAngle +
        (CurrentHeat * HitscanDef->HeatToSpreadCurve);
}
```

Tick更新：
```cpp
void AHitscanWeaponInstance::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // 更新扩散
    UpdateHeat(DeltaTime);
}
```

## 多弹丸（霰弹枪）

配置：
```cpp
// 霰弹枪Definition
BulletsPerCartridge = 8;  // 每发8颗弹丸
SpreadAngle = 5.0f;  // 大散布
HeatPerShot = 0.0f;  // 无扩散
```

伤害处理：
```cpp
// 每个弹丸独立计算伤害
for (const FHitResult& Hit : Hits)
{
    if (Hit.bBlockingHit)
    {
        // 每个弹丸应用伤害
        ApplyDamageToHit(Hit, DamagePerBullet);
    }
}
```

## 准星系统集成

散布角度查询：
```cpp
float AHitscanWeaponInstance::GetCurrentSpreadAngle() const
{
    float Angle = CurrentSpreadAngle;

    if (bIsAiming)
    {
        Angle *= GetHitscanDefinition()->SpreadAngleMultiplier_Ads;
    }
    else
    {
        Angle *= GetHitscanDefinition()->SpreadAngleMultiplier_Hip;
    }

    return Angle;
}
```

准星UI使用：
```cpp
// UShootReticleWidgetBase
void UShootReticleWidgetBase::UpdateReticle()
{
    if (AHitscanWeaponInstance* HitscanWeapon = Cast<AHitscanWeaponInstance>(CurrentWeapon))
    {
        float SpreadAngle = HitscanWeapon->GetCurrentSpreadAngle();
        float ScreenRadius = ConvertAngleToScreenRadius(SpreadAngle);

        // 更新准星大小
        UpdateReticleSize(ScreenRadius);
    }
}
```

## 常见错误

错误：不考虑扩散
```cpp
// 错误：固定散布
FVector SpreadDirection = AimDirection + RandomOffset;
```

正确：
```cpp
// 正确：基于热度动态散布
float SpreadAngle = CurrentSpreadAngle;  // 随热度增加
FVector SpreadDirection = CalculateSpreadDirection(AimDirection);
```

错误：多弹丸共享一次射线
```cpp
// 错误：霰弹枪只检测一次
FHitResult Hit;
LineTrace(Hit);
ApplyDamage(Hit, TotalDamage);
```

正确：
```cpp
// 正确：每个弹丸独立检测
for (int32 i = 0; i < BulletsPerCartridge; ++i)
{
    FHitResult Hit;
    LineTrace(Hit);
    ApplyDamage(Hit, DamagePerBullet);
}
```

## 相关笔记

- [GAS Weapon命中流程](../GAS/Weapon_命中流程.md)
- [TargetData预测流程](../GAS/TargetData_预测流程.md)
- [Projectile投射物设计](./Projectile_投射物设计.md)

## 来源

- DevelopmentLogs/2025-11/2025-11-02_武器实例分层改造.md
- Lyra武器系统
