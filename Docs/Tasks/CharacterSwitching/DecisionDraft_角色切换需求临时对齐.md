---
doc_id: Task-CharacterSwitching-DecisionDraft
title: 角色切换需求临时对齐
version: 0.1
status: Draft
last_updated: 2026-03-21
author: Codex
related_docs:
  - Docs/Tasks/CharacterSwitching/Requirements_需求.md
  - Docs/Tasks/CharacterSwitching/Overview_总览.md
  - Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md
---

# 角色切换需求临时对齐

## 文档目的
- 本文档只记录 2026-03-21 当前会话已经对齐、但尚未正式回写到权威需求文档的临时结论。
- 这里的内容用于防止后续继续围绕旧假设反复讨论。
- 在用户明确说“最终需求确认”之前，本文档不替代 `Requirements_需求.md`、`Overview_总览.md` 和 `Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md`。

## 当前已对齐结论

### 入口优先级
- 主入口：主菜单长按切换角色。
- 次入口：世界里的切换点 / 交互物，点击交互后直接切换角色。
- 当前不采用“靠近另一位主角 NPC 后直接切换”作为主线方案。

### 主菜单入口
- 参考《刺客信条：影》。
- 这是当前操控主角的切换入口，本质更接近单选切换。
- 当前只会操控男主或女主其中一个，不存在同时操控两个主角。
- 主菜单入口继续沿用已经落地的 `CommonUI + Enhanced Input` 长按方案。
- 当前已确认的输入资产方向：
  - `IA_SwitchCharacter`
  - `IMC_CharacterSwitchMenu`

### 世界入口
- 世界入口继续保留，但只作为补充入口，不再作为最终主入口。
- 当前世界入口体验定为：
  - 玩家走到切换点 / 交互物附近
  - 出现普通交互提示
  - 点击交互后直接切换
- 当前不要求世界入口做长按。
- 当前也不要求世界入口绑定到“另一位主角 NPC”。

### 关于“靠近另一位主角就切换”
- 当前不把它作为主线设计。
- 原因：
  - 入口条件复杂
  - 角色摆位、寻路、跟随、待机要配合
  - 未来若要把另一位主角做成同伴互动对象，这条入口会和其他互动抢位置
- 因此当前阶段不围绕这条体验继续扩实现。

### 切换限制
- 当前临时结论：去掉“只能在 Hub 切换角色”的硬限制。
- 原因：
  - 开发阶段限制过死，会妨碍主流程验证
  - 未来不排除特殊场景下也允许切换
  - 当前真正担心的是战斗中切换会带来状态同步、行为中断、表现链异常等问题
- 当前处理原则：
  - 先不把切换限制写死到 Hub
  - 战斗中禁止切换这类规则先保留为后续待实现项
  - 等主流程完整可用后，再按真实问题收口限制条件

### 未来扩展
- 另一位主角 NPC 的长期定位不是“切换按钮”。
- 未来更适合扩成“同伴交互对象”。
- 后续可能扩的交互方向：
  - 对话
  - 邀请跳舞
  - 拍照 / Pose
  - 接吻
  - 其他亲密互动或同伴互动
- 当前阶段只记录这个方向，不进入本轮实现和验收。

## 当前实现建议
- 菜单入口继续作为主入口推进。
- 世界入口继续保留交互物 / 切换点方案，用于补充切换与开发期验证。
- 与“另一位主角 NPC”直接切换相关的设计，当前不作为交付目标推进。
- 代码层面允许为未来“同伴交互对象”留接口，但当前实现不要提前把同伴玩法混进角色切换主线。

## 待正式回写的文档
- `Docs/Tasks/CharacterSwitching/Requirements_需求.md`
- `Docs/Tasks/CharacterSwitching/Overview_总览.md`
- `Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md`

## 回写条件
- 等用户明确说“最终需求确认”后，再把本文档结论正式并入权威文档。
- 正式回写时，需要同步清理与本文档结论冲突的旧表述，避免临时结论和权威需求长期并存。

## 生命周期
- 在最终需求尚未确认前，本文档继续保留。
- 一旦本文档中的结论已经完整回写到以下文档：
  - `Docs/Tasks/CharacterSwitching/Requirements_需求.md`
  - `Docs/Tasks/CharacterSwitching/Overview_总览.md`
  - `Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md`
- 且会话记录已经保留这次决策背景后，本文档应删除，避免长期堆积临时文件。
