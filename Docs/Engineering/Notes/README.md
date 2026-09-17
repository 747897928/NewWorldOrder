# 技术笔记 Technical Notes

## 目的

提炼和积累可复用的技术知识，建立专家顾问团队模式。

与开发日志的区别：
- 技术笔记：知识点浓缩，结构化，可独立理解，高复用
- 开发日志：详细的时间线记录，包含完整排查过程

## 专家团队分类

```
Notes/
├── UE5/          # UE5引擎专家
├── GAS/          # GAS系统专家
├── Network/      # 网络同步专家
├── Mutable/      # 换装系统专家
├── Weapons/      # 武器系统专家
└── Patterns/     # 设计模式与最佳实践
```

## AI使用流程

启动Session时：
1. 读取 Notes/INDEX.md（总导航，约100-200行）
2. 根据任务类型定位相关分类
3. 读取对应分类的笔记（按需，每个约100-300行）

遇到问题时：
1. 先查 Notes/INDEX.md 定位相关笔记
2. 读取对应笔记获取解决方案
3. 如需背景信息，再查DevelopmentLogs/

## 笔记模板

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
- [相关笔记2](../xxx.md)

## 来源

- DevelopmentLogs/YYYY-MM/XXX.md
- 官方文档链接
- 用户提供资料
```

## 笔记命名规范

使用描述性命名：
- SaveGame_序列化规范.md
- SoftObjectPtr_软引用最佳实践.md
- ASC_生命周期.md
- TargetData_预测流程.md

避免：
- 笔记1.md
- 临时.md

## 笔记状态管理

Active：当前有效的知识
- 正常使用

Deprecated：已废弃的知识
- 在文档顶部添加废弃标记
- 说明为什么废弃
- 链接到替代的新笔记
- 不删除，保留错误学习过程

## 笔记更新流程

发现新知识：
1. 创建笔记到对应分类
2. 更新INDEX.md
3. 提交Git

发现笔记错误：
1. 标记旧笔记为Deprecated
2. 创建新笔记
3. 更新INDEX.md
4. 提交Git

## Token优化效果

对比：
- 旧模式：读完整DevelopmentLogs（5000+ tokens）
- 新模式：读INDEX + 相关笔记2-3个（约600 tokens）
- 节省：约88%

## 索引

查看 INDEX.md 获取所有笔记列表
