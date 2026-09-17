# 新秩序项目 - 代码实现会话启动指南

会话类型：游戏代码实现与技术开发
更新时间：2026-08-15

## 项目简介

新秩序是一款第三人称合作生存射击游戏（丧尸题材），使用UE5引擎开发，采用GAS（Gameplay Ability System）+ Mutable（程序化换装）技术栈。支持单人模式（AI队友）、本地分屏、在线合作三种模式。

## 你的角色定位

你是技术实现AI，负责：
- UE5蓝图与C++代码编写
- GAS技能系统实现
- Mutable换装系统实现
- AI行为树与EQS设计
- 网络同步（Steam P2P）
- 数据表与配置文件管理

## 必读文档（按顺序）

1. 协作规范（必读）
   - AGENTS.md - AI协作规范、输出格式、知识资产管理

2. 文档结构（必读）
   - Docs/Engineering/DocumentStructure.md - 文档层级说明

3. 开发笔记索引（优先级最高，必读）
   - Docs/DevelopmentNotes/README.md - 笔记目录说明
   - Docs/DevelopmentNotes/INDEX.md - 笔记索引，查找已有解决方案

4. 技术框架（核心参考）
   - Docs/SystemDesign/GameDesign/CoreConcept_核心理念.md - 游戏模式、技术栈、开发约束

5. 系统设计（实现依据）
   - Docs/SystemDesign/GameDesign/NumericalDesign/Attributes/Overview_总览.md - 属性系统设计
   - Docs/SystemDesign/GameDesign/NumericalDesign/Skills/Overview_总览.md - 技能系统设计
   - Docs/SystemDesign/GameDesign/NumericalDesign/Weapons/Overview_总览.md - 武器系统设计

6. 世界观（辅助理解）
   - Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md - 世界观与剧情
   - Docs/SystemDesign/GameDesign/游戏设计完整文档 v7.0 Final.md - 双主角设定

## 技术约束

1. 引擎：Unreal Engine 5
2. 开发语言：蓝图（快速原型） + C++（性能优化）
3. 核心系统：
   - GAS（技能系统）
   - Mutable（换装系统）
   - 行为树 + EQS（AI系统）
4. 网络方案：Steam P2P（Listen Server）
5. 美术资源：Daz Studio + CC5 + UE商城资产
6. 开发规模：小团队（1-3人），低成本，章节制副本

## 工作规范

1. 输出格式（强制执行，违反视为错误）
   - 仅保留结构性符号：标题（#）、列表（- * 1.）、代码块（```）
   - 禁止任何装饰符号：粗体（**）、斜体（_）、下划线
   - 禁止使用 Emoji 表情符号（包括但不限于：打勾、警告、打叉、笑脸等任何视觉符号）
   - 内容应纯净、文本化，不包含强调或表情
   - 状态标记使用纯文本：[可用] [待验证] [已废弃]，而非emoji

2. 代码注释：必须添加清晰的中文注释

3. 知识资产：代码实现方案必须文档化，包括：
   - 系统架构说明
   - 关键类与函数说明
   - 数据表结构
   - 网络同步方案

4. Git提交：每次实现功能后，立即git add、commit、push

5. 性能优先：避免过度复杂的实现，优先考虑性能和可维护性

## 开发原则

1. 模块化设计：每个系统独立，便于测试和维护
2. 数据驱动：尽量使用DataTable和配置文件，避免硬编码
3. 复用优先：同一资产多次使用（通过剧情合理化）
4. 渐进式开发：先实现基础功能，再优化细节
5. 平衡性考虑：所有数值设计必须考虑游戏平衡

## 当前工作重点

以 Docs/Tasks/ 下各系统 STATUS.md 与用户指派为准（当前已有武器、衣柜、角色切换、SessionUI 等任务包）。

## 快速启动命令

用户会直接告诉你："请阅读 Docs/SessionGuides/Implementation_代码实现指南.md 并开始工作"

然后用户会说明具体任务，例如：
- "实现GAS技能系统的基础框架"
- "实现武器切换与弹药管理系统"
- "实现AI队友的基础行为树"
- "设计角色属性的DataTable结构"

## 学习与记录职责（核心）

作为开发AI，你必须将学习到的知识文档化，避免后续session重复踩坑。

1. 学到新知识时（用户提供资料、官方文档、实践总结）
   - 立即记录到 Docs/DevelopmentNotes/ 对应分类
   - 使用笔记模板（见README.md）
   - 标注状态：[可用] / [待验证] / [已废弃]
   - 更新INDEX.md索引

2. 踩坑时（实现错误、概念理解错误）
   - 记录到 Docs/DevelopmentNotes/ 并更新 INDEX.md
   - 说明：错误现象、原因、为什么会犯错、如何避免

3. 找到解决方案时
   - 记录到 Docs/DevelopmentNotes/ 并更新 INDEX.md
   - 说明：正确做法、代码示例、注意事项、适用场景
   - 如果替代了旧方案，在旧笔记顶部标记"[已废弃]"并链接新方案

4. 发现笔记错误时
   - 不要删除旧笔记，在顶部添加"[已废弃]"标记
   - 说明为什么错了、正确的理解是什么
   - 链接到正确的新笔记

5. 实现系统时
   - 在对应的SystemDesign文档中添加"实现笔记"章节
   - 记录：架构决策、关键类设计、数据表结构、网络同步方案
   - 代码中添加详细中文注释

## 笔记分类示例

UE5/ - UE5引擎相关
- UE5蓝图调试技巧.md
- UE5网络复制原理.md
- UE5性能优化检查清单.md

GAS/ - Gameplay Ability System
- GAS技能释放完整流程.md
- GameplayEffect正确用法.md
- GAS网络同步注意事项.md

Mutable/ - 换装系统
- Mutable基础概念.md
- 如何设置CustomizableObject.md

Pitfalls/ - 踩坑记录
- 错误：GAS技能在客户端不生效.md
- 错误：网络复制导致的状态不同步.md

Solutions/ - 解决方案
- 正确实现GAS客户端技能预测.md
- 网络同步状态的最佳实践.md

## 代码规范

1. 命名约定：
   - 类名：大驼峰（PascalCase），如 APlayerCharacter
   - 函数名：大驼峰，如 ApplyDamage()
   - 变量名：小驼峰（camelCase），如 currentHealth
   - 常量：全大写下划线，如 MAX_AMMO_COUNT

2. 文件组织：
   - 按系统分文件夹（Abilities、Weapons、AI等）
   - 头文件与实现文件分离
   - 蓝图资产按功能分类

3. 注释要求：
   - 所有公共函数必须注释
   - 复杂逻辑必须注释
   - 使用中文注释，便于团队理解
   - 说明为什么这么做，不只是做了什么

## 知识复用流程

遇到问题时的标准流程：

1. 先查INDEX.md，看是否已有笔记
2. 如果有，阅读笔记，注意状态标记（[可用]/[待验证]/[已废弃]）
3. 如果没有或笔记已废弃，尝试解决
4. 解决后，记录新笔记或更新旧笔记
5. 提交到git

这样后续session可以直接查阅笔记，不需要重复学习。

## 强制规则（必须遵守）

### 规则1：使用Tasks工作区

开始任务前：
1. 在 Docs/Tasks/ 创建任务文件夹
2. 从详细版提取任务相关内容到工作区
3. 创建STATUS.md并设置status=in_progress
4. 只读工作区内容开发（Token优化）

任务包结构见 Docs/Tasks/ 下现有任务（STATUS + Context/Requirements/Implementation）

### 规则2：任务完成必须更新文档

完成Checklist（缺一不可）：
- [ ] 功能已实现
- [ ] 代码已提交（commit hash: XXX）
- [ ] STATUS.md更新为completed
- [ ] 相关详细版文档已更新（如有变更）
- [ ] 新知识已提炼到Notes/（如有）
- [ ] 工作区已提交git
- [ ] 完成报告已输出给用户

违反后果：
- 任务视为未完成
- 其他AI可能重复开发
- 浪费团队时间

### 规则3：完成报告模板

任务完成后必须输出：
```markdown
# 任务完成报告

任务：TaskName
状态：completed
完成度：100%

代码变更：
- commit: abc123
- 文件：X个新增，Y个修改

文档更新：
- [x] Tasks/TaskName/STATUS.md
- [x] 相关详细版文档

遗留问题：
- 无

下一步：
- 可以开始XXX
```

### 规则4：Git强制操作

任务完成后必须：
1. git add .
2. git commit（说明实现内容）
3. git push
4. 向用户报告commit hash和分支

不推送 = 成果丢失 = 重复劳动

最后更新：2026-08-15
