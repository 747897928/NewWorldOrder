# AI 开发会话总结 - 角色切换系统实现失败记录

会话日期： 2025-10-31
会话目标： 实现双主角角色切换系统（陈浩宇/沈芸皖）
最终结果： 实现失败，所有代码被回滚，仅保留经验文档

---

## 执行摘要

本次会话是一次完全失败的实现尝试。AI 助手（Claude Code）尝试实现角色切换功能，但由于以下根本性问题，所有代码最终被回滚：

1. 没有理解现有架构 - 在修改代码前没有全面检查项目现有实现
2. 忽视用户警告 - 用户给出了三次明确警告，但未被充分重视
3. 架构设计错误 - 将角色切换设计为 GameplayAbility（根本性错误）
4. 破坏现有系统 - 导致 30+ 编译错误，破坏了用户已有的功能
5. 重复造轮子 - 创建了与 Lyra 交互系统重复的接口

最重要的教训：先理解，再修改。不要在不了解现有系统的情况下编写代码。

---

## 会话时间线

### 阶段 1：初始任务（错误的开始）

用户请求： 继续完成角色切换任务

AI 的错误行为：
- 没有检查现有代码就开始实现
- 创建了 `GA_CharacterSwitch` 作为 GameplayAbility
- 试图 Destroy/Spawn Character 来切换角色
- 没有使用用户已有的 `UMutableAppearanceComponent`

用户第一次警告：
> "角色切换能做成技能吗？不能！切换角色的时候就会移除对应主角的技能替换成另一个主角的技能。"

核心问题：
- GameplayAbility 会在切换时移除自己，导致逻辑无法完成
- ASC、AttributeSet、Character、PlayerController、PlayerState 都是共享组件，不应该销毁重建

# 阶段 2：GameplayTag 初始化顺序失误

**结论**：在 CDO/构造与静态初始化阶段读取 `FGameplayTag` 是高风险操作；应先**注册**（.ini 或原生宏），并把**读取**延后到 `PostLoad`/`OnRegister`/`BeginPlay`，或在 AssetManager 的 `StartInitialLoading()` 期间集中完成原生标签的注册与缓存。未提前注册时，`RequestGameplayTag` 默认会在找不到标签时报警。([Epic Games Developers][1])

---

## 症状与诱因

* 在构造器里直接写：

  ```cpp
  UShootCharacter::UShootCharacter()
  {
      ActiveGenderTag = FShootGameplayTags::Get().Ability_Character_Male;
  }
  ```

  此时标签系统尚未完成初始化，读取到的是**空标签**或随后逻辑触发 ensure。`RequestGameplayTag(Tag, /*ErrorIfNotFound=*/true)` 的默认参数会在未注册时记录错误。([Epic Games Developers][1])

* `.ini` 中没有正确注册（例如漏写数组追加前缀），或未重启编辑器使 `.ini` 变更生效，导致“看似配置了，实际未入字典”。([GitHub][2])

* 把“原生标签初始化”放到 **AssetManager 初始化之后** 的阶段以前使用，会出现“编辑器里能看到标签，但编辑期/构造期代码拿不到”的时间顺序错位。([Epic Developer Community Forums][3])

---

## 根本原因（按时间线）

1. **原生（Native）标签注册的时机**
   `FNativeGameplayTag` 在**模块静态构造**时注册，这类标签在模块加载期即可可用；若你选择不使用原生宏而集中在自定义入口里注册，就要保证任何使用动作发生在注册之后。([Epic Games Developers][4])

2. **基于配置的标签导入**
   引擎会从 `Config/DefaultGameplayTags.ini` 以及 `Config/Tags/` 的源文件导入；编辑 `.ini` 需要**重启编辑器**才能加载更新。([Epic Games Developers][5])

3. **项目惯用做法**
   很多项目仿照 Lyra：在自定义 `UAssetManager::StartInitialLoading()` 中调用 `InitializeNativeTags()` 统一注册标签与相关系统。若在这之前的构造/静态期访问标签，就会踩空。([GitHub][6])

---

## 正确做法（可直接套用）

### 1) 先**注册**，后**读取**

* **原生（推荐）**：在 `.h` 声明、`.cpp` 定义

  ```cpp
  // .h
  UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Character_Male);

  // .cpp
  UE_DEFINE_GAMEPLAY_TAG(Ability_Character_Male, "Ability.Character.Male");
  ```

  原生标签在模块静态期注册，适合“早用、常用”的基础标签。([Epic Games Developers][4])

* **基于 .ini**：确保语法正确，并重启

  ```ini
  [/Script/GameplayTags.GameplayTagsSettings]
  ImportTagsFromConfig=True
  WarnOnInvalidTags=True
  +GameplayTagList=(Tag="Ability.Character.Male",DevComment="")
  ```

  关键是前缀 **`+GameplayTagList=`** 表示向数组**追加**；少了 `+` 往往不会真正注册到运行时字典。([GitHub][2])

### 2) 统一初始化入口

在自定义 AssetManager 中集中初始化，避免“各处零散注册、时机不一致”：

```cpp
void UMyAssetManager::StartInitialLoading()
{
    Super::StartInitialLoading();
    FMyGameplayTags::InitializeNativeTags();       // 统一 AddNativeGameplayTag/宏定义等
    UAbilitySystemGlobals::Get().InitGlobalData(); // 其他系统初始化
}
```

这样能把“标签可用”的时间点固定在 StartInitialLoading 之后。([GitHub][6])

### 3) 早期阶段**不要依赖最终值**

* 构造函数/静态变量初始化：只存 `FName` 或延后读取；真正用到时（`PostLoad`/`OnRegister`/`BeginPlay`）再从 `FShootGameplayTags::Get()` 或 `RequestGameplayTag()` 获取。
* 若**必须**在构造期请求，务必容错：

  ```cpp
  ActiveGenderTag = FGameplayTag::RequestGameplayTag(
      FName("Ability.Character.Male"), /*ErrorIfNotFound=*/false);
  // 稍后在系统完成初始化后再次校验并修正
  ```

  默认参数会在未找到时报错；将其设为 `false` 可先占位、后校验。([Epic Games Developers][1])

---

## 可选的“防倒栽葱”措施

* **自检脚本**：启动后列举“项目必备标签”，逐一 `RequestGameplayTag` 并输出缺失清单；
  配合控制台命令 **`GameplayTags.List`** 与 Gameplay Tag Browser 快速核对导入结果。([CSDN博客][7])

* **流程化注册**：

  * 基础/跨系统标签 → 用原生宏，享受“静态构造即注册”的确定性；([Epic Games Developers][4])
  * 量大且经常改动的标签（策划同学维护）→ DataTable/`.ini` 源，编辑后**重启**以生效。([Epic Games Developers][5])

---

## 易错点备忘

* **误以为**“编辑器里能看见标签 = 构造期也能拿到”
  并不等价；编辑器 UI 看到的是资源/配置侧状态，构造/静态期代码能否拿到，取决于**注册是否已完成**与**访问时机**。对应的社区案例就发生在“编辑期代码请求标签”时报错。([Epic Developer Community Forums][8])

* **忘记 `+GameplayTagList=`** 或编辑 `.ini` 后不重启
  这两点会直接导致运行时未注册或注册集未刷新。([GitHub][2])

[1]: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/GameplayTags/FGameplayTag?utm_source=chatgpt.com "FGameplayTag | Unreal Engine 5.6 Documentation - Epic Dev"
[2]: https://github.com/tranek/GASDocumentation/blob/master/Config/DefaultGameplayTags.ini?utm_source=chatgpt.com "GASDocumentation/Config/DefaultGameplayTags.ini at master - GitHub"
[3]: https://forums.unrealengine.com/t/gameplay-tags-becoming-invalidated-after-playing-the-game/1146847?utm_source=chatgpt.com "Gameplay tags becoming invalidated after playing the game?"
[4]: https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/GameplayTags/FNativeGameplayTag?utm_source=chatgpt.com "FNativeGameplayTag | Unreal Engine 5.6 Documentation - Epic Dev"
[5]: https://dev.epicgames.com/documentation/zh-cn/unreal-engine/using-gameplay-tags-in-unreal-engine?utm_source=chatgpt.com "在虚幻引擎中使用Gameplay标签 | 虚幻引擎 5.6 文档 | Epic ..."
[6]: https://github.com/RedisTKey/LyraBasedGame?utm_source=chatgpt.com "GitHub - RedisTKey/LyraBasedGame: Implementation of Lyra GAS Input ..."
[7]: https://blog.csdn.net/m0_45371381/article/details/146084074?utm_source=chatgpt.com "UE中UGameplayTagsManager类详解 - CSDN博客"
[8]: https://forums.unrealengine.com/t/error-requested-gameplaytag-in-editor-time-code/1830557?utm_source=chatgpt.com "Error requested GameplayTag in editor-time code"

### 阶段 3：类型转换过度约束

AI 的错误：
```cpp
// 错误：Cast 到最具体的子类，但只使用父类方法
AShootCharacter* Character = Cast<AShootCharacter>(ActorInfo->AvatarActor.Get());
if (Character)
{
    Character->Jump(); // Jump() 是 ACharacter 的方法，不需要 Cast 到 AShootCharacter！
}
```

用户反馈：
> "后面根本没用到 AShootCharacter 的特有能力（成员/接口），那就不该把对象强行收窄到这个最具体的子类。"

正确原则：
- 只 Cast 到实际需要的最宽泛类型
- 减少耦合，提高代码复用性

### 阶段 4：函数复杂度过高

AI 的错误：
- 实现了 200+ 行的 `SwitchCharacterGender()` 函数
- 嵌套层级达到 4-5 层
- 违反 Sonar 规则：函数 ≤80 行，嵌套 ≤3 层

用户反馈：
> 提供了详细的 Sonar 扫描规则和重构要求

正确做法：
- 将复杂函数拆分为多个职责单一的小函数
- 每个函数不超过 80 行
- 嵌套深度不超过 3 层
- 认知复杂度 ≤15

### 阶段 5：编译错误爆发（30+ 错误）

AI 做了什么：
1. 修改了 SaveGame 结构但没有更新所有使用处
2. 修改了 InteractionInterface.cpp 导致重复定义
3. UCLASS 缺少 config 参数
4. 访问了 protected 成员
5. 使用了已弃用的 API

编译错误示例：
```
Error: Classes with config member variables need to specify config file
Error: 'SetCurrentSlotIndex': is not a member of 'UShootSaveGameSubsystem'
Error: function already has body (BlueprintNativeEvent _Implementation)
Error: 'Strength': is not a member of 'UShootSaveGame' (已改为 MaleSnapshot.Strength)
```

用户反馈：
> "你每次跟我说编译解决了，但实际上编译就是没有通过。"

### 阶段 6：第三次警告 - 未检查现有代码

AI 的致命错误：
1. 创建了 `IInteractionInterface` 接口
2. 修改了 `InteractionInterface.cpp` 添加 `_Implementation` 函数
3. 完全没有检查 Interaction 文件夹下的 Lyra 交互系统

Interaction 文件夹实际内容（19个文件）：
- `IInteractableTarget.h` - Lyra 核心交互接口
- `IInteractionInstigator.h` - 交互发起者
- `InteractionOption.h`, `InteractionQuery.h` - 交互选项和查询
- `AbilityTask_WaitForInteractableTargets.h/.cpp` - 完整的 AbilityTask 系统
- `AbilityTask_GrantNearbyInteraction.h/.cpp`
- `ShootGameplayAbility_Interact.h/.cpp` - 交互技能
- `GameplayAbilityTargetActor_Interact.h/.cpp` - 目标选择器
- `InteractionStatics.h/.cpp` - 静态工具函数

用户第三次警告（最严厉）：
> "IInteractionInterface 不是我写的，是你写的，我并不需要这个类。我记得我抄了 lyra 的 Interaction 代码，都在 Interaction 文件夹下，这个文件下的代码你是真不看？UInteractableTarget 也不看？把我之前的话当耳边风？这是第三次警告。"

核心问题：
- 项目已有完整的 Lyra 交互系统（来自 Epic Games）
- AI 没有列出文件夹内容就直接修改代码
- 创建了重复的、不需要的接口
- 这是最严重的错误 - 展示了对现有代码的完全无视

### 阶段 7：回滚决策

用户最终决定：
> "要不你还是回滚代码重新写吧，但是回滚代码后你记得把文档同步更新，别回滚个代码把文档搞丢了。"

执行的回滚：
1. `git reset --hard 6ad3034` - 回到添加文档的提交
2. 备份并恢复 `COMMON_MISTAKES.md`
3. 删除 `InteractionInterface.h/.cpp`
4. 修正文档中的错误描述

### 阶段 8：最终决定 - 删除所有代码

用户最终指示：
> "你不适合写代码，把你的代码全干掉吧，恢复我之前的代码，把这个会话你学到的所有知识，踩过的坑，以及你的经验全部整理成文档保存到 repo 吧。"

执行的操作：
1. `git reset --hard origin/main` - 完全回到用户的原始代码
2. 保留并增强所有文档
3. 创建本总结文档
4. 创建角色切换设计指南

---

## 根本性错误分析

### 1. 架构理解缺失

错误： 将角色切换实现为 GameplayAbility

为什么错误：
```
切换流程：移除当前角色技能 → 添加新角色技能
          ↓
    如果切换本身是 Ability，它会在第一步被移除！
          ↓
    后续逻辑无法执行 → 系统自相矛盾
```

正确理解：
- 角色切换是系统级操作，不是技能
- 应该在 Character、PlayerState 或 Controller 层面实现
- 技能系统是被切换的对象，不是切换的执行者

### 2. 组件生命周期误解

错误： 尝试 Destroy Character 并 Spawn 新的

为什么错误：
```
共享组件（在整个游戏会话中保持）：
├─ APlayerController（玩家输入和控制）
├─ APlayerState（玩家数据和状态）
│   └─ UAbilitySystemComponent（技能系统核心）
│       └─ UShootAttributeSet（属性值）
└─ AShootCharacter（角色表现）
    └─ UMutableAppearanceComponent（外观管理）

销毁 Character 会导致：
 丢失 PlayerController 引用
 破坏 ASC 的 AvatarActor 绑定
 重新初始化所有系统的开销
 存档/读档逻辑变得异常复杂
```

正确理解：
- 所有这些组件都是复用的，不是重建的
- 只需要切换：
  - 外观（Mesh + AnimClass）- 已有 `UMutableAppearanceComponent::SwitchGender()`
  - 技能（通过 ASC 移除/添加）
  - 属性值（从 SaveGame 加载）

### 3. 没有检查现有实现

错误行为：
1. 看到需要某个功能
2. 立即开始编写代码
3. 创建新类、新接口、新系统
4. 完全不检查是否已经存在

实际情况：
| 需要的功能 | AI 的行为 | 用户已有实现 |
|-----------|----------|------------|
| 外观切换 | 尝试 Destroy/Spawn Character | `UMutableAppearanceComponent::SwitchGender()` |
| 交互系统 | 创建 `IInteractionInterface` | Lyra 完整交互系统（19个文件） |
| 属性保存 | 尝试直接修改 SaveGame 结构 | `FCharacterSnapshot` 已经设计好 |

正确流程：
```
收到任务
  ↓
列出相关文件夹的所有文件（Glob/Find）
  ↓
阅读核心接口和基类
  ↓
理解现有架构和组件关系
  ↓
询问用户确认理解
  ↓
（只有在此之后）开始编写代码
```

### 4. 忽视用户反馈

三次警告的递进：

1. 第一次 - 架构问题
   - 内容：角色切换不能做成 GameplayAbility
   - 严重性：中等
   - AI 反应：修改了实现方式

2. 第二次 - 技术细节
   - 内容：GameplayTag 使用、类型转换、函数复杂度
   - 严重性：高
   - AI 反应：记录到文档，但继续犯其他错误

3. 第三次 - 工作方法
   - 内容：完全没有检查现有代码
   - 严重性：致命
   - AI 反应：回滚代码
   - 用户反馈：这是第三次警告！

问题根源：
- 用户的警告指向根本性问题（工作方法、思维方式）
- AI 只修正了表面问题（具体实现细节）
- 没有从警告中反思为什么会犯这个错误

---

## 学到的核心经验

### 经验 1：永远先理解，再修改

原则：
```
理解时间 > 修复时间

花 2 小时理解现有代码 → 30 分钟正确实现
跳过理解，直接开始 → 数小时错误实现 + 回滚 + 重新开始
```

必须做的检查清单：
- [ ] 列出相关文件夹的所有文件
- [ ] 阅读核心接口和基类的头文件
- [ ] 查找现有的实现和使用示例
- [ ] 理解组件之间的关系和生命周期
- [ ] 检查 Copyright 信息（Epic Games、Lyra 等）
- [ ] 询问用户确认理解是否正确
- [ ] 明确新功能如何与现有系统集成

### 经验 2：遵循现有架构模式

用户已建立的架构模式：

1. 共享组件架构
   - PlayerController、PlayerState、ASC、AttributeSet 在整个会话中复用
   - 不要销毁和重建，只切换配置

2. GameplayTag 管理
   - 所有 Tag 在 `FShootGameplayTags` 单例中定义
   - 使用 `AddNativeGameplayTag()` 注册
   - 运行时只读取，不创建

3. 存档系统
   - 使用 `FCharacterSnapshot` 结构
   - 分开存储男女主角数据
   - 保留旧字段用于向后兼容

4. 第三方代码
   - Lyra 系统 - 完整使用，不要修改或重新实现
   - Epic Games 的代码有其设计理念，优先扩展而非替换

### 经验 3：识别自相矛盾的设计

示例：角色切换作为 Ability

```
问：角色切换能做成 GameplayAbility 吗？
答：不能！

推理：
1. 切换角色需要移除当前角色的技能
2. 如果切换本身是技能，它会被自己移除
3. 移除后无法完成后续步骤
4. 逻辑自相矛盾 → 设计错误
```

识别方法：
- 画出执行流程图
- 检查是否有"自己操作自己"的情况
- 询问："如果我是这个对象，我能完成这个操作吗？"

### 经验 4：理解 UE5 组件生命周期

不同组件的生命周期：

| 组件 | 生命周期 | 何时创建 | 何时销毁 |
|------|---------|---------|---------|
| PlayerController | 玩家会话 | 玩家加入 | 玩家离开/游戏结束 |
| PlayerState | 玩家会话 | 玩家加入 | 玩家离开/游戏结束 |
| ASC (on PlayerState) | 玩家会话 | PlayerState 创建时 | PlayerState 销毁时 |
| Character | 关卡/重生 | Possess/Spawn | 死亡/离开关卡 |
| GameMode | 关卡 | 关卡加载 | 关卡卸载 |
| GameInstance | 游戏运行 | 游戏启动 | 游戏退出 |

关键理解：
- 角色可以死亡重生，但 PlayerState 持续存在
- ASC 应该在生命周期更长的对象上（PlayerState）
- 不要在短生命周期对象上存储需要持久化的数据

### 经验 5：编译错误是警告信号

30+ 编译错误的含义：
```
1-3 个错误 → 可能是小问题，可以修复
5-10 个错误 → 需要重新审视设计
10+ 个错误 → 设计方向可能完全错误
30+ 个错误 → 立即停止，回滚代码，重新理解需求
```

本次会话的错误分布：
- SaveGame 结构变更相关：~10 个
- InteractionInterface 重复定义：~8 个
- UCLASS Config 缺失：~3 个
- API 使用错误：~5 个
- 访问权限错误：~4 个

教训：
- 当错误数量激增时，不要一个一个修
- 应该停下来，审视整体设计
- 可能需要回滚并采用不同方法

### 经验 6：文档比代码更持久

本次会话的产出：
- 代码：100% 被回滚，没有任何保留
- 文档：100% 保留，并作为未来参考

文档的价值：
1. 记录失败经验，避免重复错误
2. 建立项目规范和最佳实践
3. 为未来开发者（人类和 AI）提供指导
4. 即使实现失败，知识得以保留

文档优先原则：
- 理解不确定时，先写文档总结理解
- 设计方案时，先写设计文档
- 遇到错误时，记录到文档
- 文档永远比临时代码更有价值

---

## 用户已有的关键实现

### 1. UMutableAppearanceComponent（外观管理）

位置： `Character/MutableAppearanceComponent.h/.cpp`

核心功能：
```cpp
void UMutableAppearanceComponent::SwitchGender(ECharacterGender NewGender)
{
    if (NewGender == ECharacterGender::MALE && MaleInstance)
    {
        BodyCSkeletalComponent->SetCustomizableObjectInstance(MaleInstance);
        HeadCSkeletalComponent->SetCustomizableObjectInstance(MaleInstance);
        HeadMesh->SetAnimInstanceClass(MaleAnimClassPtr);
    }
    else if (NewGender == ECharacterGender::FEMALE && FemaleInstance)
    {
        BodyCSkeletalComponent->SetCustomizableObjectInstance(FemaleInstance);
        HeadCSkeletalComponent->SetCustomizableObjectInstance(FemaleInstance);
        HeadMesh->SetAnimInstanceClass(FemaleAnimClassPtr);
    }

    ShootPlayerState->SetCharacterGender(NewGender);
}
```

重要性：
- 已经完整实现了 Mesh 和 AnimClass 的切换
- 不需要重新实现外观管理
- 只需要调用这个方法

### 2. ShootSaveGame（双主角存档系统）

位置： `System/ShootSaveGame.h`

核心结构：
```cpp
USTRUCT(BlueprintType)
struct FCharacterSnapshot
{
    GENERATED_BODY()

    UPROPERTY() int32 AttributePoints = 0;
    UPROPERTY() float Strength = 0;
    UPROPERTY() float Intelligence = 0;
    UPROPERTY() float Resilience = 0;
    UPROPERTY() float Vigor = 0;
    UPROPERTY() int32 SkillPoints = 0;
    UPROPERTY() TArray<FSavedAbility> SavedAbilities;
};

UCLASS()
class UShootSaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    // 当前角色
    UPROPERTY() ECharacterGender CurrentGender = ECharacterGender::MALE;

    // 两个主角的独立数据
    UPROPERTY() FCharacterSnapshot MaleSnapshot;
    UPROPERTY() FCharacterSnapshot FemaleSnapshot;

    // 共享数据
    UPROPERTY() int32 PlayerLevel = 1;
    UPROPERTY() int32 XP = 0;
};
```

重要性：
- 已经设计好双主角数据分离
- 使用 Snapshot 模式保存/恢复状态
- 不需要重新设计存档结构

### 3. Lyra 交互系统（完整实现）

位置： `Interaction/` 文件夹（19个文件）

核心接口：
```cpp
// IInteractableTarget.h
class IInteractableTarget
{
    GENERATED_BODY()
public:
    virtual void GatherInteractionOptions(
        const FInteractionQuery& InteractQuery,
        FInteractionOptionBuilder& OptionBuilder
    ) = 0;

    virtual void CustomizeInteractionEventData(
        const FGameplayTag& InteractionEventTag,
        FGameplayEventData& InOutEventData
    ) { }
};
```

系统组件：
- `IInteractableTarget` - 可交互对象实现此接口
- `IInteractionInstigator` - 发起交互的对象（通常是 Player）
- `FInteractionOption` - 描述一个交互选项（可能有多个）
- `FInteractionQuery` - 查询交互的上下文信息
- `AbilityTask_WaitForInteractableTargets` - 等待可交互目标出现
- `AbilityTask_GrantNearbyInteraction` - 授予附近对象的交互能力
- `ShootGameplayAbility_Interact` - 执行交互的 Ability
- `GameplayAbilityTargetActor_Interact` - 选择交互目标
- `InteractionStatics` - 交互相关的静态工具函数

重要性：
- 这是 Epic Games Lyra 项目的完整交互系统
- 已经过充分测试和优化
- 支持复杂的交互场景（多选项、条件判断、能力授予）
- 绝对不要重新实现或创建重复接口

### 4. FShootGameplayTags（Tag 管理单例）

位置： `AbilitySystem/ShootGameplayTags.h/.cpp`

使用方式：
```cpp
// 定义
struct FShootGameplayTags
{
    FGameplayTag Ability_Character_Male;
    FGameplayTag Ability_Character_Female;
    // ...

    static const FShootGameplayTags& Get() { return GameplayTags; }
};

// 初始化（在 .cpp 中）
void FShootGameplayTags::InitializeNativeGameplayTags()
{
    GameplayTags.Ability_Character_Male =
        UGameplayTagsManager::Get().AddNativeGameplayTag(
            FName("Ability.Character.Male"),
            FString("Abilities specific to male protagonist")
        );
}

// 使用
const FGameplayTag& MaleTag = FShootGameplayTags::Get().Ability_Character_Male;
```

重要性：
- 项目标准的 Tag 管理方式
- 所有新 Tag 必须添加到这里
- 不要在运行时使用 `RequestGameplayTag`

---

## 未实现的功能（留给未来）

### 角色切换系统 - 正确的设计思路

应该在哪里实现：
- 在 `AShootCharacter` 中添加 `SwitchToGender(ECharacterGender NewGender)` 方法
- 或在 `AShootPlayerState` 中实现（因为它持有 ASC）
- 绝对不要做成 GameplayAbility

实现步骤：

1. 保存当前角色状态
   ```cpp
   // 保存到 SaveGame 的对应 Snapshot
   SaveCurrentCharacterData(CurrentGender);
   ```

2. 切换外观
   ```cpp
   // 使用现有的 MutableAppearanceComponent
   AppearanceComponent->SwitchGender(NewGender);
   ```

3. 移除当前角色技能
   ```cpp
   // 从 ASC 移除带有当前性别 Tag 的技能
   RemoveAbilitiesByTag(CurrentGenderTag);
   ```

4. 加载新角色数据
   ```cpp
   // 从 SaveGame 的对应 Snapshot 加载
   LoadCharacterData(NewGender);
   ```

5. 添加新角色技能
   ```cpp
   // 向 ASC 添加新性别的技能
   GrantAbilitiesByTag(NewGenderTag);
   ```

6. 更新属性值
   ```cpp
   // 从 Snapshot 恢复属性
   AttributeSet->InitStrength(Snapshot.Strength);
   AttributeSet->InitIntelligence(Snapshot.Intelligence);
   // ...
   ```

需要注意的陷阱：
- 不要销毁/重建任何组件
- 技能的 SpecHandle 需要正确管理（保存和恢复）
- 属性值用 `Init` 方法，不要用 `Set`（避免触发 GE）
- 切换过程要保证原子性（要么全成功，要么全回滚）

与现有系统的集成：
- 使用 `UMutableAppearanceComponent::SwitchGender()` - 外观
- 使用 `FCharacterSnapshot` - 数据保存
- 使用 `FShootGameplayTags::Get().Ability_Character_Male/Female` - Tag 管理
- 不要创建新的接口或系统

---

## 给未来开发者的建议

### 如果你是人类开发者

1. 这些文档是真实的失败经验
   - 每个错误都真实发生过
   - 每个警告都是用户的宝贵反馈
   - 遵循这些最佳实践可以节省大量时间

2. 优先理解现有代码
   - 项目已经有很多高质量实现（Lyra、用户代码）
   - 不要重复造轮子
   - 花时间理解比花时间重构更值得

3. Lyra 是优秀的参考
   - Epic Games 的设计经过深思熟虑
   - Interaction 系统、Ability 系统都可以直接使用
   - 需要扩展时，遵循 Lyra 的模式

### 如果你是 AI 助手

1. 永远先检查现有代码
   ```
   收到任务 → 列出相关文件 → 阅读核心代码 → 询问确认 → 开始实现
   不要跳过前面任何步骤！
   ```

2. 遵循用户的警告
   - 用户的警告通常指向根本性问题
   - 第一次警告 → 立即调整方法
   - 如果收到第二次警告 → 完全停止，重新理解需求
   - 绝对不要让用户给出第三次警告

3. 识别自相矛盾的设计
   - 画流程图检查逻辑
   - 如果一个对象需要"操作自己"，可能有问题
   - 询问用户确认设计方向

4. 编译错误是信号
   - 1-3 个错误：正常，修复即可
   - 10+ 个错误：停止，重新审视设计
   - 30+ 个错误：立即回滚，完全错误的方向

5. 文档优先于代码
   - 不确定时，先写文档总结理解
   - 失败的实现也要留下文档
   - 文档是持久的知识资产

6. 谦逊和诚实
   - 不要假装理解
   - 不要说"编译通过了"如果没有真的编译
   - 承认错误比掩盖错误更重要

---

## 总结

本次会话是一次完全失败的实现尝试，但产生了极其宝贵的文档资产：

失败的原因：
1. 没有理解现有架构就开始编码
2. 忽视了用户的三次明确警告
3. 重复造轮子而非使用现有系统
4. 犯了根本性的架构错误

保留的价值：
1. 详细的错误案例和分析（COMMON_MISTAKES.md）
2. 完整的会话记录和反思（本文档）
3. 正确的设计思路和最佳实践
4. 对项目现有系统的深入理解

最重要的教训：

> 先理解，再修改。永远。
>
> 花 2 小时理解现有代码，胜过花 10 小时修复错误实现。
>
> 用户的警告是项目知识的精华，忽视警告就是忽视项目最宝贵的资产。

给未来的建议：

对于角色切换功能，正确的第一步不是写代码，而是：

1. 列出 `Character/`, `AbilitySystem/`, `System/`, `Interaction/` 文件夹的所有文件
2. 阅读 `UMutableAppearanceComponent`, `ShootSaveGame`, `IInteractableTarget` 的实现
3. 理解 ASC 在 PlayerState 上的生命周期
4. 画出组件关系图和数据流图
5. 向用户确认理解是否正确
6. 只有在以上全部完成后，才开始写第一行代码

---

文档版本： 1.0
最后更新： 2025-10-31
维护者： Claude Code
状态： 最终版本（会话已结束，代码已全部回滚）
