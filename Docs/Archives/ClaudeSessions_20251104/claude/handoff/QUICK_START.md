# 快速上下文文档 - 角色切换系统

版本: 1.0
用途: 新 session 或开发者快速了解项目状态
最后更新: 2025-10-31

---

## 5 分钟快速了解

### 当前状态

状态: 角色切换系统尚未实现

代码: 所有实现代码已回滚，当前代码库是用户的原始代码

文档: 完整的设计文档、错误文档、需求文档已就绪

### 为什么没有实现代码

在之前的 session 中，AI 助手尝试实现角色切换功能，但犯了严重的架构错误：

1. 将角色切换设计为 GameplayAbility（自相矛盾的设计）
2. 尝试销毁和重建 Character（破坏共享组件架构）
3. 创建了重复的交互接口（项目已有 Lyra 系统）
4. 完全没有检查现有代码就开始实现

经过三次警告后，用户决定回滚所有代码，只保留文档。

### 下一步应该做什么

如果你要继续实现角色切换系统：

第一步: 阅读文档（必须，不要跳过）

必读顺序：
1. requirements/character-switching-requirements.md - 了解需求
2. development/COMMON_MISTAKES.md - 了解禁区
3. development/CHARACTER_SWITCHING_DESIGN.md - 了解正确设计
4. sessions/2025-10-31-character-switching-failed.md - 了解失败案例

阅读时间: 约 30-45 分钟

第二步: 检查现有代码（必须，不要跳过）

```bash
# 列出相关文件夹的所有文件
find NewWorldOrder/Source/NewWorldOrder -name "*Appearance*"
find NewWorldOrder/Source/NewWorldOrder -name "*SaveGame*"
find NewWorldOrder/Source/NewWorldOrder/Public/Interaction -type f
find NewWorldOrder/Source/NewWorldOrder -name "*GameplayTags*"
```

必须阅读的文件：
- Character/MutableAppearanceComponent.h/.cpp
- System/ShootSaveGame.h
- AbilitySystem/ShootGameplayTags.h/.cpp
- Interaction 文件夹下所有文件（至少阅读头文件）

第三步: 确认理解

向用户确认：
- 是否理解共享组件架构
- 是否理解为什么不能做成 GameplayAbility
- 是否理解如何使用现有组件

第四步: 开始实现

按照 development/CHARACTER_SWITCHING_DESIGN.md 中的步骤实现。

---

## 项目概况

### 项目信息

名称: NewWorldOrder
引擎: Unreal Engine 5
语言: C++
核心系统: GAS (Gameplay Ability System)

### 游戏类型

动作角色扮演游戏（ARPG）

双主角系统：
- 男主角：陈浩宇
- 女主角：沈芸皖

### 技术栈

核心技术：
- GAS (Gameplay Ability System) - 战斗和技能系统
- Mutable (Customizable Object) - 角色外观系统
- Lyra Interaction System - 交互系统（来自 Epic Games）

关键特性：
- ASC 在 PlayerState 上（支持角色重生）
- 共享组件架构（复用而非重建）
- GameplayTag 驱动的技能管理

---

## 目录结构

### 源代码结构

```
NewWorldOrder/Source/NewWorldOrder/
├── Public/
│   ├── AbilitySystem/          # GAS 相关
│   │   ├── ShootAbilitySystemComponent.h
│   │   ├── ShootAttributeSet.h
│   │   ├── ShootGameplayTags.h
│   │   └── Abilities/          # 所有 GameplayAbility
│   ├── Character/              # 角色相关
│   │   ├── ShootCharacter.h
│   │   └── MutableAppearanceComponent.h  # 外观管理（重要）
│   ├── Player/                 # 玩家相关
│   │   ├── ShootPlayerState.h
│   │   └── ShootPlayerController.h
│   ├── System/                 # 系统
│   │   ├── ShootSaveGame.h     # 存档系统（重要）
│   │   └── ShootSaveGameSubsystem.h
│   ├── Interaction/            # Lyra 交互系统（重要，19个文件）
│   │   ├── IInteractableTarget.h
│   │   ├── InteractionOption.h
│   │   └── ...
│   └── ...
└── Private/
    └── (对应的 .cpp 文件)
```

### 文档结构

```
.docs/
├── requirements/               # 需求文档
│   └── character-switching-requirements.md
├── development/                # 开发文档
│   ├── COMMON_MISTAKES.md      # 常见错误（必读）
│   └── CHARACTER_SWITCHING_DESIGN.md  # 设计方案（必读）
├── sessions/                   # 会话记录
│   └── 2025-10-31-character-switching-failed.md
└── handoff/                    # 交接文档
    └── QUICK_START.md          # 本文档
```

---

## 关键概念

### 共享组件架构

核心原则: 以下组件在整个游戏会话中保持不变，不销毁不重建

```
APlayerController (输入控制)
    └─ APlayerState (玩家数据)
        └─ UAbilitySystemComponent (GAS核心)
            └─ UShootAttributeSet (属性值)

AShootCharacter (角色实体)
    └─ UMutableAppearanceComponent (外观)
```

角色切换时：
- 这些组件全部保留
- 只切换配置（Mesh、AnimClass、Abilities、Attributes 的值）

禁止操作：
- Character->Destroy()
- SpawnActor<ACharacter>()
- 重新创建任何核心组件

### GameplayAbility 限制

角色切换不能做成 GameplayAbility，原因：

```
切换流程:
1. 移除当前角色的所有技能
2. 添加新角色的所有技能

问题:
如果切换本身是技能，步骤1会移除它自己
移除后，步骤2永远不会执行
```

正确方式:
- 在 PlayerState、Character 或 Controller 中实现
- 作为普通方法，不是 Ability

### 属性值修改

切换角色时加载属性值，使用 Init 方法：

正确做法：
```cpp
AttributeSet->InitStrength(NewValue);  // 只设置值，不触发副作用
```

错误做法：
```cpp
AttributeSet->SetStrength(NewValue);   // 会触发GE和网络同步
```

---

## 现有系统详解

### 1. MutableAppearanceComponent

文件: Character/MutableAppearanceComponent.h/.cpp

功能: 管理角色外观（Mesh + AnimClass）

核心方法:
```cpp
void SwitchGender(ECharacterGender NewGender);
```

作用:
- 切换 Body 和 Head 的 SkeletalMesh
- 切换 AnimInstanceClass
- 自动更新 PlayerState 的性别标记

状态: 已完整实现，直接可用

使用示例:
```cpp
UMutableAppearanceComponent* Comp = Character->FindComponentByClass<UMutableAppearanceComponent>();
Comp->SwitchGender(ECharacterGender::FEMALE);
```

### 2. ShootSaveGame

文件: System/ShootSaveGame.h

功能: 双主角存档系统

核心结构:
```cpp
// 单个角色的数据快照
struct FCharacterSnapshot
{
    int32 AttributePoints;
    float Strength, Intelligence, Resilience, Vigor;
    int32 SkillPoints;
    TArray<FSavedAbility> SavedAbilities;
};

// 存档主类
class UShootSaveGame
{
    ECharacterGender CurrentGender;       // 当前角色
    FCharacterSnapshot MaleSnapshot;      // 男主数据
    FCharacterSnapshot FemaleSnapshot;    // 女主数据
    int32 PlayerLevel;                    // 共享：等级
    int32 XP;                             // 共享：经验
};
```

状态: 已完整设计，直接可用

使用示例:
```cpp
// 保存
FCharacterSnapshot& Snapshot = SaveGame->MaleSnapshot;
Snapshot.Strength = AttributeSet->GetStrength();

// 加载
const FCharacterSnapshot& Snapshot = SaveGame->FemaleSnapshot;
AttributeSet->InitStrength(Snapshot.Strength);
```

### 3. Lyra Interaction System

文件: Interaction/ 文件夹（19个文件）

功能: 完整的交互系统（来自 Epic Games Lyra 项目）

核心组件:

文件: IInteractableTarget.h
作用: 可交互对象实现此接口
方法: GatherInteractionOptions() - 收集交互选项

文件: IInteractionInstigator.h
作用: 交互发起者（通常是玩家）

文件: InteractionOption.h, InteractionQuery.h
作用: 交互选项和查询上下文

文件: AbilityTask_WaitForInteractableTargets.cpp
作用: 等待附近出现可交互目标

文件: ShootGameplayAbility_Interact.cpp
作用: 执行交互的 Ability

状态: 已完整实现（来自 Lyra）

重要提示: 不要创建新的交互接口，使用此系统

### 4. FShootGameplayTags

文件: AbilitySystem/ShootGameplayTags.h/.cpp

功能: GameplayTag 管理单例

当前 Tag:
- InputTag.* - 输入相关
- Abilities.* - 技能相关
- ...

需要添加:
- Ability.Character.Male
- Ability.Character.Female

状态: 需要扩展（添加角色相关 Tag）

---

## 实现检查清单

实现角色切换系统前，必须完成以下检查：

阅读文档
- [ ] requirements/character-switching-requirements.md
- [ ] development/COMMON_MISTAKES.md
- [ ] development/CHARACTER_SWITCHING_DESIGN.md
- [ ] sessions/2025-10-31-character-switching-failed.md

检查现有代码
- [ ] 列出 Character/ 文件夹所有文件
- [ ] 列出 AbilitySystem/ 文件夹所有文件
- [ ] 列出 System/ 文件夹所有文件
- [ ] 列出 Interaction/ 文件夹所有文件
- [ ] 阅读 MutableAppearanceComponent.h/.cpp
- [ ] 阅读 ShootSaveGame.h
- [ ] 阅读 ShootGameplayTags.h/.cpp
- [ ] 阅读 Interaction/ 下至少 5 个核心文件

确认理解
- [ ] 理解共享组件架构（不销毁重建）
- [ ] 理解为什么不能做成 GameplayAbility
- [ ] 理解如何使用 MutableAppearanceComponent
- [ ] 理解 FCharacterSnapshot 的数据结构
- [ ] 理解 GameplayTag 的正确使用方式
- [ ] 向用户确认以上理解

实现准备
- [ ] 选择实现位置（推荐 AShootPlayerState）
- [ ] 设计函数拆分（每个函数 <= 80 行）
- [ ] 准备测试用例

开始实现
- [ ] 添加 GameplayTag 定义
- [ ] 实现验证方法
- [ ] 实现保存方法
- [ ] 实现技能切换
- [ ] 实现属性切换
- [ ] 实现外观切换
- [ ] 实现主切换方法
- [ ] 添加错误处理
- [ ] 编写单元测试

---

## 常见问题

### Q: 我应该从哪里开始？

A: 按以下顺序：

1. 阅读所有必读文档（30-45分钟）
2. 检查现有代码（30分钟）
3. 向用户确认理解
4. 按设计文档实现

不要跳过前三步直接实现！

### Q: 为什么不能做成 GameplayAbility？

A: 因为切换过程需要移除当前角色的技能，如果切换本身是技能，会移除自己，导致后续逻辑无法执行。

详细解释参见: development/COMMON_MISTAKES.md 第2节

### Q: 我可以修改 MutableAppearanceComponent 吗？

A: 不建议。它已经完整实现了外观切换功能，直接调用 SwitchGender() 即可。

如果确实需要修改，必须先与用户讨论。

### Q: 存档结构需要修改吗？

A: 不需要。FCharacterSnapshot 已经完整设计好了双主角数据分离。

直接使用 SaveGame->MaleSnapshot 和 SaveGame->FemaleSnapshot。

### Q: 我需要创建交互接口吗？

A: 不需要。项目已有完整的 Lyra 交互系统（19个文件）。

如果需要交互触发切换，使用 IInteractableTarget，不要创建新接口。

### Q: 编译出现 30+ 错误怎么办？

A: 立即停止。这通常意味着设计方向完全错误。

应该：
1. 回滚代码
2. 重新阅读设计文档
3. 与用户讨论

参见: sessions/2025-10-31-character-switching-failed.md

### Q: 如何测试我的实现？

A: 按以下优先级：

1. 单元测试（测试核心逻辑）
2. 集成测试（测试完整流程）
3. 性能测试（测试切换耗时）
4. 边界测试（测试异常情况）

详细测试策略参见: development/CHARACTER_SWITCHING_DESIGN.md 第7节

---

## 致命错误列表

以下错误在之前的实现中已经犯过，绝对不要重复：

错误1: 角色切换做成 GameplayAbility
后果: 自相矛盾，逻辑无法完成

错误2: 销毁和重建 Character
后果: 破坏共享组件架构，大量系统失效

错误3: 运行时使用 RequestGameplayTag
后果: 性能差，容易出错

错误4: 创建重复的交互接口
后果: 与 Lyra 系统冲突，编译错误

错误5: 不检查现有代码就开始实现
后果: 重复造轮子，破坏已有功能

错误6: 函数超过 80 行或嵌套超过 3 层
后果: 违反 Sonar 规则，难以维护

错误7: 使用 Set 方法修改属性
后果: 触发不必要的 GE 和网络同步

详细说明参见: development/COMMON_MISTAKES.md

---

## 成功标准

你的实现成功当且仅当：

功能正确
- [ ] 可以从男主切换到女主
- [ ] 可以从女主切换到男主
- [ ] 切换后外观正确
- [ ] 切换后技能正确
- [ ] 切换后属性正确
- [ ] 数据被正确保存和加载

性能达标
- [ ] 切换耗时 < 100ms
- [ ] 无内存泄漏
- [ ] 无帧率下降

代码质量
- [ ] 所有函数 <= 80 行
- [ ] 嵌套深度 <= 3 层
- [ ] 通过 Sonar 扫描
- [ ] 无编译警告

测试完整
- [ ] 单元测试通过
- [ ] 集成测试通过
- [ ] 边界测试通过

---

## 资源链接

项目文档:
- 需求文档: .docs/requirements/character-switching-requirements.md
- 设计文档: .docs/development/CHARACTER_SWITCHING_DESIGN.md
- 错误文档: .docs/development/COMMON_MISTAKES.md
- 失败案例: .docs/sessions/2025-10-31-character-switching-failed.md

外部资源:
- UE5 GAS Documentation: https://docs.unrealengine.com/5.0/en-US/gameplay-ability-system-for-unreal-engine/
- Lyra Sample Project: Epic Games Launcher -> Learn Tab -> Lyra Starter Game
- Tranek's GAS Documentation: https://github.com/tranek/GASDocumentation

---

## 最后的提醒

核心原则: 先理解，再修改

实现前必须：
1. 阅读所有文档（不要跳过）
2. 检查所有现有代码（不要假设）
3. 向用户确认理解（不要自作主张）

如果遇到问题：
1. 先查文档
2. 再看代码
3. 然后询问用户

如果编译错误超过 10 个：
1. 立即停止
2. 回滚代码
3. 重新审视设计

记住: 之前的实现 100% 失败，代码全部回滚。不要重复相同的错误。

文档比代码更重要。如果不确定，先写文档总结你的理解，让用户确认后再动手。

---

最后更新: 2025-10-31
维护者: 项目团队
适用对象: 新 AI session、GPT-5、人类开发者
