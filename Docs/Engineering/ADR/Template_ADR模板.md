# ADR-XXXX: [决策标题]

## 元数据

```yaml
adr_id: ADR-XXXX
title: [决策标题]
date: YYYY-MM-DD
status: Active / Draft / Superseded
supersedes: ADR-YYYY（如果替代了旧决策）
superseded_by: ADR-ZZZZ（如果被新决策替代，由新决策作者填写）
superseded_date: YYYY-MM-DD（被替代的日期）
author: [作者名]
related_docs:
  - [相关文档路径]
```

## 决策

[一句话描述决策是什么]

示例：
- 使用预测视图方案实现弹药UI预测
- RangedWeaponInstance拆分为HitscanWeaponInstance和ProjectileWeaponInstance

## 背景

[为什么需要做这个决策？遇到了什么问题？]

示例：
- 问题：客户端扣弹药后，服务器同步有延迟，UI显示不流畅
- 现状：直接照搬Lyra的ItemTagStack需要完整库存系统
- 约束：短期内不引入完整库存系统

## 备选方案

### 方案A：[方案名]

[描述]

优点：
- [优点1]
- [优点2]

缺点：
- [缺点1]
- [缺点2]

### 方案B：[方案名]

[描述]

优点：
- [优点1]

缺点：
- [缺点1]

## 选择的方案

选择：[方案X]

理由：
- [理由1]
- [理由2]
- [理由3]

## 后果

### 正面影响
- [正面影响1]
- [正面影响2]

### 负面影响
- [负面影响1]
- [负面影响2]

### 需要注意的地方
- [注意事项1]
- [注意事项2]

## 实现细节

[关键代码位置、类名、函数名]

示例：
- 类：ARangedWeaponInstance
- 字段：PredictedAmmo, PredictedReserve
- 函数：UShootAbilityCost_Ammo::CheckCost()

## 相关文档

- 详细设计：[路径]
- 实现笔记：[路径]
- 相关ADR：[ADR编号]

## 更新历史

- [YYYY-MM-DD] 创建ADR
- [YYYY-MM-DD] 更新状态为Superseded（如果适用）
