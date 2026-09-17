# 文档重组讨论会议记录

日期：2025-11-01
参与者：项目负责人、Claude Code
会议主题：NewWorldOrder 文档分类与重组方案

---

## 会议背景

当前问题：
- 文档分散在多个位置（Design/、claude/、Docs/）
- 内容重复且版本不一致（简洁版 vs 详细版）
- 修改一个地方需要同步多个文件
- claude/ 文件夹混合了多种类型的文档

核心目标：
- 打平、合并、整理内容
- 纠错、去重
- 建立单一真相源（Single Source of Truth）
- 按分类输出，便于维护

---

## 关键决策

### 1. GameplayTag 命名规范

问题：
- 当前代码中存在 Ability_Type_Action_Skill1/2/3/4（旧模板标签）
- InputTag_1/2/3/4 与实际键位（Q/E/C/X）不符
- 需要区分技能槽位 Tag 和角色归属 Tag

决策：

方案 C - 两者共存，各自用途不同

技能槽位 Tag（功能标识，不绑定键位）：
```cpp
FGameplayTag InputTag_Ability_Skill1;    // 技能槽 1（键鼠 Q，手柄 LB）
FGameplayTag InputTag_Ability_Skill2;    // 技能槽 2（键鼠 E，手柄 RB）
FGameplayTag InputTag_Ability_Skill3;    // 技能槽 3（键鼠 C，手柄 D-Pad Up）
FGameplayTag InputTag_Ability_Ultimate;  // 大招槽（键鼠 X，手柄 LB+RB）
```

角色归属 Tag（区分男女主角技能，更扁平的结构）：
```cpp
// 采用方案 C：Abilities.Male/Female.SkillQ/E/C/X.TechnicalName
// 示例：
Abilities.Male.SkillQ.TacticalMark         // 男主 Q 技能：战术标记
Abilities.Female.SkillQ.TacticalScan       // 女主 Q 技能：战术扫描
```

说明：
- 武器槽位由 UCombatComponent 管理（int index），不涉及 Tag
- 实际键位配置在 UInputAction 中（增强输入系统）
- Tag 改名后相关文档同步更新

---

### 2. 文档命名规范

决策：

文件名格式：英文_中文.md
- 例如：MaleSkills_男主技能.md
- 理由：AI 和人类都能快速识别

文件夹名格式：英文
- 例如：SystemDesign/、Implementation/
- 理由：避免路径问题，保持代码友好

---

### 3. 最终文档结构

采用五层架构：

```
NewWorldOrder/
├── AGENTS.md
└── Docs/
    ├── DesignPillars/                    # 第一层：设计支柱（很少改）
    │   ├── CoreVision_核心愿景.md
    │   ├── Pillars_三大支柱.md
    │   └── Constraints_约束条件.md
    │
    ├── SystemDesign/                     # 第二层：系统设计（详细版，唯一定义处）
    │   ├── Attributes/
    │   │   ├── Overview_总览.md
    │   │   ├── Formulas_公式.md          # 唯一定义处
    │   │   └── GrowthCurves_成长曲线.md
    │   ├── Skills/
    │   │   ├── Overview_总览.md
    │   │   ├── MaleSkills_男主技能.md    # 唯一定义处
    │   │   ├── FemaleSkills_女主技能.md  # 唯一定义处
    │   │   ├── PassiveSkills_被动技能.md
    │   │   └── ControlMatrix_控制矩阵.md
    │   ├── Weapons/
    │   │   ├── Overview_总览.md
    │   │   ├── WeaponData_武器数值.md    # 唯一定义处
    │   │   ├── CSVStructure_CSV结构.md
    │   │   └── BalanceValidation_平衡验证.md
    │   ├── Narrative/                    # 剧情（从终版三拆分）
    │   │   ├── Story_故事大纲.md
    │   │   ├── Characters_角色设定.md
    │   │   └── Chapters_章节设计.md
    │   └── UI/
    │       ├── Overview_总览.md
    │       └── Interactions_交互设计.md
    │
    ├── Implementation/                   # 第三层：实现指南
    │   ├── GettingStarted_快速上手.md
    │   ├── QuickReference/               # 简洁版（引用 SystemDesign）
    │   │   ├── Attributes_属性.md
    │   │   ├── Skills_技能.md
    │   │   ├── Weapons_武器.md
    │   │   └── InputBindings_输入键位.md
    │   └── Guides/
    │       ├── AddingNewWeapon_添加新武器.md
    │       ├── AddingNewSkill_添加新技能.md
    │       └── NetworkReplication_网络复制.md
    │
    ├── Tasks/                            # 第四层：任务规格
    │   └── CharacterSwitching/
    │       ├── TaskBrief_任务简报.md     # 5 分钟快速了解
    │       ├── Context_上下文.md         # 需要阅读的文档清单
    │       ├── Requirements_需求.md
    │       ├── DesignSpec_设计规格.md
    │       ├── ImplementationGuide_实现指南.md
    │       ├── CommonMistakes_常见错误.md
    │       ├── Checklist_检查清单.md
    │       └── References/               # 相关文档片段
    │
    ├── Engineering/
    │   ├── Style.md
    │   └── Contributing.md
    │
    ├── ThirdParty/
    │   ├── GASDocumentation_Chinese/
    │   └── Mutable-Documentation/
    │
    ├── Archives/                         # 第五层：归档
    │   ├── FailedAttempts/
    │   └── DeprecatedDesigns/
    │
    └── MeetingNotes/                     # 会议记录
        └── 2025-11-01_DocumentReorganization_文档重组讨论.md
```

---

### 4. 文档引用规范

决策：使用相对路径 + Markdown 锚点

示例：
```markdown
详细的武器数值计算参见：
[武器系统详细设计](../../SystemDesign/Weapons/WeaponData_武器数值.md#伤害计算公式)

属性对技能的影响公式参见：
[属性系统公式](../../SystemDesign/Attributes/Formulas_公式.md#技能相关)
```

---

### 5. 单一真相源原则

核心原则：每个知识点只在一个地方定义（唯一定义处），其他地方引用

执行方式：

详细版（SystemDesign）- 唯一定义处
- 定义所有数值、公式、规则
- 包含完整的计算过程、验证、Build 示例
- 用于策划平衡、深度参考

简洁版（Implementation/QuickReference）- 引用
- 只保留常用数值表格和公式
- 引用详细版获取完整信息
- 用于日常开发、快速查阅

任务文档（Tasks）- 提取相关片段
- 从详细版提取任务相关的部分
- 避免 AI 读整个文档库
- 保持自包含，减少上下文切换

变更管理：
1. 修改唯一定义处（SystemDesign）
2. 引用处自动更新（因为是链接）
3. 必要时更新 QuickReference（如果常用数值变了）

---

## 执行计划

### 阶段 1：整理 SystemDesign（详细版）

任务：
- 合并 claude/ 文件夹的数值文档到 SystemDesign
- 纠错、去重
- 确认每个数值的唯一定义处

来源文档：
- claude/新秩序技能系统核心数值-精简版.md → SystemDesign/Skills/
- claude/新秩序武器系统完整设计文档.md → SystemDesign/Weapons/
- Design/Skills.md、Design/Weapons.md、Design/Gameplay.md（旧版，确认后删除）

输出：
- SystemDesign/Attributes/Formulas_公式.md
- SystemDesign/Skills/MaleSkills_男主技能.md
- SystemDesign/Skills/FemaleSkills_女主技能.md
- SystemDesign/Skills/PassiveSkills_被动技能.md
- SystemDesign/Weapons/WeaponData_武器数值.md
- SystemDesign/Weapons/CSVStructure_CSV结构.md
- SystemDesign/Weapons/BalanceValidation_平衡验证.md

---

### 阶段 2：整理 Tasks/CharacterSwitching

任务：
- 重组 claude/ 文件夹的角色切换文档到 Tasks
- 创建 TaskBrief 和 Context
- 从 SystemDesign 提取相关片段到 References

来源文档：
- claude/requirements/character-switching-requirements.md
- claude/development/CHARACTER_SWITCHING_DESIGN.md
- claude/development/COMMON_MISTAKES.md
- claude/handoff/QUICK_START.md
- claude/sessions/2025-10-31-character-switching-failed.md
- claude/README.md

输出：
- Tasks/CharacterSwitching/TaskBrief_任务简报.md
- Tasks/CharacterSwitching/Context_上下文.md
- Tasks/CharacterSwitching/Requirements_需求.md
- Tasks/CharacterSwitching/DesignSpec_设计规格.md
- Tasks/CharacterSwitching/ImplementationGuide_实现指南.md
- Tasks/CharacterSwitching/CommonMistakes_常见错误.md
- Tasks/CharacterSwitching/Checklist_检查清单.md
- Tasks/CharacterSwitching/References/（提取的相关文档片段）

---

### 阶段 3：创建 QuickReference（简洁版）

任务：
- 从 SystemDesign 提炼常用数值和公式
- 只保留表格、关键公式
- 添加引用链接到详细版

输出：
- Implementation/QuickReference/Attributes_属性.md
- Implementation/QuickReference/Skills_技能.md
- Implementation/QuickReference/Weapons_武器.md
- Implementation/QuickReference/InputBindings_输入键位.md

---

### 阶段 4：清理和验证

任务：
- 删除 Design/ 旧文档
- 删除 claude/ 文件夹
- 验证所有引用链接正确性
- 更新 AGENTS.md 指向新文档

---

### 阶段 5：处理终版三（稍后执行）

任务：
- 读取 游戏设计完整文档终版三.md
- 提取剧情故事内容到 SystemDesign/Narrative/
- 与现有文档对比，纠错、去重
- 将独有内容补充到相应分类

输出：
- SystemDesign/Narrative/Story_故事大纲.md
- SystemDesign/Narrative/Characters_角色设定.md
- SystemDesign/Narrative/Chapters_章节设计.md

---

## 关键约束

1. 文件名格式：英文_中文.md
2. 文件夹名：纯英文
3. 引用格式：相对路径
4. 单一真相源：每个数值只定义一次
5. 会话成果及时落地到 repo

---

## 待确认问题

1. SystemDesign/Narrative/ 文件夹英文名是否使用 Narrative？还是其他名称（如 Story）？
2. 角色切换任务的 UI 交互设计应该放在 Tasks/CharacterSwitching/ 还是 SystemDesign/UI/？
3. AGENTS.md 是否需要大幅改动，还是只更新引用链接？

---

## 下次会议议题

- 验证 SystemDesign 整理结果
- 确认 QuickReference 提炼是否充分
- 讨论更多具体任务的文档组织方式

---

会议记录人：Claude Code
文档版本：v1.0
