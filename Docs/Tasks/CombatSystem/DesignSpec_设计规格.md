# 设计规格

## 范围
- 伤害/治疗/控制效果 GE
- 技能 Actor（爆炸、医疗站、盾墙）
- GA 触发与 Cost 机制（AmmoTagStack 现状）
- 命中反馈与 TargetData 回传流程

## 现有资产（代码证据）
- 易伤：`ShootEffect_Vulnerable`
- 治疗：`ShootEffect_HealInstant`
- 快照恢复：`ShootEffect_SnapshotRestore`
- Buff/DR：`ShootEffect_MoveSpeed/FireRate/ReloadSpeed/ShieldBonus/DamageReduction/HealingDone/HealingReceived`
- 技能 Actor：`ShootSkillExplosionActor`、`ShootSkillMedicalStation`、`AShootSkillShieldWall`
- TargetData 回传：`UShootGameplayAbility_Weapon_Fire`

## 主要缺口
- 减速依赖 MoveSpeedMultiplier，尚需角色移动系统消费该属性

## 正史对照（男主技能）

来源：Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md

战术突击 Q
- 正史要点：冲刺5米；0.3秒无敌；3秒内射速+30%与移速+20%；满级撞击50伤害+1秒眩晕
- 代码现状：LaunchCharacter 位移 + 移速/射速 Buff 已有；i-frame/硬直与命中检测未完整
- 缺口：冲刺距离/升级曲线、i-frame Tag、撞击伤害/眩晕与命中规则、与 FX/Cue 绑定

震撼手雷 E
- 正史要点：30秒 CD；伤害150；范围3米（尸潮可+50%）；不同敌人血量段控制差异；易伤+30% 5秒
- 代码现状：爆炸 Actor 伤害/眩晕/易伤占位；已支持 Stun 与 Knockback
- 缺口：基于目标血量的控制分层逻辑；尸潮增益判定；投掷轨迹/落点与表现

钢铁壁垒 C
- 正史要点：5秒护盾，正面-80%远程伤害，移速-30%，可取消；升级可反伤/护盾破裂爆炸
- 代码现状：AShootSkillShieldWall 可生成护盾墙；未实现正面减伤、移速惩罚、反伤与爆炸规则
- 缺口：正面减伤判定与数值、移速惩罚、取消逻辑、反伤与爆炸触发

战术超载 X
- 正史要点：持续8秒；击杀延长；充能来源（击杀/受伤/命中/时间）
- 代码现状：已存在 Overload 状态 Tag 与相关 GA
- 缺口：充能规则与数值落地、延长窗口与击杀判定

## 正史对照（女主技能）

来源：Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md

战术扫描 Q
- 正史要点：22秒 CD；15米范围；持续15秒；标记穿墙与弱点高亮；弱点伤害+10%；尸潮红利持续翻倍
- 代码现状：已实现标记与易伤基础逻辑（TacticalScan）；状态 Tag 可用；弱点伤害+10%已落地；尸潮红利持续翻倍已落地
- 缺口：弱点高亮表现

救援掩护 E
- 正史要点：30秒 CD；隐身6秒；移速+30%；救援速度×2；救起后队友50%HP+3秒无敌；攻击打破隐身
- 代码现状：RescueCloak 具备隐身状态；移速倍率已 SetByCaller 配置；攻击破隐已接入；救起后治疗与保护已接入事件回调；救援速度倍率已注入交互事件并由救援交互能力读取
- 缺口：AI 感知与 UI 订阅；倒地角色需挂载救援交互组件并在倒地时启用

疾速充能 C
- 正史要点：18秒 CD；移速+50%持续4秒；射击不减速；击杀刷新持续时间
- 代码现状：RapidCharge 支持击杀刷新并按等级注入移速/换弹倍率
- 缺口：射击不减速机制、精英击杀奖励、升级曲线完整数值落地

医疗站 X
- 正史要点：5米范围持续15秒；每秒5% MaxHP；充能来源（治疗/队友受伤/时间）
- 代码现状：ShootSkillMedicalStation 存在周期治疗占位
- 缺口：充能规则与数值落地、治疗频率/强度与队友判定

## 正史对照（通用被动）

来源：Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md

生存大师 Survivalist
- 正史要点：MaxHP 提升；低血移速提升；低血减伤；护盾破碎后 CD 缩短
- 代码现状：未见对应通用被动 GA/GE
- 缺口：低血阈值判定、移速/减伤数值、护盾破碎事件与 CD 缩短触发

战术专家 Tactician
- 正史要点：全技能 CD 降低；满级影响大招充能效率
- 代码现状：未见对应通用被动 GA/GE
- 缺口：技能 CD 修正与充能效率修正的全局入口

速射手 Speedloader
- 正史要点：换弹速度提升；换弹时移动不减速；击杀装填返还弹药
- 代码现状：未见对应通用被动 GA/GE
- 缺口：换弹速度倍率、移动减速豁免、击杀装填逻辑

精准专家 Precision Expert
- 正史要点：暴击倍率相关数值提升（文档有数值例子）
- 代码现状：未见对应通用被动 GA/GE
- 缺口：暴击倍率加成的属性/GE 接入

## 正史对照（男主专属被动）

标记猎手 Marked Hunter
- 正史要点：击杀不同血量段回血；满级标记传播+50%
- 代码现状：击杀回血已按等级落地（>200HP 精英回血；Lv3 解锁普通击杀回血）；Lv3 标记传播已落地（传播半径+50%可配置）
- 缺口：传播表现与弱点高亮仍需结合 UI/Cue

装甲强化 Armor Enhancement
- 正史要点：护盾加成、持续+3秒、护盾破碎爆炸
- 代码现状：存在被动 GA/GE 占位，未见护盾破碎爆炸规则
- 缺口：护盾持续时间、破碎触发与爆炸效果

## 正史对照（女主专属被动）

医疗专精 Medical Expertise
- 正史要点：治疗量与治疗范围提升；满级10秒免疫致死
- 代码现状：已通过 SetByCaller 注入治疗倍率；医疗站根据标签扩展范围并附加免疫致死
- 缺口：其他治疗来源的范围/免疫致死策略仍需补齐

智能辅助 Smart Assist
- 正史要点：无人机 HP/伤害/治疗提升、持续时间+5秒、智能模式、死亡爆炸（150伤害+2秒眩晕）
- 代码现状：存在被动 GA 占位，未见无人机 Actor/AI 完整实现
- 缺口：无人机系统与死亡爆炸逻辑
- 缺少 CombatSystem 统一状态与验收清单
- 某些技能的控制/表现仅停留在建议层面

## 关键决策
- 本项目不引入 Lyra 的 WeaponStateComponent（见 `Docs/DevelopmentNotes/CombatSystem_WeaponStateComponent_决策记录.md`）
- 命中反馈不做 Lyra 级别严格校验，保持 TargetData 回传 + CommitAbility/ApplyCost 服务器权威

## 输出
- 形成可执行补齐清单与优先级建议
