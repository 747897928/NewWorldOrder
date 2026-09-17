---
note_id: GAS-001
title: Weapon命中流程
category: GAS
status: Active
created: 2025-11-06
updated: 2025-11-06
---

# Weapon命中流程

## 核心要点

- 命中型武器（Hitscan）和投射物武器（Projectile）使用不同的GAS流程
- 命中型：客户端预测 + TargetData + 服务器验证
- 投射物型：服务器生成Actor + 物理模拟 + 命中回调
- 两种流程各有适用场景，不需要强制统一

## 命中型武器流程（Hitscan）

适用武器：
- 步枪
- 狙击枪
- 霰弹枪
- 手枪

技术栈：
- UShootGameplayAbility_Weapon_Fire
- AHitscanWeaponInstance
- FGameplayAbilityTargetData

客户端流程：
```cpp
1. Ability激活
2. StartRangedWeaponTargeting（开启预测窗口）
3. AHitscanWeaponInstance::TraceBulletsInCartridge（本地射线检测）
4. 生成TargetData（包含命中点、伤害等）
5. ServerSetReplicatedTargetData（发送到服务器）
```

服务器流程：
```cpp
1. 接收TargetData
2. 验证命中合法性
3. ApplyGameplayEffectSpecToTarget（应用伤害）
4. ExecuteGameplayCue（广播表现效果）
```

代码示例：
```cpp
// Ability中
void UShootGA_Weapon_Fire::ActivateAbility(...)
{
    // 开启预测
    UAbilityTask_WaitTargetData* Task = UAbilityTask_WaitTargetData::WaitTargetData(...);
    Task->ValidData.AddDynamic(this, &ThisClass::OnTargetDataReady);
    Task->ReadyForActivation();
}

void UShootGA_Weapon_Fire::OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data)
{
    // 服务器验证并应用伤害
    ApplyGameplayEffectToTarget(...);
}
```

## 投射物武器流程（Projectile）

适用武器：
- 榴弹发射器
- 火箭筒
- 弓箭

技术栈：
- UShootGA_Weapon_Fire_Projectile
- AShootProjectileBase
- UProjectileMovementComponent

服务器流程：
```cpp
1. Ability在服务器生成AShootProjectileBase
2. 设置UProjectileMovementComponent参数（初速度、重力等）
3. 投射物飞行（物理模拟）
4. OnHit回调触发
5. ApplyRadialDamage（范围伤害 + GameplayEffect）
6. ExecuteGameplayCue（广播爆炸表现）
```

客户端流程：
```cpp
1. 接收投射物Actor复制
2. 播放飞行轨迹表现
3. 接收GameplayCue
4. 播放爆炸表现
```

代码示例：
```cpp
// Ability中
void UShootGA_Weapon_Fire_Projectile::ActivateAbility(...)
{
    if (HasAuthority())
    {
        // 只在服务器生成
        AShootProjectileBase* Projectile = GetWorld()->SpawnActor<AShootProjectileBase>(...);
        Projectile->InitVelocity(Direction, Speed);
    }
}

// 投射物中
void AShootProjectileBase::OnHit(...)
{
    if (HasAuthority())
    {
        ApplyRadialDamage(HitLocation);
        ExecuteGameplayCue(...);
        Destroy();
    }
}
```

## TargetData.UniqueId

当前实现：
- UniqueId固定为0
- 功能正常，但无法支持命中重播

未来扩展（可选）：
```cpp
// AHitscanWeaponInstance.h
private:
    int32 FireSequenceId = 0;

// AHitscanWeaponInstance.cpp
void AHitscanWeaponInstance::TraceBulletsInCartridge(...)
{
    TargetData->UniqueId = ++FireSequenceId;
    // ... 其他逻辑
}
```

用途：
- 服务器验证命中顺序
- 命中重播
- 反作弊检测

## 架构边界

命中型武器必须：
- 使用AHitscanWeaponInstance
- 发送TargetData到服务器
- 遵循预测流程

投射物武器必须：
- 使用AShootProjectileBase
- 在服务器生成Actor
- 通过物理模拟同步

不要混淆：
- 不要给投射物武器发送TargetData（除非需要导引）
- 不要让命中型武器生成投射物Actor

## 常见错误

错误：投射物在客户端生成
```cpp
void UShootGA_Weapon_Fire_Projectile::ActivateAbility(...)
{
    // 错误：客户端也生成
    AShootProjectileBase* Projectile = GetWorld()->SpawnActor<AShootProjectileBase>(...);
}
```

正确：
```cpp
void UShootGA_Weapon_Fire_Projectile::ActivateAbility(...)
{
    if (HasAuthority())  // 只在服务器
    {
        AShootProjectileBase* Projectile = GetWorld()->SpawnActor<AShootProjectileBase>(...);
    }
}
```

错误：命中型武器不发送TargetData
```cpp
// 错误：本地计算伤害
ApplyDamageLocally(HitResult);
```

正确：
```cpp
// 生成TargetData发送到服务器
FGameplayAbilityTargetDataHandle DataHandle = MakeTargetData(HitResult);
SetReplicatedTargetData(DataHandle);
```

## 相关笔记

- [TargetData预测流程](./TargetData_预测流程.md)
- [Hitscan命中实现](../Weapons/Hitscan_命中实现.md)
- [Projectile投射物设计](../Weapons/Projectile_投射物设计.md)

## 来源

- DevelopmentLogs/2025-11/2025-11-02_GAS武器命中流程.md
- Lyra射击游戏示例
