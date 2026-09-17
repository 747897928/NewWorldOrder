# Skills Quick Reference

版本: 1.1 | 最后更新: 2026-08-15（仅路径校正）

用途: AI日常开发快速查询（完整设计见 ../SystemDesign/GameDesign/NumericalDesign/Skills/）

---

## 技能系统核心

伤害占比: 武器80% + 技能20%
技能点数: 29点（Lv1-30，每级1点）
里程碑: Lv5/10/15/20/25（解锁新技能槽位）

---

## 男主技能（陈浩宇）

详见: ../SystemDesign/GameDesign/NumericalDesign/Skills/MaleSkills_男主技能.md（SSOT）

基础技能（Q/E/C）:
- Q: 战术标记 - 目标+25%伤害，弱点+10%，持续8s/CD15s
- E: 震撼手雷 - 半径5m眩晕3s，伤害50，CD20s
- C: 战术闪避 - 无敌0.5s，移速+50%持续1s，CD8s

可选技能（Lv10+，替换Q/E槽）:
- 自适应护甲（替换Q槽） - 受击-30%伤害持续5s，CD25s
- 处决打击（替换E槽） - 对<30%血敌人+100%伤害，CD12s
- 破甲射击（替换E槽） - 降低目标50%护甲持续10s，CD18s

大招（X槽，二选一，Lv5解锁）:
- 战术超载 - 无限弹药+射速+30%+移速+20%，持续8-12s，充能100点
- 极限火力（第7章解锁） - 射速×2+弹道归零+穿透，持续6s，充能100点

---

## 女主技能（沈芸皖）

详见: ../SystemDesign/GameDesign/NumericalDesign/Skills/FemaleSkills_女主技能.md（SSOT）

基础技能（Q/E/C）:
- Q: 战术扫描 - 半径15m显示敌人15s，弱点+10%，CD25s
- E: 救援掩护 - 隐身6s+移速+30%+救援加速×2，CD25s
- C: 紧急闪避 - 无敌0.5s，移速+50%持续1s，CD8s

可选技能（Lv10+，替换Q/E槽）:
- 护盾发生器（替换E槽） - 10m范围40% MaxHP护盾持续10s，CD35s
- 感染抑制剂（替换E槽） - 5m范围减速-40%减伤-30%持续10s，CD22s
- 战术无人机（替换Q槽） - 三模式切换（治疗/侦察/防御），持续20s，CD30s

大招（X槽，二选一，Lv5解锁）:
- 医疗站 - 半径5m范围5% MaxHP/s持续15s，充能100点
- 生命复苏（第5章解锁） - 复活队友50% HP+5s无敌，充能100点+90s CD

---

## 被动技能

详见: ../SystemDesign/GameDesign/NumericalDesign/Skills/PassiveSkills_被动技能.md（SSOT）

4个槽位，8种被动（Lv5/10/15/20解锁）
升级成本: Lv1→Lv2需3点，Lv2→Lv3需5点（单个被动最高8点）
总计29点无法满级所有被动（4×8=32点）

通用被动（男女共享）:
- 战术意识 - 暴击+5%/10%/15%（1/2/3级）
- 快速装填 - 换弹速度+15%/30%/50%
- 强化护甲 - 受到伤害-10%/20%/30%
- 生命强化 - 最大生命+50/100/150

男主专属:
- 爆破专家 - 爆炸伤害+20%/40%/60%
- 精准射击 - 弱点伤害+15%/30%/50%

女主专属:
- 医疗专精 - 治疗效果+20%/40%/60%
- 支援增幅 - 队友Buff效果+15%/30%/50%

---

## 控制效果矩阵

详见: ../SystemDesign/GameDesign/NumericalDesign/Skills/ControlMatrix_控制矩阵.md（SSOT）

| 敌人类型 | 眩晕 | 冰冻 | 击退 | 减速 |
|---------|------|------|------|------|
| 普通敌人 | 3s | 3s | 5m | 50%/3s |
| 精英敌人 | 1.5s | 1.5s | 2.5m | 30%/2s |
| Boss | 仅打断 | 仅打断 | 无效 | 20%/1s |

---

## 技能插槽绑定

详见: ../SystemDesign/GameDesign/NumericalDesign/Skills/Overview_总览.md

- Q: 主动技能1（战术标记/扫描）
- E: 主动技能2（手雷/掩护）
- C: 主动技能3（闪避，男女相同）
- X: 大招（终极技能，二选一，Lv5解锁）
- 可选技能: 替换Q/E槽位（Lv10+解锁）
- 被动槽: 4个（Lv5/10/15/20解锁）

---

## 推荐Build

男主（高爆发DPS）:
- 早期: 满战术标记（提升全队输出）
- 中期: 点出精准射击被动+破甲弹
- 后期: 火力全开大招+爆破专家Lv3

女主（支援/生存）:
- 早期: 满救援掩护（保护队友）
- 中期: 点出护盾发生器+强化护甲
- 后期: 医疗站大招+医疗专精Lv3

---

## 实现参考

GameplayTag前缀: Abilities.*, InputTag.*
技能基类: UShootGameplayAbility
技能数据表: Content/Data/Skills/DT_MaleSkills, DT_FemaleSkills
被动技能: Content/Data/Skills/DT_PassiveSkills

---

## 跨文档引用

- 男主技能SSOT: ../SystemDesign/GameDesign/NumericalDesign/Skills/MaleSkills_男主技能.md
- 女主技能SSOT: ../SystemDesign/GameDesign/NumericalDesign/Skills/FemaleSkills_女主技能.md
- 被动技能SSOT: ../SystemDesign/GameDesign/NumericalDesign/Skills/PassiveSkills_被动技能.md
- 控制矩阵SSOT: ../SystemDesign/GameDesign/NumericalDesign/Skills/ControlMatrix_控制矩阵.md
- 系统总览: ../SystemDesign/GameDesign/NumericalDesign/Skills/Overview_总览.md
