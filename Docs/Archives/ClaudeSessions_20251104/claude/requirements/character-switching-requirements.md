# 角色切换系统需求文档

版本: 1.0
状态: 待实现
最后更新: 2025-10-31

---

## 项目背景

NewWorldOrder 是一个基于 Unreal Engine 5 的动作角色扮演游戏，使用 GAS (Gameplay Ability System) 作为核心战斗系统。

游戏设定为双主角系统：

- 男主角：陈浩宇
- 女主角：沈芸皖

玩家在游戏过程中需要能够在两个主角之间切换，每个主角拥有独立的技能树、属性值和装备，但共享游戏进度（等级、经验、剧情）。

---

## 功能需求

### FR1: 角色切换

#### FR1.1 基本切换功能

- 用户可以通过特定操作（按键、UI、交互）触发角色切换
- 切换过程应该平滑，无明显卡顿或延迟
- 切换时不应该打断玩家的游戏体验（不重新加载关卡）

#### FR1.2 切换限制

- 某些场景下不允许切换（战斗中、过场动画中、特殊剧情区域）
- 尝试在禁止场景切换时应给出明确提示
- 系统应该能够动态控制是否允许切换

#### FR1.3 视觉反馈

- 切换时应该有适当的视觉效果（淡入淡出、特效等）
- UI 应该及时更新显示当前角色信息
- 技能栏应该立即切换到对应角色的技能

### FR2: 独立数据管理

#### FR2.1 独立属性

每个角色拥有独立的：

- 基础属性：力量、智力、韧性、活力
- 属性点：可用于提升基础属性
- 技能点：可用于学习新技能

#### FR2.2 独立技能

每个角色拥有独立的：

- 技能列表（已学习的技能）
- 技能等级
- 技能装备槽位（装备到哪个按键）
- 技能状态（锁定、可学习、已解锁、已装备）

#### FR2.3 独立装备（可选，未来扩展）

- 武器装备
- 护甲装备
- 饰品装备

### FR3: 共享数据

#### FR3.1 共享进度

两个角色共享：

- 玩家等级
- 经验值
- 剧情进度标记
- 地图解锁状态
- 成就和收藏品

#### FR3.2 共享资源（可选）

- 金钱货币
- 消耗品物品
- 材料物品

### FR4: 数据持久化

#### FR4.1 自动保存

- 切换角色时自动保存当前角色的所有数据
- 保存操作不应该失败（如果失败，应该中止切换）

#### FR4.2 存档加载

- 切换到目标角色时自动加载其数据
- 加载包括属性、技能、装备等所有内容
- 加载后角色应该处于切换前的状态

#### FR4.3 存档兼容性

- 支持向后兼容（旧版本存档可以在新版本加载）
- 保留旧字段用于迁移

### FR5: 网络支持（未来扩展）

#### FR5.1 网络同步

- 角色切换状态应该在服务器和客户端同步
- 其他玩家应该能看到角色切换

#### FR5.2 权限控制

- 只有角色的拥有者可以触发切换
- 服务器验证切换的合法性

---

## 技术约束

### TC1: 架构约束

#### TC1.1 共享组件架构

必须使用共享组件架构，以下组件在整个游戏会话中保持不变：

- APlayerController
- APlayerState
- UAbilitySystemComponent (在 PlayerState 上)
- UShootAttributeSet
- AShootCharacter

禁止使用的方案：

- 不能销毁和重建 Character
- 不能重新创建 AbilitySystemComponent
- 不能重新创建 PlayerState 或 PlayerController

#### TC1.2 GAS 集成

- 必须使用 GAS (Gameplay Ability System)
- AbilitySystemComponent 必须在 PlayerState 上（支持角色重生）
- 技能通过 GameplayTag 区分角色归属

#### TC1.3 实现位置限制

角色切换逻辑不能实现为 GameplayAbility，因为：

- 切换过程会移除当前角色的技能
- 如果切换本身是技能，会移除自己导致逻辑无法完成

推荐实现位置：

- AShootPlayerState (推荐)
- AShootCharacter
- AShootPlayerController

### TC2: 性能约束

#### TC2.1 切换性能

- 切换操作总耗时应小于 100ms
- 不应该有明显的内存分配峰值
- 不应该导致帧率下降

#### TC2.2 内存约束

- 不应该有内存泄漏
- 两个角色的数据应该常驻内存（快速切换）

### TC3: 代码质量约束

#### TC3.1 Sonar 规则

所有代码必须符合 Sonar 扫描规则：

- 函数长度 <= 80 行
- 嵌套深度 <= 3 层
- 认知复杂度 <= 15
- 圈复杂度 <= 10

#### TC3.2 代码组织

- 每个函数职责单一
- 复杂逻辑拆分为多个小函数
- 主函数只做协调

### TC4: 现有系统集成

#### TC4.1 必须使用的现有组件

项目已有以下组件，必须使用（不要重新实现）：

- UMutableAppearanceComponent::SwitchGender() - 外观切换
- FCharacterSnapshot (在 UShootSaveGame 中) - 数据存储
- FShootGameplayTags - GameplayTag 管理
- Lyra Interaction System - 交互系统（如果需要交互触发切换）

#### TC4.2 禁止重新实现

- 不要创建新的交互接口（使用 Lyra 的 IInteractableTarget）
- 不要创建新的外观管理组件
- 不要修改 SaveGame 的核心结构

---

## 现有系统说明

### 外观管理系统

文件位置: Character/MutableAppearanceComponent.h/.cpp

已实现功能:
- SwitchGender(ECharacterGender NewGender) 方法
- 自动切换 Body 和 Head 的 SkeletalMesh
- 自动切换 AnimInstanceClass
- 自动更新 PlayerState 中的性别标记

使用方式:
```cpp
MutableAppearanceComponent->SwitchGender(ECharacterGender::FEMALE);
```

### 存档系统

文件位置: System/ShootSaveGame.h

数据结构:
```cpp
struct FCharacterSnapshot
{
    int32 AttributePoints;
    float Strength;
    float Intelligence;
    float Resilience;
    float Vigor;
    int32 SkillPoints;
    TArray<FSavedAbility> SavedAbilities;
};

class UShootSaveGame
{
    ECharacterGender CurrentGender;
    FCharacterSnapshot MaleSnapshot;
    FCharacterSnapshot FemaleSnapshot;
    int32 PlayerLevel;
    int32 XP;
};
```

使用方式:
```cpp
// 保存
FCharacterSnapshot& Snapshot = SaveGame->MaleSnapshot;
Snapshot.Strength = AttributeSet->GetStrength();

// 加载
AttributeSet->InitStrength(Snapshot.Strength);
```

### GameplayTag 管理

文件位置: AbilitySystem/ShootGameplayTags.h/.cpp

需要添加的 Tag:
```cpp
FGameplayTag Ability_Character_Male;    // "Ability.Character.Male"
FGameplayTag Ability_Character_Female;  // "Ability.Character.Female"
```

使用方式:
```cpp
const FGameplayTag& MaleTag = FShootGameplayTags::Get().Ability_Character_Male;
```

禁止使用:
```cpp
// 不要在运行时使用这个！
FGameplayTag::RequestGameplayTag(FName("Ability.Character.Male"));
```

### Lyra 交互系统

文件位置: Interaction/ 文件夹（19个文件）

核心组件:
- IInteractableTarget - 可交互对象接口
- IInteractionInstigator - 交互发起者接口
- InteractionOption, InteractionQuery - 交互选项和查询
- AbilityTask_WaitForInteractableTargets - 等待交互目标
- ShootGameplayAbility_Interact - 交互技能执行

如果需要通过交互触发角色切换，使用此系统，不要创建新接口。

---

## 非功能需求

### NFR1: 可维护性

- 代码应该清晰易懂
- 关键逻辑应该有注释说明
- 函数命名应该清晰表达意图

### NFR2: 可测试性

- 核心逻辑应该可以单元测试
- 切换逻辑应该可以自动化测试
- 应该有集成测试验证完整流程

### NFR3: 可扩展性

- 未来可能添加更多主角（第三个、第四个）
- 未来可能添加更多独立数据类型
- 设计应该考虑扩展性

### NFR4: 错误处理

- 切换失败时应该有明确的错误提示
- 不应该留下半切换状态
- 失败时应该能够回滚到切换前的状态

---

## 成功标准

### 功能验收

1. 可以从男主切换到女主，反之亦然
2. 切换后外观、技能、属性完全正确
3. 切换前的数据被正确保存
4. 切换后的数据被正确加载
5. 禁止场景下无法切换，并有提示
6. 连续切换多次，数据不丢失

### 性能验收

1. 切换操作 < 100ms
2. 无明显内存泄漏
3. 无明显帧率下降

### 代码质量验收

1. 所有函数 <= 80 行
2. 嵌套深度 <= 3
3. 通过 Sonar 扫描
4. 无编译警告

### 测试覆盖

1. 单元测试覆盖核心逻辑
2. 集成测试覆盖完整流程
3. 边界情况测试通过

---

## 已知风险

### 风险1: 架构理解不足

风险描述:
- 开发者可能不理解共享组件架构
- 可能尝试销毁和重建 Character

缓解措施:
- 必须阅读 development/COMMON_MISTAKES.md
- 必须阅读 development/CHARACTER_SWITCHING_DESIGN.md
- 实现前与团队讨论设计方案

### 风险2: 破坏现有系统

风险描述:
- 可能修改或破坏已有的 MutableAppearanceComponent
- 可能修改或破坏 SaveGame 结构

缓解措施:
- 实现前必须列出相关文件夹的所有文件
- 实现前必须阅读现有组件的实现
- 不要修改现有组件，只使用其 public 接口

### 风险3: GameplayAbility 误用

风险描述:
- 可能将角色切换实现为 GameplayAbility（这是错误的）

缓解措施:
- 明确禁止将切换实现为 Ability
- 文档中清楚说明为什么不能这样做
- Code Review 时重点检查

---

## 实现优先级

### P0 (必须实现)

- FR1.1 基本切换功能
- FR2.1 独立属性
- FR2.2 独立技能
- FR3.1 共享进度
- FR4.1 自动保存
- FR4.2 存档加载

### P1 (重要但可延后)

- FR1.2 切换限制
- FR1.3 视觉反馈
- FR4.3 存档兼容性

### P2 (未来扩展)

- FR2.3 独立装备
- FR3.2 共享资源
- FR5.1 网络同步
- FR5.2 权限控制

---

## 参考文档

必读文档:

1. .docs/development/COMMON_MISTAKES.md - 常见错误和最佳实践
2. .docs/development/CHARACTER_SWITCHING_DESIGN.md - 正确的设计方案
3. .docs/sessions/2025-10-31-character-switching-failed.md - 失败案例分析
4. .docs/handoff/QUICK_START.md - 快速上下文文档

推荐阅读:

- Epic Games Lyra Project Documentation
- Unreal Engine GAS Documentation
- Project Coding Standards

---

最后更新: 2025-10-31
维护者: 项目团队
审核状态: 待审核
