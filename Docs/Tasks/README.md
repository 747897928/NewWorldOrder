# Tasks 任务工作区

## 概念

Tasks是任务专用工作区，类似CPU-内存-磁盘模型：
- 详细版文档（SystemDesign/GameDesign/等）= 磁盘（完整知识）
- 任务工作区（Tasks/TaskName/）= 内存（任务专用，Token优化）
- AI执行任务 = CPU（只读内存，高效运作）

## 目的

1. Token优化

详细版文档完整但Token成本高（5000+），AI开发时只读工作区（800-1000 tokens），节省80%。

2. 任务专注

工作区只包含任务相关内容，避免AI被海量信息淹没。

3. 协作桥梁

工作区提交和推送到git远程仓库，其他AI可接手继续，不需重新准备上下文。

## 工作流程

```
1. 任务开始
   → AI创建Tasks/TaskName/

2. 准备工作区
   → 从SystemDesign/GameDesign/等查询任务相关部分
   → 提取并精简到工作区
   → 创建STATUS.md

3. 开发执行
   → AI只读工作区内容（Token优化）
   → 专注当前任务

4. 发现缺失
   → 回详细版查询
   → 补充到工作区

5. 任务完成
   → 更新STATUS.md为completed
   → 更新详细版文档（如有设计变更）
   → 提炼经验到Notes/（如有新知识）
   → 工作区提交git

6. 协作接手
   → 其他AI读工作区继续
   → 可修改工作区（如发现错误）
```

## 工作区结构

```
Tasks/TaskName/
├── STATUS.md                  # 任务状态（强制）
├── Context_上下文.md           # 任务背景和目标
├── Requirements_需求.md        # 具体需求
├── DesignSpec_设计规格.md      # 设计方案
├── Implementation_实现指南.md  # 实现步骤
├── Checklist_检查清单.md       # 完成检查
└── References/                # 从详细版提取的参考
    ├── GameDesign_XXX.md
    └── NumericalDesign_XXX.md
```

## STATUS.md格式（强制）

```markdown
---
task_id: TaskName
status: pending / in_progress / completed / blocked
assigned_to: Claude / Codex / User
progress: 0-100%
started: YYYY-MM-DD
last_updated: YYYY-MM-DD
---

# 任务状态

当前状态：in_progress
负责人：Claude
进度：50%

最后更新内容：
- 完成了XXX
- 正在进行XXX

下一步：
- 需要完成XXX

遇到问题：
- 无

相关Commit：
- abc123: 实现了XXX功能
```

## 创建工作区

步骤：
1. 在Tasks/创建任务文件夹
2. 创建STATUS.md并设置status=in_progress
3. 从详细版查询并提取相关内容到工作区
4. git add、commit、push

示例：
```bash
mkdir -p Tasks/HomeSystemDesign
# 创建STATUS.md
# 从GameDesign/Systems/提取家园系统设计
# 从GameDesign/UI_UX/提取家园界面设计
# 精简到工作区
git add Tasks/HomeSystemDesign
git commit -m "task: 创建家园系统设计工作区"
git push
```

## 完成任务

强制Checklist：
```
- [ ] 功能已实现（或设计已完成）
- [ ] 代码已提交（commit hash: XXX）或设计文档已更新
- [ ] STATUS.md更新为completed
- [ ] 相关详细版文档已更新（如有变更）
- [ ] 新知识已提炼到Notes/（如有）
- [ ] 工作区已提交git
- [ ] 完成报告已输出给用户
```

完成报告模板：
```markdown
# 任务完成报告

任务：TaskName
状态：completed
完成度：100%

主要成果：
- 成果1
- 成果2

代码变更（如有）：
- commit: abc123
- 文件：X个新增，Y个修改

文档更新：
- [x] Tasks/TaskName/STATUS.md
- [x] SystemDesign/GameDesign/XXX.md
- [x] Notes/XXX/新增笔记

遗留问题：
- 无

下一步：
- 可以开始XXX
```

违反后果：
- 任务视为未完成
- 其他AI可能重复开发
- 浪费团队时间

## 工作区保留

完成的工作区不归档，保留在Tasks/供参考：
- 作为任务模板
- 作为历史记录
- 供未来类似任务参考

返工时复用旧工作区：
- 读取旧工作区
- 更新STATUS.md
- 继续开发

## 示例

参考：
- Tasks/CharacterSwitching/（角色切换任务）

这是完整的任务工作区示例，包含所有必需文档。

## 注意事项

1. 工作区不是临时的

虽然叫"工作区"，但必须提交git，不能认为"完成后就删除"。

2. 工作区可被修改

后续AI接手时，如发现前人错误，可以修改工作区重来。

3. 工作区是协作桥梁

不提交工作区 = 其他AI无法接手 = 协作断裂。

4. STATUS.md是关键

不更新STATUS = 其他人不知道任务状态 = 可能重复劳动。

最后更新：2025-11-06
