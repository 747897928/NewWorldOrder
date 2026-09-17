# Attributes Quick Reference

版本: 1.1 | 最后更新: 2026-08-15（仅路径校正）

用途: AI日常开发快速查询（完整设计见 ../SystemDesign/GameDesign/NumericalDesign/Attributes/）

---

## 四大属性体系

| 属性 | 英文 | 核心效果 | 每点收益 |
|------|------|---------|---------|
| 体质 | Vitality | 生命值、生命回复 | 10→6→4 HP/点 |
| 力量 | Strength | 武器伤害、近战伤害 | 3%→2%→1.5% 伤害/点 |
| 敏捷 | Agility | 移速、换弹速度、冲刺距离 | 2%→1.5%→1% 提升/点 |
| 感知 | Perception | 弱点伤害、暴击率 | 5%→3%→2% 弱点加成/点 |

递减规则: 0-10点（高收益）→ 11-20点（中收益）→ 21-30点（低收益）

---

## 初始属性

男主（陈浩宇）:
- Vitality: 5, Strength: 5, Agility: 3, Perception: 2

女主（沈芸皖）:
- Vitality: 4, Strength: 3, Agility: 5, Perception: 3

总点数: 15点（双主角相同）

---

## 核心公式（SSOT）

详见: ../SystemDesign/GameDesign/NumericalDesign/Attributes/Formulas_公式.md

体质（Vitality）:
```
MaxHP = 100 + (V≤10: V×10) + (V>10: 100+(V-10)×6) + (V>20: 160+(V-20)×4)
HPRegen = 1 + V×0.1 (每秒)
```

力量（Strength）:
```
WeaponDamage% = 100 + (S≤10: S×3) + (S>10: 30+(S-10)×2) + (S>20: 50+(S-20)×1.5)
MeleeDamage = 20 + S×2
```

敏捷（Agility）:
```
MoveSpeed% = 100 + (A≤10: A×2) + (A>10: 20+(A-10)×1.5) + (A>20: 35+(A-20)×1)
ReloadSpeed% = 100 + (A≤10: A×2) + (A>10: 20+(A-10)×1.5) + (A>20: 35+(A-20)×1)
SprintDistance = 500 + A×20
```

感知（Perception）:
```
WeakpointDamage% = 150 + (P≤10: P×5) + (P>10: 50+(P-10)×3) + (P>20: 80+(P-20)×2)
CritChance% = P×1
CritDamage% = 200（固定）
```

---

## 质变点推荐

详见: ../SystemDesign/GameDesign/NumericalDesign/Attributes/GrowthCurves_成长曲线.md

早期（Lv1-10）:
- Vitality: 8-10点（突破200HP，对抗Chapter 1-2敌人）
- Strength: 5-8点（提升武器伤害30-45%）

中期（Lv11-20）:
- Agility: 10-12点（移速+20-24%，换弹快25%）
- Perception: 8-10点（弱点伤害200%）

后期（Lv21-30）:
- 根据Build需求分配剩余点数
- 避免单属性超过25点（收益过低）

---

## 属性点获取

每升1级: +2属性点
总计（Lv30）: 15（初始）+ 58（升级）= 73点

---

## 实现参考

数据表: Content/Data/Attributes/DT_AttributeFormulas
UE类: UShootAttributeSet
公式验证: ../SystemDesign/GameDesign/NumericalDesign/Attributes/Formulas_公式.md（SSOT）

---

## 跨文档引用

- 技能系统: ../SystemDesign/GameDesign/NumericalDesign/Skills/Overview_总览.md
- 武器系统: ../SystemDesign/GameDesign/NumericalDesign/Weapons/Overview_总览.md
- 成长曲线: ../SystemDesign/GameDesign/NumericalDesign/Attributes/GrowthCurves_成长曲线.md
- 完整公式: ../SystemDesign/GameDesign/NumericalDesign/Attributes/Formulas_公式.md
