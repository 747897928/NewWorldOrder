# 属性公式 Attribute Formulas

文档版本: 1.0
最后更新: 2025-11-01
单一数据源标记: SINGLE SOURCE OF TRUTH

警告：本文档是所有属性计算公式的唯一权威来源
修改此文档前必须通知策划团队并更新版本号
任何其他文档中的公式描述仅供参考，以本文档为准

---

## 体力 Vitality

### 最大生命值 MaxHP

公式（三段递减）：

```
MaxHP = BaseHP + VitalityBonus
BaseHP = 100

VitalityBonus计算：
- 0-10点体力：每点 +10 HP
  VitalityBonus = Vitality × 10

- 11-20点体力：每点 +6 HP
  VitalityBonus = 100 + (Vitality - 10) × 6

- 21-30点体力：每点 +4 HP
  VitalityBonus = 160 + (Vitality - 20) × 4
```

关键数值点：
- 1点体力：100 + 10 = 110 HP
- 10点体力：100 + 100 = 200 HP
- 20点体力：100 + 160 = 260 HP
- 30点体力：100 + 200 = 300 HP

实现伪代码：
```cpp
float CalculateMaxHP(int32 Vitality)
{
    float BaseHP = 100.0f;
    float VitalityBonus = 0.0f;

    if (Vitality <= 10)
    {
        VitalityBonus = Vitality * 10.0f;
    }
    else if (Vitality <= 20)
    {
        VitalityBonus = 100.0f + (Vitality - 10) * 6.0f;
    }
    else // Vitality <= 30
    {
        VitalityBonus = 160.0f + (Vitality - 20) * 4.0f;
    }

    return BaseHP + VitalityBonus;
}
```

---

### 护盾容量 Shield Capacity

公式：

```
ShieldCapacity = (BaseShieldPercent + VitalityShieldBonus) × MaxHP

BaseShieldPercent = 25%
VitalityShieldBonus = Vitality × 1.5%
```

关键数值点：
- 0点体力：25% MaxHP
- 10点体力：(25% + 15%) × MaxHP = 40% MaxHP = 80护盾（200HP角色）
- 20点体力：(25% + 30%) × MaxHP = 55% MaxHP = 143护盾（260HP角色）
- 30点体力：(25% + 45%) × MaxHP = 70% MaxHP = 210护盾（300HP角色）

实现伪代码：
```cpp
float CalculateShieldCapacity(int32 Vitality, float MaxHP)
{
    float BaseShieldPercent = 0.25f; // 25%
    float VitalityShieldBonus = Vitality * 0.015f; // 每点1.5%
    float TotalShieldPercent = BaseShieldPercent + VitalityShieldBonus;

    return TotalShieldPercent * MaxHP;
}
```

---

### 击杀回血 Kill Healing

公式：

```
HealOnKill = (Vitality / 10) × 3% × MaxHP

向下取整，每10点体力提供一档
```

关键数值点：
- 0-9点体力：0% MaxHP
- 10-19点体力：3% MaxHP
- 20-29点体力：6% MaxHP
- 30点体力：9% MaxHP

示例计算：
- 260HP角色，20点体力：击杀回复 6% × 260 = 15.6 HP

实现伪代码：
```cpp
float CalculateKillHealing(int32 Vitality, float MaxHP)
{
    int32 Tiers = Vitality / 10; // 整数除法，向下取整
    float HealPercent = Tiers * 0.03f; // 每档3%

    return HealPercent * MaxHP;
}
```

重要限制：
- 仅对特殊/精英/Boss敌人生效（防止刷小怪）
- 与标记猎手被动独立计算，可叠加
- 参考：../Skills/PassiveSkills_被动技能.md#标记猎手

---

## 力量 Strength

### 武器伤害加成 Weapon Damage Bonus

公式（三段递减）：

```
DamageBonus计算：
- 0-10点力量：每点 +2%
  DamageBonus = Strength × 2%

- 11-20点力量：每点 +1.5%
  DamageBonus = 20% + (Strength - 10) × 1.5%

- 21-30点力量：每点 +1%
  DamageBonus = 35% + (Strength - 20) × 1%

最终伤害 = BaseDamage × (1 + DamageBonus)
```

关键数值点：
- 10点力量：+20% 伤害
- 20点力量：+35% 伤害
- 30点力量：+45% 伤害

示例计算：
- AK47基础35伤害，30点力量：35 × (1 + 0.45) = 50.75 伤害

实现伪代码：
```cpp
float CalculateWeaponDamageMultiplier(int32 Strength)
{
    float DamageBonus = 0.0f;

    if (Strength <= 10)
    {
        DamageBonus = Strength * 0.02f; // 每点2%
    }
    else if (Strength <= 20)
    {
        DamageBonus = 0.20f + (Strength - 10) * 0.015f; // 每点1.5%
    }
    else // Strength <= 30
    {
        DamageBonus = 0.35f + (Strength - 20) * 0.01f; // 每点1%
    }

    return 1.0f + DamageBonus;
}
```

---

### 护甲穿透 Armor Penetration

公式：

```
ArmorPenetration = Strength × 1%

有效护甲 = ArmorValue × (1 - ArmorPenetration)
```

关键数值点：
- 10点力量：10% 穿透
- 20点力量：20% 穿透
- 30点力量：30% 穿透

示例计算：
- 敌人护甲50点，30点力量穿透30%：
  有效护甲 = 50 × (1 - 0.30) = 35点

实现伪代码：
```cpp
float CalculateEffectiveArmor(int32 Strength, float EnemyArmor)
{
    float PenetrationPercent = Strength * 0.01f; // 每点1%
    return EnemyArmor * (1.0f - PenetrationPercent);
}
```

---

### 精英伤害加成 Elite Damage Bonus

公式：

```
EliteDamageBonus = (Strength / 10) × 5%

向下取整，每10点力量提供一档
最终伤害 = BaseDamage × (1 + EliteDamageBonus)
```

关键数值点：
- 0-9点力量：+0%
- 10-19点力量：+5%
- 20-29点力量：+10%
- 30点力量：+15%

示例计算：
- AK47基础35伤害，30点力量，对精英：
  35 × 1.45（武器伤害） × 1.15（精英加成） = 58.3875 伤害

实现伪代码：
```cpp
float CalculateEliteDamageMultiplier(int32 Strength)
{
    int32 Tiers = Strength / 10; // 整数除法
    float EliteBonus = Tiers * 0.05f; // 每档5%

    return 1.0f + EliteBonus;
}
```

触发条件：
- 仅对特殊/精英/Boss敌人生效
- 与武器伤害加成叠加（乘法）
- 敌人分级规则：见 ../Weapons/BalanceValidation_平衡验证.md#敌人分级

---

## 敏捷 Agility

### 移动速度 Movement Speed

公式：

```
MoveSpeedBonus = Agility × 0.8%

最终移速 = BaseMoveSpeed × (1 + MoveSpeedBonus)
BaseMoveSpeed = 100%（引擎默认）
```

关键数值点：
- 10点敏捷：+8% 移速
- 20点敏捷：+16% 移速
- 30点敏捷：+24% 移速

实现伪代码：
```cpp
float CalculateMoveSpeedMultiplier(int32 Agility)
{
    float SpeedBonus = Agility * 0.008f; // 每点0.8%
    return 1.0f + SpeedBonus;
}
```

---

### 换弹速度 Reload Speed

公式：

```
ReloadTimeReduction = Agility × 1.5%

最终换弹时间 = BaseReloadTime × (1 - ReloadTimeReduction)
```

关键数值点：
- 10点敏捷：换弹时间 -15%
- 20点敏捷：换弹时间 -30%
- 30点敏捷：换弹时间 -45%

示例计算：
- AK47基础换弹2.2秒，30点敏捷：
  2.2 × (1 - 0.45) = 1.21秒

实现伪代码：
```cpp
float CalculateReloadTime(int32 Agility, float BaseReloadTime)
{
    float ReloadReduction = Agility * 0.015f; // 每点1.5%
    return BaseReloadTime * (1.0f - ReloadReduction);
}
```

叠加规则：
- 与速射手被动叠加（乘法）
- 速射手Lv3提供额外-40%
- 参考：../Skills/PassiveSkills_被动技能.md#速射手

---

### 翻滚充能时间 Dodge Cooldown

公式：

```
DodgeCooldown = BaseCooldown - (Agility × 0.03秒)

BaseCooldown = 12秒
```

关键数值点：
- 0点敏捷：12.0秒/充能
- 10点敏捷：11.7秒/充能
- 20点敏捷：11.4秒/充能
- 30点敏捷：11.1秒/充能

实现伪代码：
```cpp
float CalculateDodgeCooldown(int32 Agility)
{
    float BaseCooldown = 12.0f;
    float CooldownReduction = Agility * 0.03f;

    return BaseCooldown - CooldownReduction;
}
```

重要限制：
- 翻滚充能不受战术专家CDR影响
- 始终保持3层充能
- 第3次翻滚触发递减惩罚（10秒内使用）
- 参考：../Skills/MaleSkills_男主技能.md#战术翻滚

---

### 翻滚伤害加成 Dodge Damage Bonus

公式：

```
DodgeDamageBonus = (Agility / 10) × 15%

向下取整，每10点敏捷提供一档
持续时间：翻滚后2秒
```

关键数值点：
- 0-9点敏捷：+0%
- 10-19点敏捷：+15%
- 20-29点敏捷：+30%
- 30点敏捷：+45%

实现伪代码：
```cpp
float CalculateDodgeDamageBonus(int32 Agility)
{
    int32 Tiers = Agility / 10;
    float DamageBonus = Tiers * 0.15f; // 每档15%

    return DamageBonus;
}
```

实现方式：
- 翻滚时施加临时GameplayEffect
- 持续2秒后自动移除
- 可多次触发，独立计时

---

## 感知 Perception

### 弱点伤害倍率 Critical Multiplier

公式：

```
CriticalMultiplier = BaseCritMultiplier + (Perception × 1%)

BaseCritMultiplier = 2.0（200%）
```

关键数值点：
- 0点感知：2.0倍（200%）
- 10点感知：2.1倍（210%）
- 20点感知：2.2倍（220%）
- 30点感知：2.3倍（230%）

示例计算：
- AK47基础35伤害，30点力量50.75伤害，30点感知：
  弱点伤害 = 50.75 × 2.3 = 116.725

实现伪代码：
```cpp
float CalculateCriticalMultiplier(int32 Perception)
{
    float BaseCritMultiplier = 2.0f;
    float PerceptionBonus = Perception * 0.01f; // 每点1%

    return BaseCritMultiplier + PerceptionBonus;
}
```

注意事项：
- 弱点倍率无上限
- 某些技能可进一步提升（如破甲射击+30%）
- 参考：../Skills/MaleSkills_男主技能.md#破甲射击

---

### 弹药掉落概率 Ammo Drop Rate

公式：

```
DropRateBonus = Perception × 1%

最终掉落率 = BaseDropRate + DropRateBonus
```

基础掉落率（按敌人类型）：
- 普通敌人：10%
- 特殊敌人：30%
- 精英敌人：60%

关键数值点：
- 30点感知，击杀普通敌人：10% + 30% = 40% 掉落率
- 30点感知，击杀特殊敌人：30% + 30% = 60% 掉落率
- 30点感知，击杀精英敌人：60% + 30% = 90% 掉落率

实现伪代码：
```cpp
float CalculateDropRate(int32 Perception, float BaseDropRate)
{
    float PerceptionBonus = Perception * 0.01f; // 每点1%
    return FMath::Clamp(BaseDropRate + PerceptionBonus, 0.0f, 1.0f);
}
```

参考：
- 弹药配置表：../Weapons/CSVStructure_CSV结构.md#AmmoConfig

---

### 侦测范围 Detection Range

公式：

```
DetectionRangeBonus = Perception × 1%

最终侦测范围 = BaseRange × (1 + DetectionRangeBonus)
```

关键数值点：
- 10点感知：+10% 侦测范围
- 20点感知：+20% 侦测范围
- 30点感知：+30% 侦测范围

实现伪代码：
```cpp
float CalculateDetectionRange(int32 Perception, float BaseRange)
{
    float RangeBonus = Perception * 0.01f;
    return BaseRange * (1.0f + RangeBonus);
}
```

---

### 弱点连击 Critical Streak

公式：

```
StreakDamageBonus = (Perception / 10) × 30%

向下取整，每10点感知提供一档
触发条件：连续2次弱点命中
效果：下3发弱点伤害额外加成
```

关键数值点：
- 0-9点感知：+0%
- 10-19点感知：+30%
- 20-29点感知：+60%
- 30点感知：+90%

示例计算：
- AK47基础50.75伤害，30点感知：
  基础弱点：50.75 × 2.3 = 116.725
  触发连击：116.725 × (1 + 0.90) = 221.7775

实现伪代码：
```cpp
float CalculateStreakBonus(int32 Perception)
{
    int32 Tiers = Perception / 10;
    float StreakBonus = Tiers * 0.30f; // 每档30%

    return StreakBonus;
}
```

实现方式：
- 记录连续弱点命中次数
- 达到2次时施加临时GameplayEffect
- 下3发弱点命中额外伤害，之后重置计数

---

## 公式验证清单

实现属性公式时必须验证以下测试用例：

体力测试：
- [ ] 1点体力 = 110 HP
- [ ] 10点体力 = 200 HP
- [ ] 20点体力 = 260 HP
- [ ] 30点体力 = 300 HP
- [ ] 30点体力护盾 = 210点（70% × 300）

力量测试：
- [ ] 10点力量 = +20% 武器伤害
- [ ] 20点力量 = +35% 武器伤害
- [ ] 30点力量 = +45% 武器伤害
- [ ] 30点力量穿透 = 30%

敏捷测试：
- [ ] 30点敏捷换弹AK47 = 1.21秒（2.2 × 0.55）
- [ ] 30点敏捷移速 = 124%（100% + 24%）
- [ ] 30点敏捷翻滚CD = 11.1秒

感知测试：
- [ ] 30点感知弱点倍率 = 2.3
- [ ] 30点感知+普通敌人掉落率 = 40%
- [ ] 30点感知弱点连击 = +90%

---

参考文档：
- Overview_总览.md - 属性系统总览
- GrowthCurves_成长曲线.md - 成长曲线分析
- ../Skills/PassiveSkills_被动技能.md - 被动技能叠加规则
- ../../claude/新秩序技能系统核心数值-精简版.md - 原始设计（归档）

最后更新: 2025-11-01
维护者: 项目团队
公式版本: v1.0
