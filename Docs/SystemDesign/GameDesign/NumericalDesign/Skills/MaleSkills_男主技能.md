# 男主技能数值 Male Skills
文档版本: 1.1  
最后更新: 2025-11-23  
权威来源: 游戏设计完整文档 v7.0 Final.md

---
## 输入与标签
- 槽位: Q/E/C/X → InputTag.Q / InputTag.E / InputTag.C / InputTag.X
- 被动: InputTag.Passive.1 / InputTag.Passive.2
- 技能类型标签: Ability.Type.Skill.Primary/Secondary/Utility/Ultimate/Passive1/Passive2
- 技能资产标签:
  - Abilities.Male.SkillQ.TacticalAssault
  - Abilities.Male.SkillE.ShockGrenade
  - Abilities.Male.SkillC.SteelBulwark
  - Abilities.Male.Ultimate.TacticalOverload
  - Abilities.Male.Passive.MarkHunter
  - Abilities.Male.Passive.ArmorEnhancement

---
## Q 战术突击 Tactical Assault
- CD 15s，LocalPredicted dash；受战术专家被动的CDR。
- 冲刺5m，0.3s无敌帧；Lv2 冲刺7m。
- Buff：冲刺后射速+30%、移速+20%，持续3s；Lv3 持续5s。
- 满级附加：冲刺撞击敌人造成50伤害并眩晕1s。
- 战术用途：接近/脱战、躲避Boss技能、起手Buff、撞击控制。

## E 震撼手雷 Shock Grenade
- CD 30s（受战术专家CDR）；投掷爆炸，基础150伤害，范围3m；尸潮红利≥10敌人→范围4.5m。
- 控制：普通敌人2s眩晕；特殊敌人0.5s硬直；精英打断技能。
- 易伤：+30%持续5s；独立乘区。
- 战术用途：尸潮控制、精英打断、集火易伤、救援开路。

## C 钢铁壁垒 Steel Bulwark
- CD 20s，防御姿态，可再按取消。
- 基础：护盾持续5s，正面-80%远程伤害，移速-30%。
- Lv2：持续7s；Lv3：反伤20%；满级：护盾破碎时3m范围100伤害爆炸。
- 战术用途：复活掩护、抗Boss、卡点、防守反击（反伤/爆炸）。

## X 战术超载 Tactical Overload
- 充能100点，来源：击杀普通+10/精英+20，受伤+5点/100伤害，技能命中+5点，时间+2点/10s，开局+50点。
- 持续8s，击杀+1s（上限+4s）。
- 效果：无限弹药，射速+30%，移速+20%，换弹+100%（即时）。
- 目标：一局可开2-3次；重点用于尸潮清场、Boss爆发。

## Passive1 标记猎手 Marked Hunter
- 定位：击杀回血+与标记协同。
- Lv1: 击杀HP>200敌人回血6% MaxHP。
- Lv2: 回血8%。
- Lv3: 回血8%（精英）+4%（普通），标记传播+50%。
- 用途：坦克续航、精英猎杀、配合女主标记或自身震撼手雷易伤。

## Passive2 装甲强化 Armor Enhancement
- 定位：护盾增幅/破碎伤害。
- Lv1: 任意护盾容量+20%。
- Lv2: +20%容量，持续+3s。
- Lv3: 同上并护盾破碎时将50%护盾值转为3m范围伤害。
- 用途：配合钢铁壁垒/角色固有护盾，坦克/反伤流。

### ArmorEnhancement C++ Reality & GE 配置（Phase1.5）

- 实现入口：
  - `UShootGA_Passive_ArmorEnhancement` 在授予后激活一次，给自身应用长期 Buff GE。
- GE 路径（数据驱动，不再在 C++ 中改 `Spec.Data->Modifiers`）：
  - `UShootEffect_Passive_ArmorEnhancement`
  - 通过 SetByCaller 注入两个加成值：
    - `SetByCaller.ShieldCapacityBonus` → `UShootAttributeSet::ShieldCapacityBonus`（加法）
    - `SetByCaller.DamageReductionBonus` → `UShootAttributeSet::DamageReductionBonus`（加法）
  - 默认持续时间：永久（若需要有限时长，可在 GA 中通过 `BuffDuration` 覆盖）。
- 套件清理：
  - Spec 会附带 `Abilities.Kit.Protagonist.Male`（DynamicGrantedTags），便于在性别切换时按套件移除长期效果。
- 未实现：
  - 护盾破碎事件监听与爆炸/击退/减速/易伤等高级规则，后续按设计补齐。

---
## 数值校验与CDR示例
- 战术专家Lv3：普通技能CD×0.85，大招充能效率+10%。
- 示例：战术突击 15s → 12.75s；震撼手雷 30s → 25.5s。
- 大招期望：普通击杀6个+精英2个可覆盖约80-100点充能，叠加被动/自然回充可实现每关2-3次释放。

---
## 切换/授予规范
- Male kit tag: Abilities.Kit.Protagonist.Male；切换角色时移除 Female kit 并授予 Male kit。
- 必须附带 Ability.Type.Skill.* 与 Abilities.Male.* 标签，便于 ASC 按套件清理与 UI 绑定。
