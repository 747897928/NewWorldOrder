# 项目文档索引

版本: 1.0
最后更新: 2025-10-31

---

## 文档组织结构

```
.docs/
├── README.md                   # 本文件 - 文档索引
├── requirements/               # 需求文档
│   └── character-switching-requirements.md
├── development/                # 开发文档
│   ├── COMMON_MISTAKES.md
│   └── CHARACTER_SWITCHING_DESIGN.md
├── sessions/                   # 会话记录
│   └── 2025-10-31-character-switching-failed.md
└── handoff/                    # 交接文档
    └── QUICK_START.md
```

---

## 快速导航

### 我是新的 AI session 或开发者

从这里开始: handoff/QUICK_START.md

这个文档会告诉你：
- 项目当前状态
- 需要阅读哪些文档
- 如何开始实现

预计阅读时间: 15-20 分钟

### 我要实现角色切换系统

阅读顺序：

1. requirements/character-switching-requirements.md (20分钟)
   - 了解功能需求
   - 了解技术约束
   - 了解现有系统

2. development/COMMON_MISTAKES.md (30分钟)
   - 了解绝对不能做的事情
   - 了解常见错误
   - 了解最佳实践

3. development/CHARACTER_SWITCHING_DESIGN.md (40分钟)
   - 了解正确的架构设计
   - 了解实现步骤
   - 了解测试策略

4. sessions/2025-10-31-character-switching-failed.md (可选，20分钟)
   - 了解之前失败的实现
   - 了解为什么会失败
   - 了解用户的反馈

总计阅读时间: 约 90-110 分钟

### 我遇到了问题

问题类型: 不知道为什么某个做法是错的
查阅: development/COMMON_MISTAKES.md

问题类型: 不知道应该怎么实现
查阅: development/CHARACTER_SWITCHING_DESIGN.md

问题类型: 不理解需求
查阅: requirements/character-switching-requirements.md

问题类型: 想了解历史背景
查阅: sessions/2025-10-31-character-switching-failed.md

---

## 文档详细说明

### requirements/ - 需求文档

#### character-switching-requirements.md

内容:
- 项目背景和目标
- 功能需求（FR1-FR5）
- 技术约束（TC1-TC4）
- 现有系统说明
- 非功能需求
- 成功标准
- 实现优先级

适用对象: 所有开发者

何时阅读: 开始实现前必读

### development/ - 开发文档

#### COMMON_MISTAKES.md

内容:
- 7 大类常见错误
- 每个错误的详细分析
- 正确做法和错误做法对比
- 检查清单

适用对象: 所有开发者（尤其是 AI）

何时阅读: 开始实现前必读

重要性: 极高（避免重复已知错误）

#### CHARACTER_SWITCHING_DESIGN.md

内容:
- 系统概述
- 架构原则
- 现有系统分析
- 设计方案
- 实现步骤（含代码）
- 常见陷阱
- 测试策略

适用对象: 实际编写代码的开发者

何时阅读: 开始编码前必读

重要性: 极高（实现指南）

### sessions/ - 会话记录

#### 2025-10-31-character-switching-failed.md

内容:
- 完整的失败实现记录
- 时间线和错误演变
- 用户的三次警告
- 根本性错误分析
- 深度反思和教训

适用对象: 想了解背景的开发者

何时阅读: 可选（但强烈推荐）

重要性: 高（理解为什么某些做法是错的）

### handoff/ - 交接文档

#### QUICK_START.md

内容:
- 5 分钟快速了解
- 项目概况
- 关键概念
- 现有系统详解
- 实现检查清单
- 常见问题
- 致命错误列表

适用对象: 新 session、新开发者

何时阅读: 刚接手项目时首先阅读

重要性: 极高（快速上下文）

---

## 阅读建议

### 情况1: 我有充足时间（2+ 小时）

推荐阅读顺序:
1. handoff/QUICK_START.md - 快速了解
2. requirements/character-switching-requirements.md - 需求
3. development/COMMON_MISTAKES.md - 禁区
4. development/CHARACTER_SWITCHING_DESIGN.md - 设计
5. sessions/2025-10-31-character-switching-failed.md - 背景

这样可以全面理解项目。

### 情况2: 我时间有限（1 小时）

必读:
1. handoff/QUICK_START.md
2. development/COMMON_MISTAKES.md

然后在实现时参考:
3. development/CHARACTER_SWITCHING_DESIGN.md

### 情况3: 我非常紧急（30 分钟）

最少必读:
1. handoff/QUICK_START.md 的前半部分
2. development/COMMON_MISTAKES.md 的"致命错误列表"部分

但强烈不建议在没有充分理解的情况下开始实现。

### 情况4: 我只是想了解项目状态

只读:
1. handoff/QUICK_START.md 的"5分钟快速了解"部分

---

## 文档维护

### 更新规则

当发生以下情况时，需要更新文档：

1. 发现新的常见错误
   - 更新 development/COMMON_MISTAKES.md
   - 添加新的错误分类和案例

2. 设计方案变更
   - 更新 development/CHARACTER_SWITCHING_DESIGN.md
   - 记录变更原因

3. 需求变更
   - 更新 requirements/character-switching-requirements.md
   - 更新版本号和变更日志

4. 完成新的实现 session
   - 在 sessions/ 下创建新的会话记录
   - 更新 handoff/QUICK_START.md 的状态

5. 文档结构调整
   - 更新本 README.md

### 维护责任

文档维护者: 项目团队

更新频率: 每次重要变更后

版本控制: 使用 Git 管理

---

## 文档规范

### Markdown 格式规范

允许使用:
- 标题 (# ## ###)
- 列表 (- 1. 2. 3.)
- 代码块 (```)
- 引用 (>)
- 链接 ([text](url))

禁止使用:
- 粗体 (**text**)
- 斜体 (*text* _text_)
- Emoji (表情符号)
- 装饰符号 (如 ✅ ❌ 等)

内容应该纯净、文本化，不包含强调或表情符号。

### 代码示例规范

好的代码示例:
```cpp
// 正确做法
void Function()
{
    // 简洁清晰的实现
}

// 错误做法（不要这样）
void BadFunction()
{
    // 错误的实现
}
```

坏的代码示例（不要使用装饰）:
```cpp
// ✅ 正确做法（不要用这种符号！）
// ❌ 错误做法（不要用这种符号！）
```

### 注释规范

使用清晰的文字说明，不使用符号装饰:

正确:
```
正确做法: 使用共享组件架构
错误做法: 销毁和重建 Character
```

错误:
```
✅ 正确做法: 使用共享组件架构
❌ 错误做法: 销毁和重建 Character
```

---

## 联系方式

问题反馈: 请通过项目 issue tracker 提交

文档建议: 欢迎提交 pull request

紧急问题: 联系项目负责人

---

最后更新: 2025-10-31
维护者: 项目团队
版本: 1.0
