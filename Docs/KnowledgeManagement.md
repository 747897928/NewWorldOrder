# 新秩序项目 - 知识管理体系

更新时间：2026-01-30

## 核心原则

- 正史需求以 Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md 为准（文件内容为 v7.2）
- 代码事实以 Source/NewWorldOrder 为准
- 任务工作区以 Docs/Tasks 为准，若与正史冲突需记录缺口
- 文档保持干净，旧逻辑不与新逻辑并存
- Docs/GASDocumentation_Chinese 与 Docs/ExampleProjectCode 为只读参考

## 文档结构

```
Docs/
- SystemDesign/
  - GameDesign/        正史设计与数值
  - GameFlow/          流程与关卡结构
  - UI/                UI 设计
- Tasks/               任务工作区（当前执行入口）
- DevelopmentNotes/    技术笔记与会话记录
- QuickReference/      快速索引
- Engineering/         规范、约束、维护规则
- SessionGuides/       会话启动指南
- Archives/            归档（只读参考）
- GASDocumentation_Chinese/  只读参考
- ExampleProjectCode/        只读参考
```

## 冲突处理规则

- 正史与任务包冲突：优先查 Source/NewWorldOrder 当前实现，记录差异
- 实现与正史冲突：登记为缺口，进入任务清单

## 维护规则

- 更新知识时直接修改原文档
- 如需保留旧版本，移动到 Docs/Archives 并在索引中说明
- 不在同一文档同时保留旧错误逻辑与新正确逻辑
- 更新后同步修改索引

## 会话工作流

1. 读取 AGENTS.md 与 SessionGuides/Implementation_代码实现指南.md
2. 查看 Tasks 下是否存在对应任务包
3. 如无任务包，先创建 Tasks/TaskName/ 并填写 STATUS
4. 以 Source/NewWorldOrder 为准核对现状
5. 更新文档并推送
