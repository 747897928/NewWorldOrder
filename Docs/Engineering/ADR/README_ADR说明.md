# Architecture Decision Records (ADR) 说明

## 什么是ADR

ADR（Architecture Decision Records）是架构决策记录，用于记录项目中的重要技术决策。

每个ADR记录：
- 决策是什么
- 为什么做这个决策
- 决策的后果和影响
- 与其他决策的关联

## 目录结构

```
ADR/
├── README_ADR说明.md（本文件）
├── Template_ADR模板.md（创建新ADR时使用）
├── Active/（当前生效的决策）
│   ├── 0001_AbilityCost_Ammo_Prediction.md
│   └── 0002_GameplayTag_Initialization.md
└── Superseded/（已被替代的决策）
    └── 0000_AbilityCost_TagStack.md
```

## 何时创建ADR

以下情况必须创建ADR：
- 架构变更（如武器实例分层重构）
- 方案选择（如弹药预测方案选择）
- 重要约束确立（如GameplayTag初始化规则）
- 第三方库选择（如网络库、UI框架）

## 如何创建ADR

1. 复制Template_ADR模板.md
2. 重命名为：XXXX_Title.md（XXXX是4位数字，递增）
3. 填写所有章节
4. 保存到Active/目录
5. 如果替代了旧ADR，将旧ADR移到Superseded/并更新superseded_by字段

## ADR生命周期

```
Draft → Active → Superseded → Archived
  ↓       ↓          ↓            ↓
 草稿   当前生效   已被替代    归档（可选）
```

- Draft：草稿中，尚未确定
- Active：当前生效，AI和开发者必须遵守
- Superseded：已被新方案替代，移到Superseded/目录
- Archived：长期不变且无参考价值，可由人工删除（AI不操作）

## AI工作流程

### 实现新功能时

1. 检查Active/是否有相关ADR
2. 按ADR中的决策实现
3. 如果需要新决策，创建新ADR

### 变更现有方案时

1. 创建新ADR（记录新方案）
2. 在新ADR中填写supersedes字段（引用旧ADR编号）
3. 将旧ADR移到Superseded/
4. 在旧ADR中添加superseded_by字段和日期

### 读取ADR时

- 只读Active/中的ADR（当前生效）
- 不读Superseded/中的ADR（除非需要了解历史）

## 示例

参见：
- Active/（当前生效的ADR示例）
- Template_ADR模板.md（创建新ADR的模板）
