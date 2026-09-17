# 文档结构与阅读策略

## 目标

- 保持单一数据源
- 让新会话能快速定位任务与现状
- 避免重复文档与冲突逻辑

## 当前文档层级

```
Docs/
- SystemDesign/            设计正史与系统规则
  - GameDesign/
  - GameFlow/
  - UI/
- Tasks/                   任务工作区（执行入口）
- DevelopmentNotes/        技术笔记与会话记录
- QuickReference/          快速索引
- Engineering/             规范与维护规则
- SessionGuides/           会话启动指南
- Archives/                归档（只读参考）
- GASDocumentation_Chinese/ 只读参考
- ExampleProjectCode/        只读参考
```

## 阅读顺序

1. AGENTS.md
2. SessionGuides/Implementation_代码实现指南.md
3. Tasks 下对应任务包
4. QuickReference 相关条目
5. SystemDesign/GameDesign 正史

## 冲突处理

- 正史与任务包冲突：以 Source/NewWorldOrder 当前实现为准，记录差异
- 实现与正史冲突：登记为缺口，进入任务清单

## 维护原则

- 更新内容时直接替换旧逻辑
- 需要保留历史时移入 Docs/Archives
- 不在同一文档中同时保留旧错误逻辑与新正确逻辑
