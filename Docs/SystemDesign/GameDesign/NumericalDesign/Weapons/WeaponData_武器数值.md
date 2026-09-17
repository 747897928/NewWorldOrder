# 武器数值 Weapon Data

文档版本: 1.0
最后更新: 2025-11-01
单一数据源标记: SINGLE SOURCE OF TRUTH

警告：本文档是所有武器数值的唯一权威来源
修改此文档前必须通知策划团队并更新版本号
任何其他文档中的武器描述仅供参考，以本文档为准

---

## 突击步枪 Assault Rifles

### AK-47

WeaponID: WPN_AK47
WeaponName: AK-47
WeaponType: AR
WeaponTier: 3

基础数值：
- BaseDamage: 35.0
- MagazineSize: 30
- FireRateRPM: 600.0（10发/秒）
- FireDelayTimeSecs: 0.12（开火后延迟时间，影响实际射速）
- ReloadTime: 2.2秒
- ReloadType: Magazine

扩散参数：
- BaseSpreadAngle: 1.5度
- SpreadPerShot: 0.8度
- MaxSpreadAngle: 6.0度
- SpreadRecovery: 3.0度/秒

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 350（35×10）
- 弹夹打空时间: 3秒
- 理论TTK（游荡者150HP）: 0.4秒（5发）
- 后期TTK（力量30）: 0.2秒（3发）

设计定位：
- 高伤害重后坐力
- 需要点射控制
- 力量Build首选
- 配合破甲射击达成极限输出

获取方式：
- Chapter 3主线奖励
- 必得武器

---

### M4A1

WeaponID: WPN_M4A1
WeaponName: M4A1
WeaponType: AR
WeaponTier: 3

基础数值：
- BaseDamage: 30.0
- MagazineSize: 30
- FireRateRPM: 700.0（11.67发/秒）
- FireDelayTimeSecs: 0.12（开火后延迟时间，影响实际射速）
- ReloadTime: 2.0秒
- ReloadType: Magazine

扩散参数：
- BaseSpreadAngle: 1.0度
- SpreadPerShot: 0.5度
- MaxSpreadAngle: 4.5度
- SpreadRecovery: 3.5度/秒

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 350（30×11.67）
- 理论TTK（游荡者150HP）: 0.43秒（6发）
- 后期TTK（力量30）: 0.26秒（4发）

设计定位：
- 低伤害高精准
- 可持续扫射
- 新手友好
- 配合速射手被动

获取方式：
- Chapter 1初始武器
- 必得武器

---

### SCAR-H

WeaponID: WPN_SCAR
WeaponName: SCAR-H
WeaponType: AR
WeaponTier: 4

基础数值：
- BaseDamage: 38.0
- MagazineSize: 20
- FireRateRPM: 550.0（9.17发/秒）
- FireDelayTimeSecs: 0.12（开火后延迟时间，影响实际射速）
- ReloadTime: 2.5秒
- ReloadType: Magazine

扩散参数：
- BaseSpreadAngle: 1.8度
- SpreadPerShot: 0.9度
- MaxSpreadAngle: 7.0度
- SpreadRecovery: 2.8度/秒

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 349（38×9.17）
- 理论TTK（游荡者150HP）: 0.33秒（4发）
- 单点伤害最高

设计定位：
- 重型突击步枪
- 单点爆发高
- 弹夹小需要精准
- 适合精准射手

获取方式：
- Chapter 7主线奖励
- 必得武器

---

## 冲锋枪 Submachine Guns

### MP5

WeaponID: WPN_MP5
WeaponName: MP5
WeaponType: SMG
WeaponTier: 2

基础数值：
- BaseDamage: 18.0
- MagazineSize: 30
- FireRateRPM: 800.0（13.33发/秒）
- FireDelayTimeSecs: 0.10（开火后延迟时间，SMG射速快）
- ReloadTime: 1.8秒
- ReloadType: Magazine

扩散参数：
- BaseSpreadAngle: 1.2度
- SpreadPerShot: 0.4度
- MaxSpreadAngle: 5.0度
- SpreadRecovery: 4.0度/秒

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 240（18×13.33）
- 理论TTK（游荡者150HP）: 0.6秒（9发）
- 换弹快1.8秒

设计定位：
- 平衡型SMG
- 近战游走
- 换弹快机动高
- 女主辅助流首选

获取方式：
- Chapter 2宝箱
- 探索获得

---

### UMP45

WeaponID: WPN_UMP45
WeaponName: UMP45
WeaponType: SMG
WeaponTier: 3

基础数值：
- BaseDamage: 22.0
- MagazineSize: 25
- FireRateRPM: 650.0（10.83发/秒）
- FireDelayTimeSecs: 0.10（开火后延迟时间，SMG射速快）
- ReloadTime: 2.0秒
- ReloadType: Magazine

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 238（22×10.83）
- 理论TTK（游荡者150HP）: 0.55秒（7发）

设计定位：
- 慢速重弹SMG
- 介于SMG和AR之间
- 过渡期武器

获取方式：
- Chapter 4商店，500金币
- 购买获得

---

### Vector

WeaponID: WPN_Vector
WeaponName: Vector
WeaponType: SMG
WeaponTier: 4

基础数值：
- BaseDamage: 16.0
- MagazineSize: 33
- FireRateRPM: 1200.0（20发/秒）
- FireDelayTimeSecs: 0.08（开火后延迟时间，极速SMG）
- ReloadTime: 1.5秒
- ReloadType: Magazine

扩散参数：
- BaseSpreadAngle: 0.8度
- SpreadPerShot: 0.3度
- MaxSpreadAngle: 4.0度
- SpreadRecovery: 5.0度/秒

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 320（16×20）
- 理论TTK（游荡者150HP）: 0.45秒（10发）
- 弹夹打空时间: 1.65秒
- 后坐力极小

设计定位：
- 极速射击
- 子弹风暴
- 配合战术超载大招
- 后坐力极小可持续扫射

获取方式：
- Chapter 6隐藏房间（找3个密码）
- 隐藏挑战

---

## 霰弹枪 Shotguns

### M870

WeaponID: WPN_M870
WeaponName: M870
WeaponType: SG
WeaponTier: 3

基础数值：
- BaseDamage: 15.0（每颗弹丸）
- MagazineSize: 8
- FireRateRPM: 60.0（1发/秒）
- FireDelayTimeSecs: 0.20（开火后延迟时间，霰弹枪后坐力大）
- ReloadTime: 0.6秒/发
- ReloadType: Single

扩散参数：
- BaseSpreadAngle: 3.0度
- SpreadPerShot: 0（霰弹不累积扩散）
- MaxSpreadAngle: 3.0度
- SpreadRecovery: 0

伤害参数：
- BulletsPerShot: 8
- CritMultiplier: 1.5（霰弹弱点倍率较低）
- FalloffCurveID: Curve_CloseRange

实战数据（重要）：
- 理论伤害: 15×8=120
- 实战命中率（3度扩散）:
  - 5米内: 75%命中率（6颗）=90伤害
  - 10米: 50%命中率（4颗）+衰减50%=30伤害
  - 15米: 基本无效
- 击杀游荡者150HP: 2发（5米内）
- TTK: 1.0秒

设计定位：
- 单发霰弹
- 近战爆发
- 配合处决打击斩杀
- 单发装填需要规划

获取方式：
- Chapter 3支线：清理农场
- 支线任务

---

### AA-12

WeaponID: WPN_AA12
WeaponName: AA-12
WeaponType: SG
WeaponTier: 4

基础数值：
- BaseDamage: 12.0（每颗弹丸）
- MagazineSize: 20
- FireRateRPM: 300.0（5发/秒）
- FireDelayTimeSecs: 0.18（开火后延迟时间，自动霰弹枪）
- ReloadTime: 2.5秒
- ReloadType: Magazine

扩散参数：
- BaseSpreadAngle: 3.5度
- SpreadPerShot: 0
- MaxSpreadAngle: 3.5度
- SpreadRecovery: 0

伤害参数：
- BulletsPerShot: 8
- CritMultiplier: 1.5
- FalloffCurveID: Curve_CloseRange

实战数据：
- 理论伤害: 12×8=96
- 实战命中率（3.5度扩散）:
  - 5米内: 70%命中率（5.6颗）=67伤害
  - 10米: 45%命中率（3.6颗）+衰减50%=22伤害
- 击杀游荡者150HP: 3发（5米内）
- TTK: 0.4秒

设计定位：
- 自动霰弹
- 贴脸DPS最高
- 20发弹夹持续输出
- 适合坦克流

获取方式：
- Chapter 9支线：地下室防守
- 支线任务

---

## 手枪 Pistols

### 格洛克17 Glock 17

WeaponID: WPN_Glock17
WeaponName: 格洛克17
WeaponType: Pistol
WeaponTier: 1

基础数值：
- BaseDamage: 20.0
- MagazineSize: 17
- FireRateRPM: 450.0（7.5发/秒）
- FireDelayTimeSecs: 0.15（开火后延迟时间，手枪标准）
- ReloadTime: 1.5秒
- ReloadType: Magazine

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 150（20×7.5）
- 理论TTK（游荡者150HP）: 1.07秒（8发）

设计定位：
- 副武器标准
- 补刀/应急
- 换弹快

获取方式：
- 初始武器
- 必得

---

### 沙漠之鹰 Desert Eagle

WeaponID: WPN_DesertEagle
WeaponName: 沙漠之鹰
WeaponType: Pistol
WeaponTier: 3

基础数值：
- BaseDamage: 45.0
- MagazineSize: 7
- FireRateRPM: 280.0（4.67发/秒）
- FireDelayTimeSecs: 0.20（开火后延迟时间，大口径手枪）
- ReloadTime: 2.0秒
- ReloadType: Magazine

扩散参数：
- BaseSpreadAngle: 2.0度
- SpreadPerShot: 1.2度
- MaxSpreadAngle: 8.0度
- SpreadRecovery: 2.5度/秒

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 210（45×4.67）
- 弱点伤害: 45×2.0=90
- 2发爆头秒杀游荡者（180伤害）
- 理论TTK（游荡者150HP）: 0.77秒（4发）

设计定位：
- 手炮
- 高伤害低射速
- 后坐力大需要控制
- 奖励精准

获取方式：
- Chapter 9隐藏BOSS
- BOSS奖励

---

## 狙击枪 Sniper Rifles

### AWP

WeaponID: WPN_AWP
WeaponName: AWP
WeaponType: SR
WeaponTier: 5

基础数值：
- BaseDamage: 180.0
- MagazineSize: 5
- FireRateRPM: 50.0（1.2秒/发）
- FireDelayTimeSecs: 0.30（开火后延迟时间，狙击枪拉栓）
- ReloadTime: 3.0秒
- ReloadType: Magazine

扩散参数：
- BaseSpreadAngle: 0.1度
- SpreadPerShot: 0
- MaxSpreadAngle: 0.1度
- SpreadRecovery: 0

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.5（狙击枪弱点倍率更高）
- FalloffCurveID: Curve_LongRange

实战数据：
- 身体伤害: 180（直接秒杀游荡者150HP）
- 弱点伤害: 180×2.5=450
- 配合破甲射击: 180×2.6=468（感知30点+破甲30%）
- TTK: 0秒（身体秒杀）

设计定位：
- 反器材狙击
- 身体秒杀普通敌人
- 奖励精准但容错高
- 配合破甲+感知达成极限输出

获取方式：
- Chapter 7密室，挑战：60秒造成10000伤害
- 隐藏挑战

---

### M24

WeaponID: WPN_M24
WeaponName: M24
WeaponType: SR
WeaponTier: 4

基础数值：
- BaseDamage: 150.0
- MagazineSize: 5
- FireRateRPM: 60.0（1.0秒/发）
- FireDelayTimeSecs: 0.30（开火后延迟时间，狙击枪拉栓）
- ReloadTime: 2.8秒
- ReloadType: Magazine

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.5
- FalloffCurveID: Curve_LongRange

实战数据：
- 身体伤害: 150（刚好秒杀游荡者150HP）
- 弱点伤害: 150×2.5=375
- 配合标记+25%: 150×1.25=187.5（可秒杀带标记的游荡者）
- 射速稍快，容错高

设计定位：
- 战术狙击
- 射速比AWP快
- 容错高
- 配合标记增伤

获取方式：
- Chapter 5 BOSS奖励
- BOSS奖励

---

## 轻机枪 Light Machine Guns

### M249

WeaponID: WPN_M249
WeaponName: M249
WeaponType: LMG
WeaponTier: 4

基础数值：
- BaseDamage: 23.0
- MagazineSize: 100
- FireRateRPM: 800.0（13.33发/秒）
- FireDelayTimeSecs: 0.12（开火后延迟时间，机枪持续火力）
- FireDelayTimeSecs: 0.30（开火后延迟时间，狙击枪拉栓）
- ReloadTime: 4.5秒
- ReloadType: Magazine

扩散参数：
- BaseSpreadAngle: 2.0度
- SpreadPerShot: 0.6度
- MaxSpreadAngle: 8.0度
- SpreadRecovery: 2.0度/秒

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 307（23×13.33）
- 弹夹打空时间: 7.5秒
- 理论TTK（游荡者150HP）: 0.45秒（7发）
- 100发持续输出

设计定位：
- 弹幕压制
- 100发弹夹持续输出
- 换弹慢4.5秒
- 配合战术超载无限弹药
- 适合防守战

获取方式：
- Chapter 8弹药库突袭
- 探索获得

---

### RPK

WeaponID: WPN_RPK
WeaponName: RPK
WeaponType: LMG
WeaponTier: 4

基础数值：
- BaseDamage: 28.0
- MagazineSize: 75
- FireRateRPM: 650.0（10.83发/秒）
- FireDelayTimeSecs: 0.12（开火后延迟时间，机枪持续火力）
- ReloadTime: 4.0秒
- ReloadType: Magazine

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 2.0
- FalloffCurveID: Curve_MidRange

实战数据：
- DPS: 303（28×10.83）
- 理论TTK（游荡者150HP）: 0.46秒（6发）
- 单发伤害更高

设计定位：
- 平衡机枪
- 单发伤害比M249高
- 换弹略快
- 适合持续输出

获取方式：
- Chapter 9主线奖励
- 必得武器

---

## 爆炸武器 Explosive Weapons

### RPG-7

WeaponID: WPN_RPG7
WeaponName: RPG-7
WeaponType: RL
WeaponTier: 5

基础数值：
- BaseDamage: 800.0
- MagazineSize: 1
- FireRateRPM: 20.0（3秒/发）
- FireDelayTimeSecs: 0.50（开火后延迟时间，火箭发射器后坐力极大）
- ReloadTime: 4.0秒
- ReloadType: Single

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 1.0（爆炸武器无弱点）
- FalloffCurveID: Curve_NoFalloff
- ExplosionRadius: 3米

实战数据：
- 800伤害AOE
- 一发秒杀吞噬者（600HP精英）
- 无距离衰减
- 弹药极其稀缺

设计定位：
- 反坦克武器
- 用于Boss或紧急情况
- 弹药稀缺（最多3发）
- 无距离衰减

获取方式：
- Chapter 10军火库，密码解谜
- 隐藏挑战

---

### M32榴弹发射器

WeaponID: WPN_M32
WeaponName: M32榴弹
WeaponType: GL
WeaponTier: 4

基础数值：
- BaseDamage: 200.0
- MagazineSize: 6
- FireRateRPM: 180.0（3发/秒）
- FireDelayTimeSecs: 0.40（开火后延迟时间，榴弹发射器）
- ReloadTime: 3.5秒
- ReloadType: Magazine

伤害参数：
- BulletsPerShot: 1
- CritMultiplier: 1.0
- FalloffCurveID: Curve_NoFalloff
- ExplosionRadius: 2米

实战数据：
- 200伤害AOE
- 6发转轮
- 配合震撼手雷易伤+30%=260伤害
- 适合清理聚集敌人

设计定位：
- 榴弹发射器
- 清理聚集敌人
- 6发转轮可持续输出
- 配合技能易伤效果

获取方式：
- Chapter 10探索
- 探索获得

---

## 武器获取汇总

主线获取（必得）：
- M4A1（Chapter 1初始）
- AK47（Chapter 3主线）
- SCAR-H（Chapter 7主线）
- RPK（Chapter 9主线）

支线任务：
- M870（Chapter 3：清理农场）
- AA-12（Chapter 9：地下室防守）

商店购买：
- UMP45（Chapter 4，500金币）

BOSS奖励：
- M24（Chapter 5 BOSS）
- 沙漠之鹰（Chapter 9隐藏BOSS）

隐藏挑战：
- Vector（Chapter 6：密码解谜）
- AWP（Chapter 7：伤害挑战）
- RPG-7（Chapter 10：军火库密码）

宝箱/探索：
- MP5（Chapter 2：宝箱）
- M249（Chapter 8：弹药库）
- M32（Chapter 10：探索）

---

## 武器推荐路线

新手路线（容错高）：
- Chapter 1-3：M4A1（主）+ 格洛克17（副）
- Chapter 4-6：AK47（主）+ M870（副）
- Chapter 7-9：SCAR-H（主）+ M24（副）
- Chapter 10：RPK（主）+ M32（副）

激进路线（高伤害）：
- Chapter 1-3：M4A1（主）+ MP5（副）
- Chapter 4-6：AK47（主）+ UMP45（副）
- Chapter 7-9：AWP（主）+ Vector（副）
- Chapter 10：SCAR-H（主）+ RPG-7（副）

防守路线（持续输出）：
- Chapter 1-3：M4A1（主）+ MP5（副）
- Chapter 4-6：AK47（主）+ M870（副）
- Chapter 7-9：M249（主）+ AA-12（副）
- Chapter 10：RPK（主）+ M32（副）

---

参考文档：
- Overview_总览.md - 武器系统总览
- CSVStructure_CSV结构.md - CSV表结构
- BalanceValidation_平衡验证.md - TTK验证
- ../Attributes/Formulas_公式.md - 属性影响
- ../Skills/Overview_总览.md - 技能协同
- ../../claude/新秩序武器系统完整设计文档.md - 原始设计（归档）

最后更新: 2025-11-01
维护者: 项目团队
武器数值版本: v1.0
