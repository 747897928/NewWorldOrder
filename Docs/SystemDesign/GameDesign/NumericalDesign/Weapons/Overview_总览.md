# 武器系统总览 Weapon System Overview

文档版本: 1.0
最后更新: 2025-11-01
单一数据源标记: SystemDesign/GameDesign/NumericalDesign/Weapons/

---

## 系统定位

武器系统是新秩序的核心输出手段，占总伤害的80%。设计理念遵循守望先锋哲学：枪械是主要伤害来源，技能是倍增器。

核心设计理念：
- 武器占80%输出，技能占20%
- TTK目标：0.3-0.7秒
- 武器族群差异化明显
- 弹药经济制造战术张力
- 双武器槽（Lv20解锁）增加战术深度

---

## 数据管道

CSV → DataTable → 武器实例

主数据表：
- WeaponStats.csv → DT_WeaponStats
- 包含BaseDamage、MagazineSize、FireRateRPM、扩散参数等

衰减曲线表：
- WeaponFalloffCurves.csv → DT_FalloffCurves
- 定义四个关键点（距离cm / 倍率）

敌人血量表：
- EnemyStatsPerChapter.csv → DT_EnemyStats
- 按章节/难度设置HP，供平衡验证

弹药配置表：
- AmmoConfig.csv → DT_AmmoConfig
- 指定最大携带量与掉落数量

实现：
- ARangedWeaponInstance在LoadWeaponData中根据WeaponID拉取数据
- 详细结构：CSVStructure_CSV结构.md

---

## 武器族群

### 突击步枪（AR）
定位：中距离全能
代表武器：AK47、M4A1、SCAR-H
特点：
- 平衡的伤害、射速、精准度
- 20-50米最佳交战距离
- 30发弹夹，适合持续输出

### 冲锋枪（SMG）
定位：高射速近战
代表武器：MP5、UMP45、Vector
特点：
- 高射速、低伤害
- 近距离优势明显
- 换弹快，适合游走

### 霰弹枪（SG）
定位：5米爆发
代表武器：M870、AA-12
特点：
- 极高单发伤害
- 5米内秒杀，10米急剧衰减
- 单发装填或弹夹，各有优劣

### 狙击枪（SR）
定位：远距离精准
代表武器：AWP、M24
特点：
- 极高伤害，低射速
- 身体秒杀普通敌人
- 弱点伤害极限爆发

### 轻机枪（LMG）
定位：持续火力
代表武器：M249、RPK
特点：
- 大弹夹（75-100发）
- 持续输出能力强
- 换弹慢，需要战术时机

### 爆炸武器（RL/GL）
定位：战术工具
代表武器：RPG-7、M32榴弹
特点：
- 极高AOE伤害
- 弹药稀缺
- 用于Boss或紧急情况

详细数值：WeaponData_武器数值.md

---

## 武器槽位

Lv1-19：1把主武器 + 1把手枪
Lv20+：2把主武器 + 1把手枪

插槽命名：
- weapon_socket_hand_r：右手主武器
- weapon_socket_spine_back_r/l：背部右/左（第二主武器）
- weapon_socket_thigh_r：手枪（大腿）
- weapon_socket_pelvis_melee：近战（腰间）

切枪机制：
- 按3键切换主武器
- 动画1.5秒，期间无法瞬切
- 服务器驱动，防止作弊

---

## 衰减系统

距离衰减曲线：

Curve_CloseRange（霰弹）：
- 0-5米：100%伤害
- 10米：50%伤害
- 20米：10%伤害

Curve_MidRange（步枪/冲锋枪）：
- 0-20米：100%伤害
- 50米：70%伤害
- 80米：40%伤害

Curve_LongRange（狙击）：
- 0-30米：100%伤害
- 60米：90%伤害
- 100米：75%伤害

Curve_NoFalloff（爆炸类）：
- 全距离100%伤害

详细曲线：CSVStructure_CSV结构.md#WeaponFalloffCurves

---

## 关键公式

距离伤害：
```
FinalDamage = BaseDamage × DistanceFalloff(Distance)
```

弱点伤害：
```
CriticalDamage = FinalDamage × CritMultiplier × (1 + PerceptionBonus)
```

护甲穿透：
```
EffectiveArmor = ArmorValue × (1 - StrengthPenetration)
```

TTK计算：
```
TTK = (击杀发数 - 1) / (射速RPM / 60)
```

---

## 平衡目标

TTK目标：0.3-0.7秒

伤害比例：
- 武器：80%
- 技能：20%（倍增器）

武器成长：
- 初期：M4A1（击杀游荡者0.5秒）
- 中期：AK47（击杀游荡者0.3秒）
- 后期：力量30+AK47（击杀游荡者0.2秒）

章节血量递增：
- Chapter 1-3：游荡者150HP
- Chapter 4-6：游荡者180HP
- Chapter 7-9：游荡者220HP
- Chapter 10：游荡者250HP

质变点：
- 力量30点：AK47从4发→3发击杀（质变点）
- Chapter 7血量220HP：制造4发→5发（维持难度）

详细验证：BalanceValidation_平衡验证.md

---

## 弹药经济

携带量设计：
- AR/SMG：150发（5个弹夹）
- SG：40发（5个弹夹）
- SR：20发（4个弹夹）
- LMG：200发（2个弹夹）
- RL：3发（稀缺资源）

掉落机制：
- 普通敌人：10%概率（感知+1%/点）
- 特殊敌人：30%概率
- 精英敌人：60%概率

弹药平衡：
- Chapter 5防守战（60秒）：
  - 消耗：149发（AK47，力量30）
  - 掉落：162发（基础掉落率）
  - 结余：193发（刚好够用）

详细配置：CSVStructure_CSV结构.md#AmmoConfig

---

## 双武器槽策略

Lv20解锁双武器槽后：

弹药规则：
- 总携带量不变
- 弹药类型独立（步枪弹≠霰弹）
- 拾取弹药仅填充对应类型

配置示例：

配置1：AK47 + M870（全能+近战）
- AK47：150发+30发=180发
- M870：40发+8发=48发
- 总携带：228发

配置2：AK47 + AWP（输出+狙击）
- AK47：150发+30发=180发
- AWP：20发+5发=25发
- 总携带：205发

配置3：M249 + M870（压制+爆发）
- M249：200发+100发=300发
- M870：40发+8发=48发
- 总携带：348发

---

## 武器解锁时间线

Chapter 1（教学）：
- 初始：M4A1 + 格洛克17

Chapter 2-3（探索）：
- 解锁：AK47、M870、MP5

Chapter 4-5（成长）：
- 解锁：UMP45、M24

Chapter 6-7（强化）：
- 解锁：Vector、SCAR-H、AWP

Chapter 8-9（武装）：
- 解锁：M249、RPK、AA-12、沙漠之鹰

Chapter 10（终局）：
- 解锁：RPG-7、M32榴弹

详细获取方式：WeaponData_武器数值.md#武器获取

---

## 与其他系统交互

属性系统：
- 力量：武器伤害+45%（30点）
- 敏捷：换弹速度-45%（30点）
- 感知：弱点倍率+30%（30点）
- 参考：../Attributes/Overview_总览.md

技能系统：
- 战术标记：伤害+25%
- 破甲射击：弱点倍率+30%
- 战术超载：射速+30%+无限弹药
- 参考：../Skills/Overview_总览.md

存档系统：
- 武器装备状态保存在Snapshot中
- 包括：WeaponID、弹药、配件、品质
- 参考：../../../Tasks/CharacterSwitching/DesignGuide_设计指南.md

---

## 实现要点

GAS集成：
- 武器是SourceObject，技能挂在角色ASC
- 装备时授予能力（GrantAbilitiesToOwner）
- 卸载时移除能力

代码位置：
- Equipment/Weapon/RangedWeaponInstance.h/cpp
- LoadWeaponData：从DataTable加载数据
- CalculateDamageAtDistance：距离伤害计算
- CalculateCriticalDamage：弱点伤害计算

网络同步：
- 弹药管理在服务器执行
- 扩散恢复在服务器计算
- OnRep通过UGameplayMessageSubsystem广播到UI

换弹方式：
- ReloadType = Magazine：整匣换弹
- ReloadType = Single：逐发装填（霰弹枪）

新武器流程：
1. 更新CSV
2. 重新生成DataTable
3. 配置UWeaponDefinition
4. 验证挂点与动画

---

## 设计原则

武器差异化：
- 每个族群有明确定位
- 不同距离最佳武器不同
- 弹夹大小影响战斗节奏

数值明确性：
- BaseDamage=35（每发35伤害）
- FireRateRPM=600（发/分钟）
- ReloadTime=2.2秒（换弹时间）

防滥用机制：
- 弹药限制制造战术张力
- 爆炸武器弹药稀缺
- 双武器槽Lv20解锁，避免前期太强

成长曲线：
- 初期武器够用但非最优
- 中期解锁高级武器
- 后期属性加成达成质变

---

参考文档：
- WeaponData_武器数值.md - 所有武器数据（单一数据源）
- CSVStructure_CSV结构.md - CSV表结构定义
- BalanceValidation_平衡验证.md - TTK验证与敌人分级
- ../Attributes/Formulas_公式.md - 属性影响武器
- ../Skills/Overview_总览.md - 技能协同
- ../../claude/新秩序武器系统完整设计文档.md - 原始设计（归档）

最后更新: 2025-11-01
维护者: 项目团队
