# 文档结构设计与阅读策略

## 为什么要拆分文档

问题：
- 单个大文档（2500+行）维护困难，修改一处需要翻阅大量内容
- AI读取大文档消耗大量token，效率低下
- 内容混杂（剧情+数值+实现），不同角色（策划/程序）需要的信息不同
- 文档重复，版本不一致

解决方案：
- 按职责拆分（剧情/属性/技能/武器/任务）
- 单一数据源（SSOT）原则：每个知识点只在一处定义
- 分层架构：设计支柱 -> 系统设计 -> 快速参考 -> 任务包

## 文档层级结构

```
NewWorldOrder/
├── AGENTS.md                         # 入口文档，AI必读
└── Docs/
    ├── SystemDesign/                 # 第一层：系统设计（唯一定义处）
    │   ├── Attributes/               # 属性系统
    │   ├── Skills/                   # 技能系统
    │   ├── Weapons/                  # 武器系统
    │   └── Narrative/                # 剧情系统（从终版三拆分）
    │
    ├── QuickReference/               # 第二层：快速参考（引用SystemDesign）
    │   ├── Attributes.md             # 常用属性表
    │   ├── Skills.md                 # 常用技能表
    │   ├── Weapons.md                # 常用武器表
    │   └── CharacterSwitching.md     # 角色切换快速参考
    │
    ├── Tasks/                        # 第三层：任务包（自包含）
    │   └── CharacterSwitching/       # 角色切换任务
    │       ├── Overview_总览.md
    │       ├── DesignGuide_设计指南.md
    │       ├── BestPractices_最佳实践.md
    │       └── ImplementationChecklist_实现清单.md
    │
    ├── Engineering/                  # 工程规范
    │   ├── Style.md
    │   ├── Contributing.md
    │   ├── DevelopmentNotes.md
    │   └── DocumentStructure.md      # 本文档
    │
    ├── Archives/                     # 归档
    │   ├── OldDesign_20251101/       # 旧设计文档
    │   └── ClaudeSessions/           # Claude会话记录归档
    │
    └── MeetingNotes/                 # 会议记录
        └── 2025-11-01_DocumentReorganization_文档重组讨论.md
```

## AI阅读策略（解决"只看AGENTS.md"的问题）

核心原则：AGENTS.md是索引，不是全部内容

### 策略1：AGENTS.md明确告知需要阅读的文档

AGENTS.md应该：
- 开头明确列出文档结构
- 针对不同任务类型，明确指出需要阅读的文档路径
- 使用"默认阅读本文件，按需查阅XXX"的模式

示例：
```markdown
- 你正在处理 Unreal Engine 5 C++ 项目 NewWorldOrder。
- 默认仅需阅读本文件；若需要玩法与数值细节，请按需查阅：
  - 快速参考：Docs/QuickReference/ - AI日常开发快速查询
  - 详细设计：Docs/SystemDesign/ - 完整系统设计文档（SSOT）
  - 实现指南：Docs/Tasks/CharacterSwitching/ - 特定任务的实现包
```

### 策略2：在任务描述中明确文档要求

用户在派发任务时，应该在任务描述中明确：
```
任务：实现角色切换功能
必读文档：
- AGENTS.md
- Docs/Tasks/CharacterSwitching/Overview_总览.md
- Docs/QuickReference/CharacterSwitching.md
```

### 策略3：文档内部使用相对路径引用

每个文档应该包含：
- 本文档定位：这是什么文档，属于哪一层
- 相关文档链接：详细版在哪里，快速参考在哪里

示例：
```markdown
# 武器快速参考

本文档位于：Docs/QuickReference/Weapons.md
详细设计文档：[武器系统详细设计](../SystemDesign/Weapons/WeaponData_武器数值.md)

本文档仅包含常用数值表格，完整公式和验证请查阅详细设计文档。
```

### 策略4：任务包（Tasks/）自包含

任务包文档应该：
- 在Overview中明确列出需要阅读的所有文档
- 提取相关片段到References/子目录
- 避免AI需要跳转多个文档

示例结构：
```
Tasks/CharacterSwitching/
├── Overview_总览.md                   # 包含阅读清单
├── DesignGuide_设计指南.md            # 自包含设计说明
├── BestPractices_最佳实践.md
├── ImplementationChecklist_实现清单.md
└── References/                        # 提取的相关文档片段
    ├── GAS_Initialization.md          # 从SystemDesign提取
    └── Mutable_Appearance.md          # 从SystemDesign提取
```

## 文档维护规范

### 单一数据源（SSOT）原则

规则：
- 每个知识点（数值/公式/规则）只在一个地方定义
- 其他地方通过引用链接指向唯一定义处
- 修改时只需修改唯一定义处

示例：
```markdown
错误做法：
- QuickReference/Skills.md 中定义：战术标记CD 15秒
- SystemDesign/Skills/MaleSkills_男主技能.md 中定义：战术标记CD 15秒
（两处定义，容易不一致）

正确做法：
- SystemDesign/Skills/MaleSkills_男主技能.md 中定义：战术标记CD 15秒（唯一定义处）
- QuickReference/Skills.md 中引用：[详细设计](../SystemDesign/Skills/MaleSkills_男主技能.md#战术标记)
```

### 文档修改流程

步骤：
1. 确认唯一定义处：这个知识点在哪个文档中定义？
2. 修改唯一定义处：只修改那个文档
3. 检查引用处：QuickReference是否需要更新？
4. 更新文档版本：在文档底部记录修改历史
5. 提交到Git：附上清晰的commit message

### 文档归档规则

何时归档：
- 文档已被新文档替代
- 会话记录超过3个月且无参考价值
- 失败的设计尝试（保留教训）

归档位置：
- 旧设计文档：Docs/Archives/OldDesign_YYYYMMDD/
- 会话记录：Docs/Archives/ClaudeSessions/YYYY-MM/
- 失败尝试：Docs/Archives/FailedAttempts/

归档操作：
```bash
# 移动旧文档到归档目录
git mv Docs/Design/Weapons.md Docs/Archives/OldDesign_20251101/Weapons.md

# 创建弃用说明
echo "# 已弃用" > Docs/Design/README_DEPRECATED.md
echo "本目录文档已归档到 Docs/Archives/OldDesign_20251101/" >> Docs/Design/README_DEPRECATED.md

# 提交
git add .
git commit -m "docs: 归档旧Design/文件夹到Archives"
git push
```

## 常见问题

### Q1: 如何确保AI读取正确的文档？

A: 在AGENTS.md中明确列出文档路径，并在任务描述中重申必读文档清单。

### Q2: 如果发现文档过时怎么办？

A:
1. 先更新唯一定义处（SystemDesign）
2. 再更新引用处（QuickReference）
3. 提交Git并在commit message中说明
4. 在会话中告知用户

### Q3: 拆分文档后会不会导致信息碎片化？

A: 通过以下方式避免：
- 任务包（Tasks/）自包含，提取相关片段
- QuickReference提供快速索引
- 文档间使用相对路径链接

### Q4: 如何处理跨系统的设计（如技能影响属性）？

A:
- 在各自的SystemDesign中定义自己的部分
- 使用相对路径引用对方文档
- 在Overview中说明跨系统关系

示例：
```markdown
# SystemDesign/Skills/Overview_总览.md

## 技能与属性的关系

技能伤害受属性影响，具体公式参见：
[属性系统公式](../Attributes/Formulas_公式.md#技能相关)
```

## 文档版本历史

- v1.0 (2025-11-04)：创建文档结构设计与阅读策略
