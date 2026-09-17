---
task_id: CombatSystem
status: in_progress
assigned_to: Codex
progress: 98%
started: 2026-01-30
last_updated: 2026-01-31
---

# 任务状态

当前状态：in_progress
负责人：Codex
进度：98%

当前现状（代码）
- 已有伤害/治疗/易伤/快照恢复相关 GE
- 已补齐通用 Stun/Slow GE（Status.Stunned/Status.Slowed），ShockGrenade 默认使用 Stun 控制
- 已有爆炸、医疗站、盾墙等技能 Actor
- Knockback 已在 ShootSkillExplosionActor 中落地（LaunchCharacter / AddRadialImpulse）
- 已根据正史文档列出男主/女主技能与被动缺口清单（DesignSpec_设计规格.md）
- MedicalExpertise 被动已接入医疗站范围/免疫致死与治疗倍率
- RescueCloak 已接入可配置移速与攻击破隐
- RapidCharge 已接入等级移速/换弹倍率与击杀延长
- TacticalScan 已接入尸潮红利持续翻倍与弱点标记伤害+10%
- MarkHunter 击杀回血已按等级规则落地
- MarkHunter Lv3 标记传播已落地（半径加成可配置）
- RescueCloak 已接入救起后治疗与保护回调
- RescueCloak 已接入救援速度倍率（交互事件倍率）
- 救援交互能力新增救起完成事件回调
- 新增救援交互组件，并在 AShootCharacter 监听 Health<=0 时启用/恢复后关闭（倒地表现仍需补齐）
- ShieldWall 改为 SetByCaller 注入护盾与减伤数值，默认使用专用 GE
- MedicalStation HealEffect 为空时回退到通用即时治疗 GE
- 救援交互能力广播 Duration 消息，供 UI 显示进度条与剩余时间
- PlayerController 监听 Duration 消息并广播蓝图委托
- TacticalScan/MarkHunter 使用统一阵营判断入口（TeamId + Faction Tag 回退）
- 被动能力授予即激活已统一到 GrantAbilitiesWithKit(bActivatePassives=true)
- RapidCharge/MarkHunter 使用统一击杀归属入口（Killer/Instigator/Controller/Pawn）
- MultiplayerSessions 搜索兼容 SEARCH_PRESENCE 与 PRESENCESEARCH

下一步
- 对照正史数值与技能设计补齐控制/伤害缺口清单
  - 男主/女主技能与被动已完成初步缺口对照，下一步需推进实现

相关文档
- DamageAndControl_AssetsScan.md
