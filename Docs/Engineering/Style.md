# Engineering Style Guide

## 基础约定
- 缩进使用制表符（Tab 宽 4）；续行可用空格对齐。
- 命名：类型/函数/成员 `PascalCase`；布尔带 `b` 前缀；反射类型保留 `U/A/S/F/I/E/T` 前缀。
- 头/源分离：公共 API 放 `Source/<Module>/Public`，实现放 `Private`，一类一文件。
- 关键字与括号之间保留空格，如 `if (Condition)`；保留 `default` 分支并显式 `break/return`。
- 反射类型不放命名空间；非反射类型可使用 `UE::Subsys` 等命名空间。
- 优先 UE 基本类型：`int32`、`uint8`、`FString`、`TArray`；常量使用 `constexpr`/`const`。
- 日志与断言使用 `UE_LOG`、`UE_CLOG`、`check`、`ensure`。

## Include 策略
1. 自身头文件。
2. 同模块依赖。
3. 其他模块。
4. 第三方与标准库。
- 能前置声明就不 include；宏文件最小化引用范围。

### Headers & Forward Declarations（编译期依赖规范）
- .h 优先使用前置声明（class/struct），只有在继承、按值成员、内联/sizeof/模板实例化/反射宏访问成员时才 include 完整类型；反射基类、核心依赖按官方示例在 .h include。
- .cpp 顶部必须 include 自己的 .h，然后按需 include 完整类型（例如使用 FOverlapResult 时 include "Engine/OverlapResult.h"；使用 UGameplayAbility 时 include 对应头）。
- 避免循环依赖：一侧前置声明，另一侧在 .cpp include；不要互相 include。
- GAS GetSet<T>() 返回 const T*，只能只读；写属性请通过 GameplayEffect/ASC 修改，不要直接改 AttributeSet 成员。
- 动态多播（DECLARE_DYNAMIC_MULTICAST_DELEGATE）统一使用 AddDynamic/RemoveDynamic 绑定，不要用 AddUObject/FDelegateHandle。

## 代码注释
- 中文注释用于说明跨类消息流、复制回调、GAS 行为等复杂逻辑（与 `AGENTS.md` 要求一致）。
- 简单赋值或局部变量无需赘述，保持文档与实际行为同步。

## EditorConfig（自动工具）
```
[*.{h,hpp,cpp,inl}]
indent_style=tab
tab_width=4
indent_size=4
end_of_line=lf
charset=utf-8
trim_trailing_whitespace=true
insert_final_newline=true
```

## 文档命名规范

### 文件命名格式

所有Markdown文档必须遵循：英文_中文.md

示例：
- 正确：Overview_总览.md
- 正确：MaleSkills_男主技能.md
- 正确：ImplementationChecklist_实现清单.md
- 错误：Overview.md（缺少中文）
- 错误：总览.md（缺少英文）
- 错误：Overview-总览.md（使用连字符而非下划线）

英文部分：
- 使用PascalCase（首字母大写）
- 多个单词直接连接（如CharacterSwitching）
- 不使用空格或连字符

中文部分：
- 简洁明确（6个汉字以内）
- 直接说明文档内容

分隔符：
- 必须使用下划线（_）
- 不使用连字符（-）或空格

### 文件夹命名格式

文件夹只使用英文，不包含中文。

示例：
- 正确：SystemDesign/
- 正确：QuickReference/
- 正确：CharacterSwitching/
- 错误：SystemDesign_系统设计/（不要加中文）
- 错误：system-design/（不要用连字符）

### AI创建文档时的检查清单

创建新文档前，AI必须：
- 确认文件名符合"英文_中文.md"格式
- 确认英文部分使用PascalCase
- 确认使用下划线分隔
- 确认文件夹名称只有英文

违反规范的文档会导致：
- 文档系统混乱
- 引用链接失效
- 跨session协作困难

## 文档元数据规范

### 元数据格式

所有重要文档（SystemDesign、Tasks、ADR等）必须在文件开头包含元数据块：

```markdown
---
doc_id: [文档ID]
title: [文档标题]
version: [版本号]
status: Active / Draft / Superseded / Archived
last_updated: YYYY-MM-DD
author: [作者]
supersedes: [前一版本doc_id]（如果适用）
related_docs:
  - [相关文档路径]
---

# 文档标题

[文档内容...]
```

### 元数据字段说明

doc_id：
- 格式：[类别]-[系统]-[编号]
- 示例：SYS-WPN-001（系统设计-武器-001）、ADR-0001、TASK-CS-001

version：
- 格式：主版本.次版本（如1.0, 2.1）
- 主版本变更：重大内容修订
- 次版本变更：小幅修正、补充

status：
- Active：当前生效（AI主要阅读）
- Draft：草稿中
- Superseded：已被新版本替代
- Archived：归档（历史参考）

last_updated：
- 格式：YYYY-MM-DD
- 每次修改必须更新

related_docs：
- 列出相关文档的相对路径
- 帮助AI理解文档关联

### 元数据示例

SystemDesign文档示例：

```markdown
---
doc_id: SYS-WPN-001
title: 武器系统设计
version: 2.1
status: Active
last_updated: 2025-11-08
author: 项目负责人
supersedes: SYS-WPN-001-v2.0
related_docs:
  - ../Attributes/Overview_总览.md
  - ../../Engineering/ADR/Active/0003_Weapon_Hierarchy_Refactor.md
---
```

ADR文档示例：

```markdown
---
adr_id: ADR-0003
title: 武器实例分层重构
date: 2025-11-02
status: Active
supersedes: ADR-0000
superseded_by: null
author: Claude Code
---
```

### AI使用元数据的规则

阅读文档时：
- 优先阅读status=Active的文档
- 检查last_updated判断是否最新
- 如果status=Superseded，查找superseded_by指向的新文档

创建新文档时：
- 必须包含完整元数据
- doc_id必须唯一（检查是否与现有冲突）
- status初始为Draft或Active

更新文档时：
- 必须更新last_updated
- 如果是重大修订，更新version（主版本+1）
- 如果替代旧文档，填写supersedes字段

归档旧文档时：
- 更新status为Superseded或Archived
- 填写superseded_by字段（如果有新文档）
- 移动到Archives/目录
