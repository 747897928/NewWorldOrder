# 新秩序项目 - 知识管理体系

更新时间：2025-11-06

## 核心理念

知识资产沉淀：每次AI会话都必须产出可持久化的知识资产，避免重复劳动。

## 知识管理架构

```
知识管理体系
├── 会话启动（新session如何快速上手）
│   ├── SessionGuides/Narrative_剧情创作指南.md
│   └── SessionGuides/Implementation_代码实现指南.md
│
├── 设定变更追踪（剧情设定如何版本管理）
│   └── SystemDesign/Narrative/CHANGELOG.md
│
├── 技术笔记系统（知识提炼，Token优化）
│   ├── Engineering/Notes/README.md - 笔记规范
│   ├── Engineering/Notes/INDEX.md - 笔记索引（AI必读）
│   ├── Engineering/Notes/UE5/ - UE5专家
│   ├── Engineering/Notes/GAS/ - GAS专家
│   ├── Engineering/Notes/Network/ - 网络同步专家
│   ├── Engineering/Notes/Mutable/ - 换装专家
│   ├── Engineering/Notes/Weapons/ - 武器专家
│   └── Engineering/Notes/Patterns/ - 设计模式与最佳实践
│
├── 开发日志系统（详细记录，历史审计）
│   ├── Engineering/DevelopmentLogs/README.md - 日志规范
│   ├── Engineering/DevelopmentLogs/INDEX.md - 日志索引
│   └── Engineering/DevelopmentLogs/YYYY-MM/ - 按月归档
│
└── 设计文档（SSOT单一数据源）
    ├── SystemDesign/Narrative/ - 剧情与世界观
    ├── SystemDesign/Attributes/ - 属性系统
    ├── SystemDesign/Skills/ - 技能系统
    └── SystemDesign/Weapons/ - 武器系统
```

## 问题场景与解决方案

### 场景1：剧情设定变更

问题：你修改了龙旗首领的设定，但AI还在用旧设定写剧情

解决方案：
1. 你修改 Factions_势力设定.md 中的内容
2. 在 CHANGELOG.md 中记录变更
3. 下次AI启动时会先读CHANGELOG，了解到变更
4. AI基于新设定重新推演剧情

工作流程：
```
用户修改设定
→ 更新设定文档
→ 记录到CHANGELOG
→ 新session读取CHANGELOG
→ AI感知变更，使用新设定
```

### 场景2：技术知识积累

问题：AI学到了GAS的用法，但下次session又忘了，你需要重复解释

解决方案：
1. AI完成复杂问题排查后，记录详细过程到 Engineering/DevelopmentLogs/YYYY-MM/
2. 从日志中提炼核心知识点到 Engineering/Notes/GAS/
3. 使用笔记模板，标注状态（Active/Deprecated）
4. 更新Notes/INDEX.md索引
5. 下次session启动时，只读INDEX.md + 相关笔记（Token优化88%）

工作流程：
```
排查问题 + 解决
→ 记录详细日志到DevelopmentLogs/
→ 提炼知识点到Notes/对应分类
→ 更新Notes/INDEX.md
→ 提交到git
→ 新session读INDEX.md定位笔记
→ 快速获取知识，解决问题
```

### 场景3：踩坑与纠错

问题：AI实现了错误的方案，你指出错误并提供正确做法，但下次session又犯同样错误

解决方案：
1. AI踩坑时，记录完整排查过程到 Engineering/DevelopmentLogs/YYYY-MM/
2. 在日志中说明错误现象、原因、解决过程
3. 提炼正确做法到 Engineering/Notes/对应分类/
4. 在笔记中明确"常见错误"和"正确做法"
5. 如果替代了旧方案，在旧笔记顶部标记status: Deprecated

工作流程：
```
AI犯错
→ 记录详细排查日志到DevelopmentLogs/
→ 用户纠正
→ 提炼正确方案到Notes/
→ 更新INDEX.md
→ 新session读到正确笔记，避免重蹈覆辙
```

### 场景4：知识过时或错误

问题：AI之前记录的笔记是错的，但新session还在用

解决方案：
1. 发现笔记错误时，不要删除
2. 修改YAML元数据：status: Deprecated
3. 在笔记顶部说明为什么废弃
4. 创建新笔记，链接到旧笔记
5. 更新Notes/INDEX.md，标注旧笔记已废弃

工作流程：
```
发现笔记错误
→ 修改status: Deprecated
→ 说明错误原因
→ 创建正确的新笔记
→ 更新INDEX.md
→ 新session看到状态标记，使用新笔记
```

## AI的工作循环

### 剧情AI的工作循环

```
1. 启动：读取 Narrative_剧情创作指南.md
2. 检查变更：读取 CHANGELOG.md（优先级最高）
3. 阅读设定：读取 Worldview、Factions、Characters等
4. 执行任务：基于最新设定创作剧情
5. 落地文档：将剧情写入Chapters/或SideQuests/
6. 提交git：git add、commit、push
7. 发现设定矛盾：向用户指出，等待确认
8. 用户修改设定：帮助用户更新CHANGELOG
```

### 开发AI的工作循环

```
1. 启动：读取 Implementation_代码实现指南.md
2. 查笔记：读取 Engineering/Notes/INDEX.md（100-200行，快速定位）
3. 查已有方案：根据任务类型读取对应分类笔记（如GAS任务读Notes/GAS/）
4. 实现功能：编写代码，添加中文注释
5. 遇到复杂问题：
   - 排查 + 解决
   - 记录详细过程到DevelopmentLogs/YYYY-MM/
   - 提炼核心知识到Notes/对应分类/
6. 更新索引：更新Notes/INDEX.md
7. 落地文档：在SystemDesign中添加实现笔记
8. 提交git：git add、commit、push
```

## 笔记模板

### 技术笔记模板（Notes/）

```markdown
---
note_id: [分类]-[编号]
title: [笔记标题]
category: [UE5/GAS/Network/Mutable/Weapons/Patterns]
status: Active / Deprecated
created: YYYY-MM-DD
updated: YYYY-MM-DD
---

# 笔记标题

## 核心要点

- 要点1
- 要点2
- 要点3

## 代码示例

```cpp
// 示例代码
```

## 常见错误

错误做法：
- 错误1
- 错误2

正确做法：
- 正确1
- 正确2

## 相关笔记

- [相关笔记1](../xxx.md)

## 来源

- DevelopmentLogs/YYYY-MM/XXX.md
- 官方文档链接
```

### 开发日志模板（DevelopmentLogs/）

```markdown
# 问题/任务标题

日期：YYYY-MM-DD
负责人：用户/AI
状态：进行中/已解决/已归档

## 背景

为什么需要做这件事

## 问题描述

详细描述

## 排查过程

1. 第一步尝试
2. 第二步尝试

## 解决方案

最终方案

## 经验教训

核心知识点（已提炼到Notes/）
```

### 设定变更模板

```markdown
## [YYYY-MM-DD] 变更标题

影响范围：具体影响哪些文档或章节
变更原因：为什么要改

### 旧设定
...

### 新设定
...

### AI注意事项
基于新设定，你需要注意...
```

## 使用建议

### 作为用户（项目管理者）

1. 修改设定时
   - 更新设定文档（如Factions_势力设定.md）
   - 在CHANGELOG.md中记录变更
   - 新session的AI会自动感知

2. 提供技术资料时
   - 告诉AI："请将这个知识提炼到技术笔记"
   - AI会记录到Engineering/Notes/对应分类
   - 后续session可以直接查阅

3. 发现AI犯错时
   - 纠正AI，并要求记录排查过程和正确方案
   - AI会记录日志到DevelopmentLogs/，提炼笔记到Notes/
   - 避免后续session重复犯错

### 作为AI（新session）

1. 启动时
   - 读取SessionGuides/对应指南
   - 剧情AI必读CHANGELOG.md
   - 开发AI必读Engineering/Notes/INDEX.md（Token优化）

2. 工作时
   - 遇到问题先查Notes/INDEX.md定位相关笔记
   - 复杂问题排查后记录到DevelopmentLogs/
   - 提炼核心知识到Notes/
   - 不主动读DevelopmentLogs/（除非需要历史背景）

3. 结束时
   - 所有成果落地到文档
   - 提交到git
   - 更新必要的索引

## 知识管理原则

1. 文档是活的，不是死的
   - 可以更新、纠正、废弃
   - 使用状态标记（[可用]/[待验证]/[已废弃]）

2. 不删除旧知识，只标记废弃
   - 保留错误的学习过程
   - 说明为什么错了
   - 链接到正确的新知识

3. 知识要可索引、可搜索
   - 维护INDEX.md
   - 使用描述性文件名
   - 添加相关笔记链接

4. 所有聊天成果必须文档化
   - 剧情创作 → Chapters/或SideQuests/
   - 技术实现 → 代码 + 实现笔记
   - 复杂问题排查 → DevelopmentLogs/（详细）
   - 知识提炼 → Notes/（精炼）
   - 设定变更 → CHANGELOG.md

## 优势

相比传统的聊天记录方式：

1. 新session快速上手（读指南+索引）
2. 知识可积累、可纠正
3. 避免重复劳动
4. 设定变更可追踪
5. 错误可被记录和避免
6. 符合软件工程最佳实践

相比单一笔记方式：

1. Token优化：AI只读精炼笔记（节省88%）
2. 双轨管理：详细日志保留历史，精炼笔记快速查询
3. 专家团队模式：按分类组织，按需读取
4. 历史审计：DevelopmentLogs保留完整决策过程

这就是一个完整的知识管理体系，让AI团队像真正的开发团队一样协作。
