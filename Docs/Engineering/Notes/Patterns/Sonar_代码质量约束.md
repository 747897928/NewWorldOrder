---
note_id: PATTERN-001
title: Sonar代码质量约束
category: Patterns
status: Active
created: 2025-11-06
updated: 2025-11-06
---

# Sonar代码质量约束

## 核心要点

- Sonar硬约束：函数行数<=80、嵌套<=3、认知复杂度<=15
- 遵循单一职责原则
- 使用提前返回减少嵌套
- 提取辅助函数保持主流程清晰

## 硬约束规则

函数行数：
- 限制：<=80行
- 理由：超过80行的函数难以理解和维护
- 违反：Sonar报Major issue

嵌套层级：
- 限制：<=3层
- 理由：深层嵌套增加认知负担
- 违反：Sonar报Major issue

认知复杂度：
- 限制：<=15
- 理由：复杂逻辑难以测试和调试
- 违反：Sonar报Critical issue

## 重构技巧

技巧1：提前返回
```cpp
// 错误：深层嵌套
void ProcessData(Data* data)
{
    if (data)
    {
        if (data->IsValid())
        {
            if (data->HasPermission())
            {
                // 实际逻辑
            }
        }
    }
}

// 正确：提前返回
void ProcessData(Data* data)
{
    if (!data) return;
    if (!data->IsValid()) return;
    if (!data->HasPermission()) return;

    // 实际逻辑
}
```

技巧2：提取辅助函数
```cpp
// 错误：100行的复杂函数
void ApplyRadialDamage(const FVector& Origin)
{
    // 范围查询（10行）
    // 距离计算（20行）
    // 材质查询（15行）
    // 伤害计算（25行）
    // GameplayEffect应用（30行）
}

// 正确：拆分为多个函数
void ApplyRadialDamage(const FVector& Origin)  // 主流程54行
{
    TArray<AActor*> Targets = FindTargetsInRadius(Origin);  // 提取

    for (AActor* Target : Targets)
    {
        float Distance = CalculateDistance(Origin, Target);
        float Damage = CalculateExplosiveDamage(Distance);  // 提取

        UPhysicalMaterial* PhysMat = ResolvePhysicalMaterial(Target);  // 提取
        if (PhysMat)
        {
            Damage *= GetMaterialMultiplier(PhysMat);
        }

        ApplyDamageToTarget(Target, Damage);  // 提取
    }
}

float CalculateExplosiveDamage(float Distance) const  // 辅助函数
{
    if (Distance <= InnerRadius) return BaseDamage;
    if (Distance >= OuterRadius) return 0.0f;

    float Ratio = (Distance - InnerRadius) / (OuterRadius - InnerRadius);
    return BaseDamage * FMath::Pow(1.0f - Ratio, Falloff);
}
```

技巧3：使用卫语句
```cpp
// 错误：深层if-else
void Execute()
{
    if (Condition1)
    {
        if (Condition2)
        {
            DoSomething();
        }
        else
        {
            DoOtherThing();
        }
    }
    else
    {
        DoDefaultThing();
    }
}

// 正确：卫语句
void Execute()
{
    if (!Condition1)
    {
        DoDefaultThing();
        return;
    }

    if (!Condition2)
    {
        DoOtherThing();
        return;
    }

    DoSomething();
}
```

技巧4：封装条件判断
```cpp
// 错误：复杂条件
if (bIsAlive && Health > 0 && !bIsStunned && HasAmmo())
{
    Fire();
}

// 正确：封装为函数
if (CanFire())
{
    Fire();
}

bool CanFire() const
{
    return bIsAlive &&
           Health > 0 &&
           !bIsStunned &&
           HasAmmo();
}
```

## 函数编写准则

原则1：单一职责
- 一个函数只做一件事
- 函数名应准确描述功能
- 如果函数名需要用"和"连接，说明职责不单一

原则2：逻辑主线清晰
```cpp
void MainFunction()
{
    // 第一步：验证参数
    if (!ValidateInput()) return;

    // 第二步：准备数据
    Data data = PrepareData();

    // 第三步：执行操作
    Result result = ExecuteOperation(data);

    // 第四步：处理结果
    ProcessResult(result);
}
```

原则3：避免重复代码
```cpp
// 错误：重复逻辑
void ProcessPlayerDamage(...)
{
    float Damage = BaseDamage * DamageMultiplier;
    if (bIsCritical) Damage *= CritMultiplier;
    ApplyDamage(Player, Damage);
}

void ProcessEnemyDamage(...)
{
    float Damage = BaseDamage * DamageMultiplier;
    if (bIsCritical) Damage *= CritMultiplier;
    ApplyDamage(Enemy, Damage);
}

// 正确：提取公共逻辑
void ProcessDamage(AActor* Target, ...)
{
    float Damage = CalculateFinalDamage(BaseDamage, bIsCritical);
    ApplyDamage(Target, Damage);
}

float CalculateFinalDamage(float Base, bool bCrit) const
{
    float Damage = Base * DamageMultiplier;
    if (bCrit) Damage *= CritMultiplier;
    return Damage;
}
```

## 检测工具

临时脚本：
```python
# temp_sonar.py
import os
import re

def count_function_lines(file_path):
    with open(file_path, 'r') as f:
        content = f.read()

    # 查找函数定义
    pattern = r'(\w+::\w+\([^)]*\)[^{]*\{)'
    matches = re.finditer(pattern, content)

    for match in matches:
        # 计算函数行数
        start = match.end()
        brace_count = 1
        lines = 1

        for i, char in enumerate(content[start:]):
            if char == '{': brace_count += 1
            if char == '}': brace_count -= 1
            if char == '\n': lines += 1

            if brace_count == 0:
                if lines > 80:
                    print(f"{file_path}: {match.group(1)} - {lines} lines")
                break

# 扫描目录
for root, dirs, files in os.walk("Source/NewWorldOrder"):
    for file in files:
        if file.endswith(".cpp"):
            count_function_lines(os.path.join(root, file))
```

使用方式：
```bash
python temp_sonar.py
# 查看输出
rm temp_sonar.py  # 删除临时脚本
```

## 实践案例

案例：AShootProjectileBase::ApplyRadialDamage

重构前：
- 100+行
- 嵌套4层
- 认知复杂度20+

重构后：
```cpp
// 主函数54行
void AShootProjectileBase::ApplyRadialDamage(const FVector& Origin)
{
    if (!WeaponInstance.IsValid()) return;

    UAbilitySystemComponent* ASC = GetASC();
    if (!ASC) return;

    TArray<AActor*> Targets = FindTargetsInRadius(Origin);

    for (AActor* Target : Targets)
    {
        float Damage = CalculateDamageForTarget(Origin, Target);
        ApplyDamageToTarget(ASC, Target, Damage);
    }
}

// 辅助函数
float AShootProjectileBase::CalculateDamageForTarget(const FVector& Origin, AActor* Target) const
{
    float Distance = FVector::Dist(Origin, Target->GetActorLocation());
    float Damage = CalculateExplosiveDamage(Distance);

    if (Config.bApplyMaterialMultipliers)
    {
        UPhysicalMaterial* PhysMat = ResolvePhysicalMaterial(Origin, Target);
        if (PhysMat)
        {
            Damage *= GetPhysicalMaterialMultiplier(PhysMat);
        }
    }

    return Damage;
}
```

结果：
- 主函数54行（符合<=80）
- 嵌套2层（符合<=3）
- 认知复杂度10（符合<=15）

## 相关笔记

- [SSOT单一数据源](./SSOT_单一数据源.md)

## 来源

- DevelopmentLogs/2025-11/2025-11-02_Sonar约束实践.md
- Sonar官方规则文档
