# 女主技能数值 Female Skills
文档版本: 1.1  
最后更新: 2025-11-23  
权威来源: 游戏设计完整文档 v7.0 Final.md

---
## 输入与标签
- 槽位: Q/E/C/X → InputTag.Q / InputTag.E / InputTag.C / InputTag.X
- 被动: InputTag.Passive.1 / InputTag.Passive.2
- 技能类型标签: Ability.Type.Skill.Primary/Secondary/Utility/Ultimate/Passive1/Passive2
- 技能资产标签:
  - Abilities.Female.SkillQ.TacticalScan
  - Abilities.Female.SkillE.RescueCloak
  - Abilities.Female.SkillC.RapidCharge
  - Abilities.Female.Ultimate.MedicalStation
  - Abilities.Female.Passive.MedicalExpertise
  - Abilities.Female.Passive.SmartAssist

---
## Q 战术扫描 Tactical Scan
- CD 22s，范围15m，持续15s；尸潮红利（≥10敌）持续×2→30s。
- 效果：穿墙标记所有敌人；弱点高亮；弱点伤害+10%。
- 用途：侦察、狙击增伤、尸潮信息优势。

## E 救援掩护 Rescue Cloak
- CD 30s；E 同时是交互键（复活/拾取）。
- 效果：女主隐身6s，移速+30%，救援速度×2；成功救起队友时队友回复50%HP并获得3s无敌。攻击会打破隐身。
- 用途：高风险救援、脱战、绕后或隐身取物。

## C 疾速充能 Rapid Charge
- CD 18s；基础移速+50%，持续4s，期间射击无移速惩罚；每次击杀刷新持续+4s（无上限）。
- Lv2：移速加成为+60%；Lv3：换弹时间-50%，击杀延长2s（但刷新无上限）。
- 用途：机动输出、追击、边打边走；与高射速/连杀流配合。

## X 医疗站 Medical Station
- 充能100点，来源：治疗队友+0.8点/1%HP，技能使用+5点，队友受伤+3点/100伤害，时间+2点/10s，开局+50点。
- 效果：生成医疗站Actor，半径5m，持续15s；每秒治疗范围内队友5% MaxHP（共75% MaxHP）。
- 目标：每副本可使用2-3次；支撑Boss战/防守点。

## Passive1 医疗专精 Medical Expertise
- Lv1：治疗量+20%。
- Lv2：治疗量+40%，治疗范围+30%。
- Lv3：治疗量+60%，范围+50%，受治疗者10s免死（血量不低于1）。
- 用途：医疗站/急救技能强化，抗压救援。

## Passive2 智能辅助 Smart Assist
- 面向无人机类技能（后续扩展）。
- Lv1：无人机HP+50。
- Lv2：再+50HP，伤害/治疗+50%。
- Lv3：再+50HP，伤害/治疗+50%，持续+5s，智能模式（自动治疗/侦查/防御），死亡爆炸150伤害+2s眩晕（3m）。
- 用途：辅助/自动化玩法，配合无人机可选技能。

---
## 切换/授予规范
- Female kit tag: Abilities.Kit.Protagonist.Female；切换角色时移除 Male kit 并授予 Female kit。
- 必须附带 Ability.Type.Skill.* 与 Abilities.Female.* 标签，便于 ASC 按套件清理与 UI 绑定。

---
## Phase1 C++ 实现状态（当前 Reality）

- Q TacticalScan
  - 已有：服务器端 Overlap 扫描（ECC_Pawn），对敌方应用 Status.Marked/Status.Vulnerable，并应用 ShootEffect_Vulnerable（Duration 覆盖为技能 Duration）。
  - 已有：到期移除目标上的 Status 标签（由 GA 自身计时触发 EndAbility 统一清理）。
  - TODO(主角-TacticalScan-GroupBonus): “尸潮红利（≥10敌）持续×2” 规则尚未实现。
  - TODO(主角-TacticalScan-UI): 弱点高亮/穿墙提示等表现层依赖 Cue/UI 系统补完。

- E RescueCloak
  - 已有：施放时对自身应用 MoveSpeed Buff（Duration=CloakDuration），并添加 Status.Cloaked（到期/取消移除）。
  - TODO(主角-RescueCloak-Revive): 救援速度×2、救起后 3s 无敌/回血等交互逻辑尚未接入。
  - TODO(主角-RescueCloak-BreakOnAttack): “攻击打破隐身” 尚未实现（需要武器开火/伤害事件驱动）。

- C RapidCharge
  - 已有：施放后进入 Buff 窗口（MoveSpeed/ReloadSpeed），持续 BuffDuration；窗口内监听击杀事件刷新 Buff 与持续时间。
  - TODO(主角-RapidCharge-NoMovePenalty): “期间射击无移速惩罚” 尚未实现（需要与射击移速惩罚管线对接）。
  - TODO(主角-RapidCharge-LevelScaling): Lv2/Lv3 数值分段与精英击杀奖励尚未接入。

- X MedicalStation
  - 已有：服务器 Spawn 医疗站 Actor（AShootSkillMedicalStation），周期治疗范围内友军（按 TeamId 过滤）。
  - 已有：默认治疗通过 UShootEffect_HealInstant + SetByCaller.Heal 应用到 Health（每秒按 MaxHP 百分比计算）。
  - TODO(主角-MedicalStation-Scale): 治疗量随队伍/等级缩放、死亡保护等设计尚未实现。

- Passive1 MedicalExpertise
  - 已有：授予后自动激活一次，给自身应用长期 GE（ShootEffect_Passive_MedicalExpertise），并带 Abilities.Kit.Protagonist.Female Tag 便于切换移除。
  - TODO(主角-MedicalExpertise-Design): “不死窗口”“治疗范围提升”等高级规则尚未实现。

- Passive2 SmartAssist
  - 已有：授予后自动激活一次，给自身应用长期 GE（ShootEffect_Passive_SmartAssist），并带 Abilities.Kit.Protagonist.Female Tag 便于切换移除。
  - TODO(主角-SmartAssist-DroneAI): 无人机 Actor/AI 行为留到后续 Phase。
