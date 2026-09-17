# 知识资产维护规则（AI自动执行）

本文档定义AI必须执行的自动维护规则。

---

## 维护触发时机

AI在以下时机必须执行维护检查：

### 1. 完成剧情任务后（写完章节）

必须检查：
- CHANGELOG是否过期
- 章节版本是否过多
- SettingRegistry是否需要整理

### 2. 完成代码任务后（实现功能）

必须检查：
- ADR是否需要归档
- DevelopmentNotes是否过多
- CHANGELOG（代码）是否过期

### 3. 重要约束

- 清理前必须先commit和push（保护措施）
- 检查达到阈值才执行清理（避免频繁操作浪费token）
- 永远归档，永不删除（Archives的清理由用户手动决定）

---

## 维护规则详情

### 规则1：CHANGELOG过期检查

触发条件：
- CHANGELOG.md中有变更满足过期条件

过期条件（同时满足）：
- 应用进度=100%（所有受影响文档已修订）
- 距离创建日期≥7天
- 无新的相关变更

执行操作：
1. 将变更从CHANGELOG.md移到CHANGELOG_Recent.md
2. 在CHANGELOG.md顶部添加"已归档变更"说明
3. 保持变更的完整内容（不合并、不删减）

示例：
```markdown
## 已归档变更

- [2025-11-07] 龙旗首领设定变更 → 已移到Recent（应用完成+7天）
```

---

### 规则2：CHANGELOG_Recent过期检查

触发条件：
- CHANGELOG_Recent.md中有变更满足Archive条件

Archive条件（满足其一）：
- 距离移到Recent≥21天（总共28天后归档）
- 用户手动触发Phase切换

执行操作：
1. 将变更从CHANGELOG_Recent.md移到CHANGELOG_Archive.md
2. 合并为摘要（保留关键信息，删除详细内容）
3. 完整内容保存到Archives/CHANGELOG_PhaseX_Detailed.md

示例摘要：
```markdown
## Phase 1: 项目建立期（2025-11-04至2025-11-15）

### 变更摘要
- 武器系统：实例分层重构，拆分为Hitscan和Projectile
- 弹药系统：采用预测视图方案
- 角色切换：实现完整功能
- 剧情设定：龙旗首领从病重改为健康

详细记录：见Archives/CHANGELOG_Phase1_Detailed.md
```

---

### 规则3：章节版本过多检查

触发条件：
- 某章节有>3个版本（如Chapter_003_v1.md, v2.md, v3.md, v4.md）

执行操作：
1. 保留当前版本（Chapter_003.md）
2. 保留最近1个旧版本（Chapter_003_v3.md）
3. 其余版本移到Archives/ChapterVersions/
4. 在Archives/INDEX.md中记录归档内容

示例：
```
归档：
- Chapter_003_v1.md（首领病重版）
- Chapter_003_v2.md（过渡版）
归档原因：设定变更，保留v3和当前版本即可
```

---

### 规则4：ADR过多检查

触发条件：
- Engineering/ADR/Superseded/中有>15个ADR

执行操作：
1. 将Superseded/中的所有ADR合并到Archives/ADR_History.md
2. 按时间倒序排列（最新的在前）
3. 保持每个ADR的完整内容
4. 在ADR_History.md顶部添加索引

ADR_History.md格式：
```markdown
# ADR历史归档

本文档包含所有已被替代的ADR。

## 索引

- ADR-0010: [标题]（2025-11-XX，被ADR-0015替代）
- ADR-0009: [标题]（2025-11-XX，被ADR-0012替代）
...

---

[ADR完整内容，按编号倒序]
```

---

### 规则5：SettingRegistry整理检查

触发条件：
- SettingRegistry.md中有>50个设定
- 且部分设定已经"稳定"（3个月未改）

执行操作：
1. 识别"稳定设定"（status=Locked或3个月未改）
2. 将稳定设定移到StableSettings_稳定设定.md
3. 在SettingRegistry中保留"活跃设定"
4. 更新两个文件的交叉引用

判断"稳定"的条件：
- 距离last_updated≥90天
- 或用户明确标记为"稳定"

---

### 规则6：Phase切换检查

触发条件（AI主动询问用户）：
- 用户说"重写剧情"、"推倒重来"、"大改"
- 检测到大量设定被删除（>5个重要设定）
- CHANGELOG.md累积>50条
- 用户明确说"Phase切换"、"开始新阶段"

执行操作：
1. 显示询问模板（见PROJECT_PHASE.md）
2. 用户确认后按Phase切换流程执行
3. 不要自作主张，必须用户明确确认

---

## 维护执行流程

### 标准流程

任务完成后，AI必须按以下顺序执行：

```
1. 完成功能/内容
2. 更新文档
3. Git提交（功能/内容）← 清理前先保护
4. 维护检查（读取本文件）
5. 逐项检查6个规则
6. 如果满足条件，执行清理
7. Git提交（清理操作）
8. 向用户汇报
```

### 关键约束

清理前必须Git提交：
- 原因：如果清理出错，可以回滚
- 流程：git add . && git commit -m "功能" && git push
- 然后：执行清理
- 最后：git add . && git commit -m "维护" && git push

避免频繁清理（token优化）：
- 检查达到阈值才清理
- 不要每次任务都清理
- 阈值见各规则的"触发条件"

永远归档，永不删除：
- 所有内容移到Archives/
- 不直接删除任何文件
- Archives/的清理由用户手动决定

---

## 维护检查模板

AI执行维护检查时，必须使用以下模板输出结果：

```
维护检查结果：

规则1（CHANGELOG过期）：
- 检查项：CHANGELOG.md中X条变更
- 结果：[无需清理 / 已清理Y条到Recent]
- 详情：[变更标题列表]

规则2（CHANGELOG_Recent过期）：
- 检查项：CHANGELOG_Recent.md中X条变更
- 结果：[无需清理 / 已清理Y条到Archive]

规则3（章节版本过多）：
- 检查项：检查FullChapters/
- 结果：[无需清理 / 归档了Chapter_XXX的Y个旧版本]

规则4（ADR过多）：
- 检查项：Superseded/中X个ADR
- 结果：[无需清理 / 合并到ADR_History.md]

规则5（SettingRegistry整理）：
- 检查项：SettingRegistry中X个设定
- 结果：[无需清理 / 移动Y个稳定设定到StableSettings]

规则6（Phase切换）：
- 检查项：用户意图
- 结果：[无需切换 / 已询问用户]

总结：[本次维护共清理X项 / 无需清理]
```

---

## 注意事项

### 什么时候不需要维护检查

- 任务很小（如修改一个变量名）
- 任务与文档无关（纯代码优化）
- 用户明确说"不需要维护检查"

### 什么时候必须维护检查

- 完成剧情创作（写章节）
- 完成功能实现（架构变更）
- 新增/修改设定
- 用户没有明确说"不需要检查"

### 检查失败怎么办

如果维护检查出错（如文件不存在）：
- 不要中断任务
- 记录错误信息
- 向用户汇报："维护检查遇到错误：[错误信息]"
- 用户可以手动修复或忽略

### 用户可以跳过吗

可以，用户说"跳过维护检查"时：
- AI不执行维护检查
- 但必须在汇报中说明："用户要求跳过维护检查"
- 下次任务仍然会检查

---

最后更新：2025-11-08
