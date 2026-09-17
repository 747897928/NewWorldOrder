---
note_id: WEAPON-002
title: Projectile投射物设计
category: Weapons
status: Active
created: 2025-11-06
updated: 2026-08-27
---

# Projectile投射物设计

## 核心要点

- 投射物武器通过生成Actor实现物理飞行和爆炸
- 只在服务器生成，客户端通过复制同步
- 支持爆炸伤害衰减、弱点倍率开关
- 使用UProjectileMovementComponent处理物理

## 当前实现覆盖旧草案（2026-08-27）

本文件后续保留的早期示例用于说明玩法意图，不再作为当前类名、字段名或伤害调用链的依据。当前项目已经统一到以下实现：

- 投射物配置位于 `UShootInventoryFragment_ProjectileWeaponConfig` 的 `FProjectileWeaponConfig`，由 `UShootRangedWeaponInstance` 通过 Fragment 读取；不存在独立的 `UProjectileWeaponDefinition` 或 Fragment `ProjectileDamageEffect` 字段。
- 伤害效果由 `B_WeaponInstance_*` 的 `DamageGameplayEffect` 提供。榴弹发射器和火箭筒分别使用自己的 Damage GE，服务器生成投射物并由投射物应用 GE Spec；不再使用投射物字段保存伤害 GE，也不使用 SetByCaller 手工写入最终伤害。
- `FShootGameplayEffectContext` 只承载爆炸原点、内外半径、衰减系数和是否启用材质倍率等上下文；`UShootDamageExecution` 负责 GE 内基础伤害、爆炸衰减以及配置允许时的材质/弱点倍率。
- `ExplosionCueTag` 由投射物配置驱动，服务器通过 GameplayCue 广播爆炸表现。火箭前后喷口、榴弹枪口音画闭包仍由各自武器 Montage/GameplayCue 资产承载，不能把手持手雷 GA 当作榴弹发射器实现。
- GA 传入的是“枪口 -> 准星目标点”的世界空间初速度，因此 `AShootProjectileBase` 必须设置 `UProjectileMovementComponent::bInitialVelocityInLocalSpace=false`。否则 Spawn Rotation 会被重复应用，非世界 +X 朝向会表现为侧飞或反飞。
- Grenade 对真实阻挡点使用 UE `UGameplayStatics::SuggestProjectileVelocity` 求固定初速的低/高弧；之后的重力、扫掠、弹跳和旋转全部由 `UProjectileMovementComponent` 处理，不启用 StaticMesh 物理模拟，也不维护逐帧自定义重力。
- `Bounciness`、`BounceFriction`、`BounceStopSpeed` 属于 `FProjectileWeaponConfig` 的蓝图配置，不硬编码在 Grenade 子类。Rocket/Grenade 的速度、重力、Fuse、半径和表现资产同样由 ID Fragment 配置。
- 当前实际资产和验收记录见 `Docs/Tasks/WeaponSystem/SpecialWeapons_GE迁移与Fragment字段审计.md`；Editor Target 冷编译、玩家手工验收和单进程 PIE 连续轨迹采样已通过。

## 武器类型

适用武器：
- 火箭筒：直射、无重力、无衰减
- 榴弹发射器：抛物线、有弹跳、衰减
- 弓箭：抛物线、有重力

## 类架构

Definition定义：
```cpp
UCLASS()
class UProjectileWeaponDefinition : public UWeaponDefinition
{
    GENERATED_BODY()

public:
    // 投射物类
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<AShootProjectileBase> ProjectileClass;

    // 投射物伤害效果
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UGameplayEffect> ProjectileDamageEffect;

    // 投射物配置
    UPROPERTY(EditDefaultsOnly)
    FProjectileWeaponConfig ProjectileConfig;
};
```

配置结构：
```cpp
USTRUCT()
struct FProjectileWeaponConfig
{
    GENERATED_BODY()

    // 速度
    UPROPERTY(EditDefaultsOnly)
    float InitialSpeed = 3000.0f;

    UPROPERTY(EditDefaultsOnly)
    float MaxSpeed = 3000.0f;

    UPROPERTY(EditDefaultsOnly)
    float GravityScale = 1.0f;

    // 爆炸参数
    UPROPERTY(EditDefaultsOnly)
    float DamageInnerRadius = 200.0f;

    UPROPERTY(EditDefaultsOnly)
    float DamageOuterRadius = 500.0f;

    UPROPERTY(EditDefaultsOnly)
    float DamageFalloff = 1.0f;  // 0=无衰减

    // 弱点倍率开关
    UPROPERTY(EditDefaultsOnly)
    bool bApplyMaterialMultipliers = false;  // 默认关闭

    // 引信
    UPROPERTY(EditDefaultsOnly)
    float FuseTime = 3.0f;  // 榴弹用
};
```

Projectile基类：
```cpp
UCLASS()
class AShootProjectileBase : public AActor
{
    GENERATED_BODY()

public:
    // 初始化
    void InitVelocity(const FVector& Direction, float Speed);

    // 爆炸
    void ApplyRadialDamage(const FVector& Origin);

protected:
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, FVector NormalImpulse,
        const FHitResult& Hit);

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovement;

    UPROPERTY(Replicated)
    FProjectileWeaponConfig Config;

    UPROPERTY(Replicated)
    TWeakObjectPtr<ARangedWeaponInstance> WeaponInstance;
};
```

## GA生成投射物

Ability实现：
```cpp
void UShootGA_Weapon_Fire_Projectile::ActivateAbility(...)
{
    if (!HasAuthority())
    {
        EndAbility(...);
        return;
    }

    // 获取配置
    UProjectileWeaponDefinition* ProjDef = GetProjectileDefinition();
    TSubclassOf<AShootProjectileBase> ProjectileClass = ProjDef->ProjectileClass;

    if (!ProjectileClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ProjectileClass is null"));
        EndAbility(...);
        return;
    }

    // 生成投射物
    FActorSpawnParameters SpawnParams;
    SpawnParams.Owner = GetAvatarActorFromActorInfo();
    SpawnParams.Instigator = Cast<APawn>(SpawnParams.Owner);

    AShootProjectileBase* Projectile = GetWorld()->SpawnActor<AShootProjectileBase>(
        ProjectileClass,
        MuzzleLocation,
        MuzzleRotation,
        SpawnParams
    );

    if (Projectile)
    {
        // 设置配置
        Projectile->SetConfig(ProjDef->ProjectileConfig);
        Projectile->SetWeaponInstance(WeaponInstance);

        // 初始化速度
        FVector Direction = MuzzleRotation.Vector();
        Projectile->InitVelocity(Direction, ProjDef->ProjectileConfig.InitialSpeed);
    }

    EndAbility(...);
}
```

## 投射物实现

初始化：
```cpp
void AShootProjectileBase::InitVelocity(const FVector& Direction, float Speed)
{
    if (ProjectileMovement)
    {
        ProjectileMovement->Velocity = Direction * Speed;
    }
}

void AShootProjectileBase::BeginPlay()
{
    Super::BeginPlay();

    // 配置ProjectileMovement
    if (ProjectileMovement)
    {
        ProjectileMovement->InitialSpeed = Config.InitialSpeed;
        ProjectileMovement->MaxSpeed = Config.MaxSpeed;
        ProjectileMovement->ProjectileGravityScale = Config.GravityScale;
    }

    // 绑定碰撞
    if (CollisionComp)
    {
        CollisionComp->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
    }

    // 引信定时器（榴弹）
    if (Config.FuseTime > 0.0f)
    {
        GetWorldTimerManager().SetTimer(
            FuseTimerHandle,
            this,
            &ThisClass::OnFuseExpired,
            Config.FuseTime,
            false
        );
    }
}
```

碰撞处理：
```cpp
void AShootProjectileBase::OnHit(...)
{
    if (!HasAuthority())
        return;

    // 应用爆炸伤害
    ApplyRadialDamage(Hit.ImpactPoint);

    // 广播GameplayCue
    FGameplayCueParameters CueParams;
    CueParams.Location = Hit.ImpactPoint;
    CueParams.Normal = Hit.ImpactNormal;

    UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(
        GetOwner(),
        FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Weapon.Explosion")),
        EGameplayCueEvent::Executed,
        CueParams
    );

    // 销毁
    Destroy();
}
```

## 爆炸伤害计算

ApplyRadialDamage：
```cpp
void AShootProjectileBase::ApplyRadialDamage(const FVector& Origin)
{
    if (!WeaponInstance.IsValid())
        return;

    UAbilitySystemComponent* ASC = WeaponInstance->GetAbilitySystemComponent();
    if (!ASC)
        return;

    // 范围查询
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(GetInstigator());

    GetWorld()->OverlapMultiByChannel(
        Overlaps,
        Origin,
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(Config.DamageOuterRadius),
        Params
    );

    // 对每个命中目标应用伤害
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* TargetActor = Overlap.GetActor();
        if (!TargetActor)
            continue;

        UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
        if (!TargetASC)
            continue;

        // 计算伤害
        float Distance = FVector::Dist(Origin, TargetActor->GetActorLocation());
        float Damage = CalculateExplosiveDamage(Distance);

        // 应用弱点倍率（如果开启）
        if (Config.bApplyMaterialMultipliers)
        {
            FHitResult Hit;
            // 射线检测获取物理材质
            if (LineTraceToTarget(Origin, TargetActor, Hit))
            {
                UPhysicalMaterial* PhysMat = Hit.PhysMaterial.Get();
                if (PhysMat)
                {
                    float Multiplier = GetPhysicalMaterialMultiplier(PhysMat);
                    Damage *= Multiplier;
                }
            }
        }

        // 创建GameplayEffectSpec
        FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
            ProjectileDamageEffect,
            1.0f,
            ASC->MakeEffectContext()
        );

        if (SpecHandle.IsValid())
        {
            // 设置伤害值
            SpecHandle.Data->SetSetByCallerMagnitude(
                FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")),
                Damage
            );

            // 应用
            ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
        }
    }
}
```

伤害衰减计算：
```cpp
float AShootProjectileBase::CalculateExplosiveDamage(float Distance) const
{
    float BaseDamage = WeaponInstance->GetBaseDamage();

    // 内圈满伤
    if (Distance <= Config.DamageInnerRadius)
    {
        return BaseDamage;
    }

    // 外圈范围外无伤害
    if (Distance >= Config.DamageOuterRadius)
    {
        return 0.0f;
    }

    // 衰减计算
    if (Config.DamageFalloff == 0.0f)
    {
        // 无衰减
        return BaseDamage;
    }

    float Ratio = (Distance - Config.DamageInnerRadius) /
                  (Config.DamageOuterRadius - Config.DamageInnerRadius);

    float Attenuation = FMath::Pow(1.0f - Ratio, Config.DamageFalloff);

    return BaseDamage * Attenuation;
}
```

## 派生类型

火箭：
```cpp
UCLASS()
class AShootProjectileRocket : public AShootProjectileBase
{
    GENERATED_BODY()

public:
    AShootProjectileRocket()
    {
        // 直射、无重力
        ProjectileMovement->ProjectileGravityScale = 0.0f;
    }
};
```

榴弹：
```cpp
UCLASS()
class AShootProjectileGrenade : public AShootProjectileBase
{
    GENERATED_BODY()

public:
    AShootProjectileGrenade()
    {
        // 允许弹跳
        ProjectileMovement->bShouldBounce = true;
        ProjectileMovement->Bounciness = 0.3f;
    }

protected:
    void OnFuseExpired()
    {
        // 引信超时爆炸
        ApplyRadialDamage(GetActorLocation());
        Destroy();
    }
};
```

## 弱点倍率开关

配置：
```cpp
// 火箭：不吃弱点
FProjectileWeaponConfig RocketConfig;
RocketConfig.bApplyMaterialMultipliers = false;
RocketConfig.DamageFalloff = 0.0f;  // 无衰减

// 榴弹：不吃弱点，有衰减
FProjectileWeaponConfig GrenadeConfig;
GrenadeConfig.bApplyMaterialMultipliers = false;
GrenadeConfig.DamageFalloff = 1.0f;  // 线性衰减

// 特殊爆炸武器：吃弱点
FProjectileWeaponConfig SpecialConfig;
SpecialConfig.bApplyMaterialMultipliers = true;
```

## 常见错误

错误：客户端生成投射物
```cpp
// 错误：没有HasAuthority检查
void ActivateAbility(...)
{
    SpawnActor<AShootProjectileBase>(...);  // 客户端也生成
}
```

正确：
```cpp
void ActivateAbility(...)
{
    if (HasAuthority())  // 只在服务器
    {
        SpawnActor<AShootProjectileBase>(...);
    }
}
```

错误：爆炸伤害在客户端计算
```cpp
void OnHit(...)
{
    ApplyRadialDamage(...);  // 客户端也计算
}
```

正确：
```cpp
void OnHit(...)
{
    if (HasAuthority())  // 只在服务器
    {
        ApplyRadialDamage(...);
    }
}
```

## 相关笔记

- [GAS Weapon命中流程](../GAS/Weapon_命中流程.md)
- [Hitscan命中实现](./Hitscan_命中实现.md)

## 来源

- DevelopmentLogs/2025-11/2025-11-02_武器实例分层改造.md
- Lyra投射物系统
