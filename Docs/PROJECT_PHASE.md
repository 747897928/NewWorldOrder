# 项目阶段管理

当前阶段：Phase 1 - 项目建立期
开始日期：2025-11-04
状态：进行中

注意：本文件阶段信息可能滞后；各系统最新进度以 Docs/Tasks/<System>/STATUS.md 与 git log 为准。

---

## 什么是Phase（项目阶段）

Phase是项目的一个相对独立的开发周期，通常对应一个大的里程碑或重大变更。

Phase切换场景：
- 大规模重写剧情（推倒重来）
- 架构重构（如从Lyra迁移到自研框架）
- 核心设定变更（如世界观推倒重来）
- 从原型阶段进入正式开发阶段

Phase切换效果：
- 清空CHANGELOG.md，从干净状态开始
- 旧Phase的所有CHANGELOG（Active+Recent）归档到Archive
- 在Archive中创建Phase分隔
- 更新本文件（PROJECT_PHASE.md）

---

## 当前阶段详情

### Phase 1: 项目建立期

时间：2025-11-04至今

目标：
- 建立技术框架和核心系统
- 确立世界观和核心设定
- 建立文档管理体系

主要产出：
- GAS架构建立
- 武器系统实现
- 角色切换功能
- 6个核心设定文档
- CHANGELOG三层结构
- ADR机制
- Phase管理机制

下一阶段预期：
- Phase 2可能是"剧情迭代期"或"大规模重写"
- 取决于用户对当前设定的满意度

---

## 历史阶段

当前无历史Phase（Phase 1是第一个阶段）

---

## Phase切换操作指南

### 何时触发Phase切换

AI检测到以下情况时，主动询问用户是否需要Phase切换：

1. 用户明确表示"重写剧情"、"推倒重来"、"大改"
2. 检测到大量设定被删除（>5个重要设定）
3. CHANGELOG累积过多（>50条）且用户表示"太乱了"
4. 用户明确说"开始新阶段"、"Phase切换"

### 用户确认前的询问模板

```
检测到可能需要Phase切换（项目阶段切换）。

当前阶段：Phase 1 - 项目建立期
建议新阶段：Phase 2 - [用户意图]

Phase切换效果：
- 清空当前CHANGELOG.md，从干净状态开始
- 旧Phase的所有变更记录归档到Archive
- 适用场景：大规模重写、推倒重来、新阶段开始

Phase切换影响：
- 历史变更记录移到Archive（仍可查阅）
- AI不再被旧变更干扰
- 适合"重新开始"的场景

是否执行Phase切换？（是/否）
如果是，请说明新Phase的名称和目标。
```

### Phase切换执行步骤

用户确认后，AI按以下步骤执行：

1. 读取当前CHANGELOG.md和CHANGELOG_Recent.md的所有内容

2. 在CHANGELOG_Archive.md中添加Phase分隔：
   ```markdown
   ## Phase 1: [阶段名称]（[开始日期]至[结束日期]）

   ### 核心里程碑
   - [里程碑1]
   - [里程碑2]

   ### 变更摘要
   [合并所有Active和Recent的变更，保留关键信息]

   详细记录：见Archives/CHANGELOG_Phase1_Detailed.md
   ```

3. 将CHANGELOG.md和CHANGELOG_Recent.md的完整内容复制到：
   ```
   Archives/CHANGELOG_Phase1_Detailed.md
   ```

4. 清空CHANGELOG.md，写入新Phase的初始内容：
   ```markdown
   # 剧情设定变更日志（当前活跃）

   本文档只保留最近10条活跃变更。AI开始工作前必须阅读。

   ---

   ## [YYYY-MM-DD] Phase 2开始

   状态：已应用
   影响范围：整个项目
   变更内容：Phase 1结束，Phase 2开始

   Phase 1摘要：
   [核心里程碑]

   Phase 2目标：
   [新Phase的目标]

   历史变更：
   Phase 1的详细变更记录已归档到：
   - Docs/Archives/（归档摘要，按实际文件名）
   - Docs/Archives/（归档详细记录，按实际文件名）

   AI注意事项：
   - 从干净状态开始
   - 不再关注Phase 1的历史变更
   - 专注于Phase 2的目标
   ```

5. 清空CHANGELOG_Recent.md

6. 更新PROJECT_PHASE.md（本文件）：
   - 更新"当前阶段"
   - 将旧Phase移到"历史阶段"

7. git commit -m "Phase切换：Phase 1 → Phase 2"

8. 向用户汇报：
   ```
   Phase切换完成

   旧阶段：Phase 1 - 项目建立期（2025-11-04至2025-11-XX）
   新阶段：Phase 2 - [新Phase名称]

   归档情况：
   - Phase 1的X条变更记录已归档
   - 详细记录：Docs/Archives/（按实际文件名）
   - 摘要记录：Docs/Archives/（按实际文件名）

   当前状态：
   - CHANGELOG.md已清空，从Phase 2开始
   - AI不再被Phase 1的历史变更干扰
   - 可以"重新开始"创作或开发

   如需查阅Phase 1历史，请访问Archives/
   ```

---

## AI工作指引

### 正常工作时

- 当前Phase的CHANGELOG.md是你的主要参考
- 不需要关注Archives中的历史Phase
- 专注于当前Phase的目标和变更

### Phase切换时

- 主动检测用户意图（重写、推倒重来）
- 询问用户是否需要Phase切换
- 用户确认后按流程执行
- 确保归档完整，不丢失信息

### 跨Phase查询时

- 如果用户明确要求了解历史（如"Phase 1做了什么"）
- 读取Archives/CHANGELOG_PhaseX_Detailed.md
- 或读取CHANGELOG_Archive.md中的摘要

---

## 注意事项

Phase切换是重大操作：
- 适合"重新开始"的场景
- 不适合小规模修改
- 必须用户明确确认
- AI不能自作主张执行

Phase切换后：
- AI专注度提升（不被历史干扰）
- 但历史记录仍可查阅（在Archives）
- 适合大规模重写剧情的场景

---

最后更新：2025-11-08
