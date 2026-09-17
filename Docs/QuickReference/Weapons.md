# Weapons Quick Reference

版本: 1.1 | 最后更新: 2026-08-15（仅路径校正）

用途: AI日常开发快速查询（完整设计见 ../SystemDesign/GameDesign/NumericalDesign/Weapons/）

---

## 武器系统核心

伤害占比: 武器80% + 技能20%
TTK目标: 0.3-0.7秒（对150HP敌人）
武器槽位: 主武器（始终）+ 副武器（Lv20解锁）

---

## 六大武器系列

详见: ../SystemDesign/GameDesign/NumericalDesign/Weapons/WeaponData_武器数值.md（SSOT）

| 类型 | 代表武器 | 基础伤害 | 射速(RPM) | DPS | TTK(150HP) |
|------|---------|---------|----------|-----|-----------|
| 突击步枪(AR) | AK-47 | 35 | 600 | 350 | 0.4s(5发) |
| 冲锋枪(SMG) | MP5 | 25 | 900 | 375 | 0.4s(6发) |
| 霰弹枪(SG) | M870 | 120 | 60 | 120 | 0.2s(2发) |
| 狙击步枪(SR) | M24 | 120 | 40 | 80 | 0.1s(2发) |
| 轻机枪(LMG) | M249 | 40 | 750 | 500 | 0.3s(4发) |
| 火箭筒(RL) | RPG-7 | 300 | 20 | 100 | 0.1s(1发) |

---

## 关键武器数据速查

AK-47（全能AR）:
- BaseDamage: 35, Magazine: 30, FireRate: 600RPM, Reload: 2.2s
- 获取: Chapter 1 主线
- 推荐: 通用场景，平衡输出

M4A1（精准AR）:
- BaseDamage: 30, Magazine: 30, FireRate: 750RPM, Reload: 2.0s
- 获取: Chapter 2 商店购买
- 推荐: 中远程精准射击

MP5（高射速SMG）:
- BaseDamage: 25, Magazine: 30, FireRate: 900RPM, Reload: 1.8s
- 获取: Chapter 1 支线任务
- 推荐: 近战快速清场

M870（高爆发SG）:
- BaseDamage: 120(8弹丸×15), Magazine: 8, FireRate: 60RPM, Reload: 3.5s
- 获取: Chapter 2 宝箱
- 推荐: 近距离秒杀精英

M24（高伤SR）:
- BaseDamage: 120, Magazine: 5, FireRate: 40RPM, Reload: 3.0s
- 获取: Chapter 3 主线
- 推荐: 远程狙杀，弱点暴击

M249（火力压制LMG）:
- BaseDamage: 40, Magazine: 100, FireRate: 750RPM, Reload: 5.0s
- 获取: Chapter 4 Boss掉落
- 推荐: 持续火力，防守战

RPG-7（范围爆破RL）:
- BaseDamage: 300, Magazine: 1, FireRate: 20RPM, Reload: 4.0s, Radius: 8m
- 获取: Chapter 4 军火库
- 推荐: 清群怪，破坏掩体

---

## TTK验证表

详见: ../SystemDesign/GameDesign/NumericalDesign/Weapons/BalanceValidation_平衡验证.md

150HP敌人（Chapter 1-2）:
- AK-47: 5发0.4s
- MP5: 6发0.4s
- M870: 2发0.2s（近距离）
- M24: 2发3.0s（狙击）

300HP敌人（Chapter 3-4）:
- AK-47: 9发1.2s
- M249: 8发0.6s
- RPG-7: 1发直秒

600HP Boss（Chapter 5）:
- M249: 15发1.2s
- RPG-7: 2发8.0s（装填时间）
- 推荐: 武器+技能组合

---

## 弹药经济

详见: ../SystemDesign/GameDesign/NumericalDesign/Weapons/BalanceValidation_平衡验证.md

Chapter 5 最终防守战:
- 消耗: 149发（5波敌人）
- 掉落: 162发
- 结余: +13发（平衡正向）

弹药类型:
- 步枪弹: AR/SR通用
- 手枪弹: SMG通用
- 霰弹: SG专用
- 机枪弹: LMG专用
- 火箭弹: RL/GL专用

---

## CSV数据结构

详见: ../SystemDesign/GameDesign/NumericalDesign/Weapons/CSVStructure_CSV结构.md

核心表:
1. WeaponStats.csv - 武器基础数值（伤害/射速/弹匣/装填）
2. WeaponFalloffCurves.csv - 距离衰减曲线（近/中/远）
3. EnemyStatsPerChapter.csv - 敌人分章节生命值
4. AmmoConfig.csv - 弹药配置（容量/掉落）

导入: UE5 DataTable → C++ FWeaponStatsRow

---

## 伤害计算流程

```cpp
// 1. 基础伤害
float BaseDamage = WeaponStats.BaseDamage;

// 2. 距离衰减
float DistanceFactor = FalloffCurve.GetFloatValue(Distance);

// 3. 属性加成
float StrengthBonus = AttributeSet->GetWeaponDamageMultiplier(); // Strength影响

// 4. 弱点判定
float WeakpointMultiplier = bIsWeakpoint ? AttributeSet->GetWeakpointDamageMultiplier() : 1.0f;

// 5. 最终伤害
float FinalDamage = BaseDamage * DistanceFactor * StrengthBonus * WeakpointMultiplier;
```

---

## 推荐武器搭配

前期（Chapter 1-2）:
- 主武器: AK-47（全能）
- 副武器: MP5（近战清场）

中期（Chapter 3-4）:
- 主武器: M4A1（精准）
- 副武器: M870（精英秒杀）

后期（Chapter 5+）:
- 主武器: M249（火力压制）
- 副武器: RPG-7（范围清场）

Boss战:
- 主武器: M24（弱点狙击）
- 副武器: M249（持续输出）

---

## 实现参考

基类: AShootWeapon
数据表: Content/Data/Weapons/DT_WeaponStats
伤害计算: UShootDamageStatics::CalculateWeaponDamage()
CSV导入: Content/Data/Weapons/CSV/

---

## 跨文档引用

- 武器数值SSOT: ../SystemDesign/GameDesign/NumericalDesign/Weapons/WeaponData_武器数值.md
- CSV结构: ../SystemDesign/GameDesign/NumericalDesign/Weapons/CSVStructure_CSV结构.md
- 平衡验证: ../SystemDesign/GameDesign/NumericalDesign/Weapons/BalanceValidation_平衡验证.md
- 系统总览: ../SystemDesign/GameDesign/NumericalDesign/Weapons/Overview_总览.md
- 属性系统: ../SystemDesign/GameDesign/NumericalDesign/Attributes/Overview_总览.md
