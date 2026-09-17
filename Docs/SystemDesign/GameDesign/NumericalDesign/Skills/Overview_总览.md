# 技能系统总览 Skill System Overview
文档版本: 1.1  
最后更新: 2025-11-23  
单一数据源标记: SystemDesign/GameDesign/NumericalDesign/Skills/
权威来源: 游戏设计完整文档 v7.0 Final.md

---
## 输入与标签
- 主动槽位: Q/E/C/X → InputTag.Q / InputTag.E / InputTag.C / InputTag.X
- 被动槽位: InputTag.Passive.1 / InputTag.Passive.2
- 武器/交互: InputTag.LMB / InputTag.RMB / InputTag.Ability.Interact（保持不变）
- 能力类型标签: Ability.Type.Skill.Primary/Secondary/Utility/Ultimate/Passive1/Passive2
- 性别套件标签: Abilities.Kit.Protagonist.Male / Abilities.Kit.Protagonist.Female（切换角色时成组授予/移除）
- 技能资产标签: 见男女主技能一览中的 Abilities.* 具体名称

---
## 男主（陈浩宇）技能摘要
- Q 战术突击 `Abilities.Male.SkillQ.TacticalAssault`（InputTag.Q，Primary）  
  冲刺5m，0.3s无敌；冲刺后射速+30%移速+20%，持续3s（Lv3 5s）；满级撞击50伤害+1s眩晕。CD 15s，Lv2距离7m。
- E 震撼手雷 `Abilities.Male.SkillE.ShockGrenade`（InputTag.E，Secondary）  
  150伤害，3m范围（尸潮≥10敌人→4.5m）；普通敌人2s眩晕/特殊0.5s硬直/精英打断，易伤+30%持续5s。CD 30s，受战术专家被动影响。
- C 钢铁壁垒 `Abilities.Male.SkillC.SteelBulwark`（InputTag.C，Utility）  
  能量护盾5s，正面-80%远程伤害，移速-30%，可再按取消；Lv2 7s，Lv3反伤20%，满级破碎爆炸3m 100伤害。CD 20s。
- X 战术超载 `Abilities.Male.Ultimate.TacticalOverload`（InputTag.X，Ultimate）  
  充能100点（击杀/受伤/技能命中/时间自然充能；开局50点）；持续8s，击杀+1s上限+4s；无限弹药，射速+30%，移速+20%，换弹+100%（即瞬换）。
- 被动1 标记猎手 `Abilities.Male.Passive.MarkHunter`（Passive1）  
  Lv1 击杀HP>200敌人回血6%；Lv2 8%；Lv3 同时普通敌人4%，标记传播+50%。
- 被动2 装甲强化 `Abilities.Male.Passive.ArmorEnhancement`（Passive2）  
  Lv1 任意护盾+20%；Lv2 持续+3s；Lv3 护盾破碎爆炸（50%护盾值转3m伤害）。

---
## 女主（沈芸皖）技能摘要
- Q 战术扫描 `Abilities.Female.SkillQ.TacticalScan`（InputTag.Q，Primary）  
  15m范围标记敌人15s（穿墙可见），弱点伤害+10%，尸潮≥10敌人持续×2。CD 22s。
- E 救援掩护 `Abilities.Female.SkillE.RescueCloak`（InputTag.E，Secondary）  
  隐身6s（仅女主），移速+30%，救援速度×2；复活成功队友50%HP+3s无敌。CD 30s，E 同时是交互键。
- C 疾速充能 `Abilities.Female.SkillC.RapidCharge`（InputTag.C，Utility）  
  自身移速+50%持续4s，射击不降速；击杀刷新持续+4s（无上限）；Lv2 移速+60%，Lv3 换弹-50%且击杀延长2s无上限。CD 18s。
- X 医疗站 `Abilities.Female.Ultimate.MedicalStation`（InputTag.X，Ultimate）  
  充能100点（治疗队友/技能使用/队友受伤/自然充能，开局50点）；生成医疗站，5m范围每秒治疗5% MaxHP，持续15s。设计目标每副本2-3次。
- 被动1 医疗专精 `Abilities.Female.Passive.MedicalExpertise`（Passive1）  
  Lv1 治疗量+20%；Lv2 治疗量+40%范围+30%；Lv3 治疗量+60%范围+50%，受治疗者10s免死（血量不低于1）。
- 被动2 智能辅助 `Abilities.Female.Passive.SmartAssist`（Passive2）  
  面向无人机系技能：Lv1 无人机HP+50；Lv2 伤害/治疗+50%；Lv3 再+50HP、+50%效果、持续+5s、智能模式、自爆150伤害+2s眩晕3m。

---
## 兼容性与切换规则
- 技能授予：AShootPlayerState::ApplyGenderAbilityKit 调用 UShootAbilitySystemComponent::GrantAbilitiesWithKit，按性别套件 tag 成组授予/移除；被动授予时立即激活。
- 输入约束：武器开火/换弹保持 InputTag.LMB/R，交互保持 InputTag.Ability.Interact；技能槽仅使用 Q/E/C/X，不再使用数字输入标签。
- 标签对应：各技能需携带 Ability.Type.Skill.* 与 Abilities.* 标签，便于角色切换时清理。

