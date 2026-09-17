# 角色切换系统设计指南

项目： NewWorldOrder - 双主角系统（陈浩宇 / 沈芸皖）
版本： 1.0
状态： 设计文档（未实现）
最后更新： 2025-10-31

---

## 文档目的

本文档提供角色切换系统的正确设计方案，基于：
1. 对项目现有架构的深入理解
2. 从失败实现中吸取的教训
3. UE5 + GAS（Gameplay Ability System）的最佳实践
4. Epic Games Lyra 项目的设计模式

重要： 实现前必须阅读 `COMMON_MISTAKES.md` 和 `SESSION_SUMMARY.md`，了解哪些方法绝对不能用。

---

## 目录

1. [系统概述](#系统概述)
2. [架构原则](#架构原则)
3. [现有系统分析](#现有系统分析)
4. [设计方案](#设计方案)
5. [实现步骤](#实现步骤)
6. [常见陷阱](#常见陷阱)
7. [测试策略](#测试策略)

---

## 系统概述

### 需求描述

双主角系统：
- 男主角： 陈浩宇 - 独立的外观、技能、属性
- 女主角： 沈芸皖 - 独立的外观、技能、属性
- 共享数据： 等级、经验值、剧情进度
- 切换能力： 玩家可以在特定场景切换控制的角色

### 核心特性

1. 无缝切换
   - 不销毁和重建任何核心组件
   - 切换过程平滑，无明显延迟
   - 保持所有系统状态（网络、输入、UI）

2. 独立进度
   - 每个角色有独立的技能树
   - 每个角色有独立的属性值
   - 每个角色有独立的装备和物品

3. 数据持久化
   - 切换时自动保存当前角色状态
   - 切换后自动加载目标角色状态
   - 支持存档系统的完整集成

4. 技能系统集成
   - 基于 GAS（Gameplay Ability System）
   - 使用 GameplayTag 区分角色技能
   - 动态移除/添加技能，保持 SpecHandle

---

## 架构原则

### 原则 1：共享组件架构（最重要）

核心理念： 角色切换是配置切换，不是对象切换

```
固定不变的组件（整个游戏会话）：
├─ APlayerController          → 玩家输入控制
├─ APlayerState                → 玩家数据和网络同步
│   └─ UAbilitySystemComponent → GAS 核心（技能、属性、效果）
│       └─ UShootAttributeSet  → 属性值（生命、法力、力量等）
└─ AShootCharacter             → 角色实体
    └─ UMutableAppearanceComponent → 外观管理

切换时改变的只是：
├─ 外观配置（Mesh + AnimClass）
├─ 技能配置（移除/添加 Abilities）
└─ 属性值（从 SaveGame 加载）
```

禁止的操作：
-  `Destroy(Character)` 或 `SpawnActor<ACharacter>()`
-  重新创建 PlayerState 或 AbilitySystemComponent
-  销毁和重建任何核心组件

为什么？
- PlayerController 持有输入绑定和网络连接
- PlayerState 持有 ASC，销毁会丢失所有 GE 和技能状态
- 重建开销巨大，且容易出现状态不一致

### 原则 2：单一职责分离

每个组件只负责自己的部分：

| 组件 | 职责 | 切换时的行为 |
|------|------|------------|
| `UMutableAppearanceComponent` | 外观管理 | 调用 `SwitchGender(NewGender)` |
| `UAbilitySystemComponent` | 技能管理 | 移除旧技能，添加新技能 |
| `UShootAttributeSet` | 属性管理 | 从 SaveGame 加载新属性值 |
| `UShootSaveGame` | 数据持久化 | 保存旧角色，加载新角色 |
| `AShootPlayerState` | 状态协调 | 协调以上所有组件的切换 |

不要让一个组件做太多事情，也不要让多个组件做同一件事。

### 原则 3：数据驱动设计

使用 GameplayTag 区分角色：

```cpp
// FShootGameplayTags.h
struct FShootGameplayTags
{
    // 角色标识
    FGameplayTag Ability_Character_Male;      // "Ability.Character.Male"
    FGameplayTag Ability_Character_Female;    // "Ability.Character.Female"

    // 用于标记技能属于哪个角色
    FGameplayTag Ability_Exclusive_Male;      // 男主专属技能
    FGameplayTag Ability_Exclusive_Female;    // 女主专属技能
    FGameplayTag Ability_Shared;              // 共享技能（两个角色都有）
};
```

使用数据表配置技能：

```cpp
// 每个角色的初始技能配置
UPROPERTY(EditDefaultsOnly, Category="Character|Male")
TArray<TSubclassOf<UGameplayAbility>> MaleStartingAbilities;

UPROPERTY(EditDefaultsOnly, Category="Character|Female")
TArray<TSubclassOf<UGameplayAbility>> FemaleStartingAbilities;
```

### 原则 4：原子性操作

切换必须是原子的：

```cpp
bool SwitchCharacter(ECharacterGender NewGender)
{
    // 1. 验证阶段（所有检查都通过才继续）
    if (!ValidateSwitchPreconditions(NewGender))
        return false;

    // 2. 保存阶段（保存当前状态）
    if (!SaveCurrentCharacterState())
    {
        // 保存失败，中止切换
        return false;
    }

    // 3. 切换阶段（修改配置）
    FScopedTransaction Transaction("Switch Character");
    {
        SwitchAppearance(NewGender);
        SwitchAbilities(NewGender);
        SwitchAttributes(NewGender);
    }

    // 4. 验证阶段（确认切换成功）
    if (!ValidateSwitchSuccess(NewGender))
    {
        // 切换失败，回滚到之前的状态
        RollbackSwitch();
        return false;
    }

    return true;
}
```

要么全部成功，要么全部回滚。不要留下半切换状态。

---

## 现有系统分析

### 1. UMutableAppearanceComponent（外观管理）

文件位置： `Character/MutableAppearanceComponent.h/.cpp`

已实现功能：
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

分析：
-  完整实现了 Mesh 和 AnimClass 切换
-  使用 Mutable 的 CustomizableObject 系统
-  自动更新 PlayerState 中的性别标记
-  直接可用，不需要修改

使用方式：
```cpp
// 在角色切换时调用
MutableAppearanceComponent->SwitchGender(ECharacterGender::FEMALE);
```

### 2. ShootSaveGame（双主角存档）

文件位置： `System/ShootSaveGame.h`

已实现结构：
```cpp
// 角色快照 - 保存单个角色的所有数据
USTRUCT(BlueprintType)
struct FCharacterSnapshot
{
    GENERATED_BODY()

    // 属性点
    UPROPERTY() int32 AttributePoints = 0;

    // 基础属性
    UPROPERTY() float Strength = 0;
    UPROPERTY() float Intelligence = 0;
    UPROPERTY() float Resilience = 0;
    UPROPERTY() float Vigor = 0;

    // 技能点
    UPROPERTY() int32 SkillPoints = 0;

    // 已保存的技能
    UPROPERTY() TArray<FSavedAbility> SavedAbilities;
};

// 存档主类
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

    // ... 其他共享数据（剧情进度、地图解锁等）
};
```

分析：
-  完美的双主角数据分离设计
-  使用 Snapshot 模式便于保存/恢复
-  清晰区分独立数据和共享数据
-  直接可用，不需要修改结构

使用方式：
```cpp
// 保存当前角色
void SaveCurrentCharacter(ECharacterGender Gender)
{
    FCharacterSnapshot& Snapshot = (Gender == ECharacterGender::MALE)
        ? SaveGame->MaleSnapshot
        : SaveGame->FemaleSnapshot;

    Snapshot.Strength = AttributeSet->GetStrength();
    Snapshot.Intelligence = AttributeSet->GetIntelligence();
    // ... 保存其他属性和技能
}

// 加载目标角色
void LoadTargetCharacter(ECharacterGender Gender)
{
    const FCharacterSnapshot& Snapshot = (Gender == ECharacterGender::MALE)
        ? SaveGame->MaleSnapshot
        : SaveGame->FemaleSnapshot;

    AttributeSet->InitStrength(Snapshot.Strength);
    AttributeSet->InitIntelligence(Snapshot.Intelligence);
    // ... 加载其他属性和技能
}
```

### 3. Lyra 交互系统

文件位置： `Interaction/` 文件夹（19个文件）

核心组件：

| 文件 | 用途 |
|------|------|
| `IInteractableTarget.h` | 可交互对象接口 |
| `IInteractionInstigator.h` | 交互发起者接口 |
| `InteractionOption.h` | 交互选项定义 |
| `InteractionQuery.h` | 交互查询上下文 |
| `AbilityTask_WaitForInteractableTargets.h/.cpp` | 等待可交互目标 |
| `AbilityTask_GrantNearbyInteraction.h/.cpp` | 授予附近交互 |
| `ShootGameplayAbility_Interact.h/.cpp` | 交互技能 |
| `GameplayAbilityTargetActor_Interact.h/.cpp` | 目标选择 |
| `InteractionStatics.h/.cpp` | 静态工具函数 |

设计模式：
```cpp
// 1. 可交互对象实现接口
class ANPCCharacter : public IInteractableTarget
{
    virtual void GatherInteractionOptions(
        const FInteractionQuery& InteractQuery,
        FInteractionOptionBuilder& OptionBuilder) override
    {
        FInteractionOption Option;
        Option.Text = FText::FromString("切换角色");
        Option.InteractionAbilityToGrant = UGA_SwitchCharacter::StaticClass();
        OptionBuilder.AddOption(Option);
    }
};

// 2. 玩家靠近时，AbilityTask 检测到目标
// 3. 收集交互选项
// 4. 显示 UI 提示
// 5. 玩家按键后，授予并激活交互 Ability
// 6. Ability 执行实际的交互逻辑
```

分析：
-  Epic Games 完整实现，经过充分测试
-  支持复杂交互场景（多选项、条件判断）
-  与 GAS 完美集成
-  不要创建新的交互接口
-  如果需要角色切换交互，使用此系统

### 4. FShootGameplayTags（Tag 管理）

文件位置： `AbilitySystem/ShootGameplayTags.h/.cpp`

现有 Tag 结构：
```cpp
struct FShootGameplayTags
{
    // 输入 Tag
    FGameplayTag InputTag_LMB;
    FGameplayTag InputTag_RMB;
    FGameplayTag InputTag_1;
    FGameplayTag InputTag_2;
    // ...

    // 技能 Tag
    FGameplayTag Abilities_Attack;
    FGameplayTag Abilities_Summon;
    // ...

    // 获取单例
    static const FShootGameplayTags& Get() { return GameplayTags; }

    // 初始化（在模块启动时调用）
    static void InitializeNativeGameplayTags();

private:
    static FShootGameplayTags GameplayTags;
};
```

需要添加的 Tag：
```cpp
// 在 ShootGameplayTags.h 中添加
FGameplayTag Ability_Character_Male;
FGameplayTag Ability_Character_Female;

// 在 ShootGameplayTags.cpp 的 InitializeNativeGameplayTags() 中注册
GameplayTags.Ability_Character_Male = UGameplayTagsManager::Get().AddNativeGameplayTag(
    FName("Ability.Character.Male"),
    FString("Abilities specific to male protagonist Chen Haoyu")
);

GameplayTags.Ability_Character_Female = UGameplayTagsManager::Get().AddNativeGameplayTag(
    FName("Ability.Character.Female"),
    FString("Abilities specific to female protagonist Shen Yunwan")
);
```

---

## 设计方案

### 方案概述

实现位置： 在 `AShootPlayerState` 中添加角色切换逻辑

为什么在 PlayerState？
1. PlayerState 持有 AbilitySystemComponent
2. PlayerState 的生命周期覆盖整个游戏会话
3. PlayerState 负责网络同步（支持未来多人模式）
4. PlayerState 是"玩家数据"的自然归属

核心方法：
```cpp
// ShootPlayerState.h
class AShootPlayerState : public APlayerState
{
public:
    // 切换角色（公共接口）
    UFUNCTION(BlueprintCallable, Category="Character")
    bool SwitchToCharacter(ECharacterGender NewGender);

    // 获取当前角色性别
    UFUNCTION(BlueprintPure, Category="Character")
    ECharacterGender GetCurrentGender() const { return CurrentGender; }

private:
    // 当前角色性别
    UPROPERTY(Replicated)
    ECharacterGender CurrentGender;

    // 内部实现方法
    bool ValidateSwitchPreconditions(ECharacterGender NewGender);
    void SaveCurrentCharacterData();
    void LoadTargetCharacterData(ECharacterGender TargetGender);
    void SwitchAbilities(ECharacterGender OldGender, ECharacterGender NewGender);
    void SwitchAttributes(ECharacterGender TargetGender);
    void SwitchAppearance(ECharacterGender TargetGender);
    void BroadcastSwitchEvent(ECharacterGender NewGender);
};
```

### 数据流图

```
用户触发切换请求
    ↓
AShootPlayerState::SwitchToCharacter(NewGender)
    ↓
┌─────────────────────────────────────────┐
│ 1. 验证前提条件                          │
│    - 新旧性别是否不同                     │
│    - 是否在允许切换的场景                 │
│    - 角色是否处于可切换状态               │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ 2. 保存当前角色数据                      │
│    ┌─────────────────────────┐          │
│    │ SaveGame->MaleSnapshot   │          │
│    │ 或 FemaleSnapshot         │          │
│    └─────────────────────────┘          │
│    - 属性值（Strength, Int, etc）       │
│    - 技能列表（Abilities + SpecHandle） │
│    - 装备和物品                          │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ 3. 移除当前角色技能                      │
│    ┌─────────────────────────┐          │
│    │ AbilitySystemComponent   │          │
│    └─────────────────────────┘          │
│    - 查找带有 OldGenderTag 的技能       │
│    - 移除这些技能（ClearAbility）       │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ 4. 切换外观                             │
│    ┌─────────────────────────┐          │
│    │ MutableAppearanceComp    │          │
│    └─────────────────────────┘          │
│    - SwitchGender(NewGender)            │
│    - 自动切换 Mesh + AnimClass          │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ 5. 加载目标角色数据                      │
│    ┌─────────────────────────┐          │
│    │ SaveGame->Snapshot       │          │
│    └─────────────────────────┘          │
│    - 读取属性值                          │
│    - 读取技能列表                        │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ 6. 初始化目标角色属性                    │
│    ┌─────────────────────────┐          │
│    │ ShootAttributeSet        │          │
│    └─────────────────────────┘          │
│    - InitStrength(Snapshot.Strength)    │
│    - InitIntelligence(...)               │
│    - 使用 Init 方法，不触发 GE          │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ 7. 添加目标角色技能                      │
│    ┌─────────────────────────┐          │
│    │ AbilitySystemComponent   │          │
│    └─────────────────────────┘          │
│    - 遍历 Snapshot.SavedAbilities       │
│    - GiveAbility() 并恢复 SpecHandle    │
└─────────────────────────────────────────┘
    ↓
┌─────────────────────────────────────────┐
│ 8. 广播切换事件                          │
│    - UI 更新                            │
│    - 音效播放                            │
│    - 剧情触发                            │
└─────────────────────────────────────────┘
    ↓
切换完成
```

---

## 实现步骤

### 步骤 1：添加 GameplayTag 定义

文件： `ShootGameplayTags.h`

```cpp
// 在 FShootGameplayTags 结构体中添加
struct FShootGameplayTags
{
    // ... 现有 Tag ...

    // 角色标识 Tag
    FGameplayTag Ability_Character_Male;
    FGameplayTag Ability_Character_Female;

    // 获取单例
    static const FShootGameplayTags& Get() { return GameplayTags; }
    static void InitializeNativeGameplayTags();

private:
    static FShootGameplayTags GameplayTags;
};
```

文件： `ShootGameplayTags.cpp`

```cpp
void FShootGameplayTags::InitializeNativeGameplayTags()
{
    // ... 现有 Tag 初始化 ...

    // 角色标识 Tag
    GameplayTags.Ability_Character_Male = UGameplayTagsManager::Get().AddNativeGameplayTag(
        FName("Ability.Character.Male"),
        FString("Abilities and features specific to male protagonist Chen Haoyu")
    );

    GameplayTags.Ability_Character_Female = UGameplayTagsManager::Get().AddNativeGameplayTag(
        FName("Ability.Character.Female"),
        FString("Abilities and features specific to female protagonist Shen Yunwan")
    );
}
```

重要： 不要在运行时使用 `FGameplayTag::RequestGameplayTag()`！

### 步骤 2：在 PlayerState 添加切换方法声明

文件： `ShootPlayerState.h`

```cpp
UCLASS()
class AShootPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    // 切换角色（主接口）
    UFUNCTION(BlueprintCallable, Category="Character")
    bool SwitchToCharacter(ECharacterGender NewGender);

    // 获取当前角色性别
    UFUNCTION(BlueprintPure, Category="Character")
    ECharacterGender GetCurrentGender() const { return CurrentGender; }

    // 设置当前角色性别（仅在初始化时调用）
    void SetCharacterGender(ECharacterGender NewGender);

protected:
    // 当前角色性别
    UPROPERTY(ReplicatedUsing=OnRep_CurrentGender)
    ECharacterGender CurrentGender = ECharacterGender::MALE;

    UFUNCTION()
    void OnRep_CurrentGender();

private:
    // 验证切换前提条件（不超过 30 行）
    bool ValidateSwitchPreconditions(ECharacterGender NewGender) const;

    // 保存当前角色数据到 SaveGame（不超过 60 行）
    bool SaveCurrentCharacterData();

    // 从 SaveGame 加载目标角色数据（不超过 60 行）
    bool LoadTargetCharacterData(ECharacterGender TargetGender);

    // 切换技能（移除旧的，添加新的）（不超过 70 行）
    void SwitchAbilities(ECharacterGender OldGender, ECharacterGender NewGender);

    // 切换属性值（不超过 20 行）
    void SwitchAttributes(const FCharacterSnapshot& Snapshot);

    // 切换外观（不超过 20 行）
    void SwitchAppearance(ECharacterGender TargetGender);

    // 广播切换事件（不超过 15 行）
    void BroadcastCharacterSwitchEvent(ECharacterGender NewGender);

    // 获取当前角色的 Snapshot 引用
    FCharacterSnapshot& GetCurrentSnapshot();
    const FCharacterSnapshot& GetSnapshotForGender(ECharacterGender Gender) const;
};
```

注意： 每个方法的行数限制确保符合 Sonar 规则。

### 步骤 3：实现验证方法

文件： `ShootPlayerState.cpp`

```cpp
bool AShootPlayerState::ValidateSwitchPreconditions(ECharacterGender NewGender) const
{
    // 检查是否切换到相同性别
    if (CurrentGender == NewGender)
    {
        UE_LOG(LogTemp, Warning, TEXT("Already playing as %s character"),
            NewGender == ECharacterGender::MALE ? TEXT("male") : TEXT("female"));
        return false;
    }

    // 检查是否有有效的 Character
    const AShootCharacter* Character = Cast<AShootCharacter>(GetPawn());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("No valid character found for gender switch"));
        return false;
    }

    // 检查 AbilitySystemComponent
    if (!AbilitySystemComponent)
    {
        UE_LOG(LogTemp, Error, TEXT("No AbilitySystemComponent found"));
        return false;
    }

    // 检查 MutableAppearanceComponent
    UMutableAppearanceComponent* AppearanceComp =
        Character->FindComponentByClass<UMutableAppearanceComponent>();
    if (!AppearanceComp)
    {
        UE_LOG(LogTemp, Error, TEXT("No MutableAppearanceComponent found"));
        return false;
    }

    // 检查是否在允许切换的场景（可选）
    // 例如：不在战斗中、不在过场动画中等
    if (Character->IsInCombat())
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot switch character during combat"));
        return false;
    }

    return true;
}
```

行数： ~45 行（符合 ≤80 行要求）

### 步骤 4：实现保存方法

文件： `ShootPlayerState.cpp`

```cpp
bool AShootPlayerState::SaveCurrentCharacterData()
{
    // 获取 SaveGame
    UShootSaveGameSubsystem* SaveSubsystem =
        GetWorld()->GetGameInstance()->GetSubsystem<UShootSaveGameSubsystem>();
    if (!SaveSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get SaveGameSubsystem"));
        return false;
    }

    UShootSaveGame* SaveGame = SaveSubsystem->GetCurrentSaveGame();
    if (!SaveGame)
    {
        UE_LOG(LogTemp, Error, TEXT("No active save game"));
        return false;
    }

    // 获取当前角色的 Snapshot 引用
    FCharacterSnapshot& Snapshot = GetCurrentSnapshot();

    // 保存属性值
    if (UShootAttributeSet* AttributeSet = GetAttributeSet())
    {
        Snapshot.Strength = AttributeSet->GetStrength();
        Snapshot.Intelligence = AttributeSet->GetIntelligence();
        Snapshot.Resilience = AttributeSet->GetResilience();
        Snapshot.Vigor = AttributeSet->GetVigor();
        Snapshot.AttributePoints = AttributeSet->GetAttributePoints();
        Snapshot.SkillPoints = AttributeSet->GetSkillPoints();
    }

    // 保存技能列表
    Snapshot.SavedAbilities.Empty();
    if (AbilitySystemComponent)
    {
        for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
        {
            FSavedAbility SavedAbility;
            SavedAbility.AbilityClass = Spec.Ability->GetClass();
            SavedAbility.AbilityLevel = Spec.Level;

            // 保存技能的 Tag 信息
            for (const FGameplayTag& Tag : Spec.GetDynamicSpecSourceTags())
            {
                const FString TagString = Tag.ToString();

                if (TagString.StartsWith(TEXT("Abilities.Status")))
                {
                    SavedAbility.AbilityStatus = Tag;
                }
                else if (TagString.StartsWith(TEXT("Abilities.Type")))
                {
                    SavedAbility.AbilityType = Tag;
                }
                else if (TagString.StartsWith(TEXT("InputTag")))
                {
                    SavedAbility.AbilitySlot = Tag;
                }
            }

            Snapshot.SavedAbilities.Add(SavedAbility);
        }
    }

    return true;
}

FCharacterSnapshot& AShootPlayerState::GetCurrentSnapshot()
{
    UShootSaveGameSubsystem* SaveSubsystem =
        GetWorld()->GetGameInstance()->GetSubsystem<UShootSaveGameSubsystem>();
    UShootSaveGame* SaveGame = SaveSubsystem->GetCurrentSaveGame();

    return (CurrentGender == ECharacterGender::MALE)
        ? SaveGame->MaleSnapshot
        : SaveGame->FemaleSnapshot;
}

const FCharacterSnapshot& AShootPlayerState::GetSnapshotForGender(ECharacterGender Gender) const
{
    UShootSaveGameSubsystem* SaveSubsystem =
        GetWorld()->GetGameInstance()->GetSubsystem<UShootSaveGameSubsystem>();
    const UShootSaveGame* SaveGame = SaveSubsystem->GetCurrentSaveGame();

    return (Gender == ECharacterGender::MALE)
        ? SaveGame->MaleSnapshot
        : SaveGame->FemaleSnapshot;
}
```

行数： ~75 行（符合要求）

### 步骤 5：实现技能切换方法

文件： `ShootPlayerState.cpp`

```cpp
void AShootPlayerState::SwitchAbilities(ECharacterGender OldGender, ECharacterGender NewGender)
{
    if (!AbilitySystemComponent)
    {
        return;
    }

    // 获取性别 Tag
    const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();
    const FGameplayTag OldGenderTag = (OldGender == ECharacterGender::MALE)
        ? GameplayTags.Ability_Character_Male
        : GameplayTags.Ability_Character_Female;

    // 移除当前角色的技能
    TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;
    for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
    {
        if (Spec.Ability && Spec.Ability->AbilityTags.HasTag(OldGenderTag))
        {
            AbilitiesToRemove.Add(Spec.Handle);
        }
    }

    for (const FGameplayAbilitySpecHandle& Handle : AbilitiesToRemove)
    {
        AbilitySystemComponent->ClearAbility(Handle);
    }

    // 添加新角色的技能
    const FCharacterSnapshot& NewSnapshot = GetSnapshotForGender(NewGender);
    for (const FSavedAbility& SavedAbility : NewSnapshot.SavedAbilities)
    {
        if (!SavedAbility.AbilityClass)
        {
            continue;
        }

        // 创建 AbilitySpec
        FGameplayAbilitySpec AbilitySpec(
            SavedAbility.AbilityClass,
            SavedAbility.AbilityLevel,
            INDEX_NONE,
            this
        );

        // 添加技能的 Tag
        if (SavedAbility.AbilityStatus.IsValid())
        {
            AbilitySpec.DynamicAbilityTags.AddTag(SavedAbility.AbilityStatus);
        }
        if (SavedAbility.AbilityType.IsValid())
        {
            AbilitySpec.DynamicAbilityTags.AddTag(SavedAbility.AbilityType);
        }
        if (SavedAbility.AbilitySlot.IsValid())
        {
            AbilitySpec.DynamicAbilityTags.AddTag(SavedAbility.AbilitySlot);
        }

        // 授予技能
        AbilitySystemComponent->GiveAbility(AbilitySpec);
    }
}
```

行数： ~60 行（符合要求）

### 步骤 6：实现属性和外观切换

文件： `ShootPlayerState.cpp`

```cpp
void AShootPlayerState::SwitchAttributes(const FCharacterSnapshot& Snapshot)
{
    UShootAttributeSet* AttributeSet = GetAttributeSet();
    if (!AttributeSet)
    {
        return;
    }

    // 使用 Init 方法设置属性值（不触发 GameplayEffect）
    AttributeSet->InitStrength(Snapshot.Strength);
    AttributeSet->InitIntelligence(Snapshot.Intelligence);
    AttributeSet->InitResilience(Snapshot.Resilience);
    AttributeSet->InitVigor(Snapshot.Vigor);
    AttributeSet->InitAttributePoints(Snapshot.AttributePoints);
    AttributeSet->InitSkillPoints(Snapshot.SkillPoints);
}

void AShootPlayerState::SwitchAppearance(ECharacterGender TargetGender)
{
    AShootCharacter* Character = Cast<AShootCharacter>(GetPawn());
    if (!Character)
    {
        return;
    }

    UMutableAppearanceComponent* AppearanceComp =
        Character->FindComponentByClass<UMutableAppearanceComponent>();
    if (AppearanceComp)
    {
        AppearanceComp->SwitchGender(TargetGender);
    }
}
```

行数： 每个方法 ~15 行（符合要求）

### 步骤 7：实现主切换方法

文件： `ShootPlayerState.cpp`

```cpp
bool AShootPlayerState::SwitchToCharacter(ECharacterGender NewGender)
{
    // 1. 验证前提条件
    if (!ValidateSwitchPreconditions(NewGender))
    {
        UE_LOG(LogTemp, Warning, TEXT("Character switch validation failed"));
        return false;
    }

    // 2. 保存当前角色数据
    if (!SaveCurrentCharacterData())
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to save current character data"));
        return false;
    }

    const ECharacterGender OldGender = CurrentGender;

    // 3. 切换技能
    SwitchAbilities(OldGender, NewGender);

    // 4. 切换外观
    SwitchAppearance(NewGender);

    // 5. 加载目标角色数据
    const FCharacterSnapshot& NewSnapshot = GetSnapshotForGender(NewGender);

    // 6. 切换属性
    SwitchAttributes(NewSnapshot);

    // 7. 更新当前性别
    CurrentGender = NewGender;

    // 8. 广播切换事件
    BroadcastCharacterSwitchEvent(NewGender);

    UE_LOG(LogTemp, Log, TEXT("Successfully switched to %s character"),
        NewGender == ECharacterGender::MALE ? TEXT("male") : TEXT("female"));

    return true;
}

void AShootPlayerState::BroadcastCharacterSwitchEvent(ECharacterGender NewGender)
{
    // 触发委托或事件
    OnCharacterSwitched.Broadcast(NewGender);

    // 可以在这里添加：
    // - UI 更新
    // - 音效播放
    // - 剧情触发
    // - 成就解锁等
}
```

行数： ~45 行（符合要求）

### 步骤 8：添加网络复制支持（可选）

文件： `ShootPlayerState.cpp`

```cpp
void AShootPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AShootPlayerState, CurrentGender);
}

void AShootPlayerState::OnRep_CurrentGender()
{
    // 在客户端收到性别变化通知时调用
    // 可以用于更新 UI 或播放特效
    OnCharacterSwitched.Broadcast(CurrentGender);
}
```

---

## 常见陷阱

### 陷阱 1：角色切换做成 GameplayAbility

错误示例：
```cpp
//  绝对不要这样做！
UCLASS()
class UGA_CharacterSwitch : public UGameplayAbility
{
    virtual void ActivateAbility(...) override
    {
        // 切换技能会移除当前角色的所有技能
        RemoveCurrentAbilities(); // ← 这会移除 GA_CharacterSwitch 自己！

        // 下面的代码永远不会执行！
        AddNewAbilities(); // ← 永远不会到达
        EndAbility();      // ← 永远不会到达
    }
};
```

为什么错误：
- 技能系统是被切换的对象，不是执行切换的主体
- 切换时会移除技能，Ability 会移除自己

正确做法：
- 在 PlayerState、Character 或 Controller 中实现
- 作为普通方法，不是 Ability

### 陷阱 2：销毁和重建 Character

错误示例：
```cpp
//  绝对不要这样做！
void SwitchCharacter(ECharacterGender NewGender)
{
    // 销毁当前 Character
    AShootCharacter* OldCharacter = GetPawn();
    OldCharacter->Destroy();

    // 生成新 Character
    AShootCharacter* NewCharacter = GetWorld()->SpawnActor<AShootCharacter>(...);
    Controller->Possess(NewCharacter);
}
```

为什么错误：
- PlayerController、PlayerState、ASC 都会丢失引用
- 大量初始化开销
- 网络同步问题
- 存档/读档逻辑复杂化

正确做法：
- 复用所有组件
- 只切换配置（Mesh、Abilities、Attributes）

### 陷阱 3：在运行时使用 RequestGameplayTag

错误示例：
```cpp
//  不要在运行时使用
FGameplayTag MaleTag = FGameplayTag::RequestGameplayTag(FName("Ability.Character.Male"));
```

为什么错误：
- 如果 Tag 未定义，会返回无效 Tag
- 每次调用都查询 TagManager，性能差
- Tag 字符串散布在代码各处，难以维护

正确做法：
```cpp
//  使用单例
const FGameplayTag& MaleTag = FShootGameplayTags::Get().Ability_Character_Male;
```

### 陷阱 4：使用 Set 方法修改属性

错误示例：
```cpp
//  不要直接 Set
AttributeSet->SetStrength(NewValue); // 会触发 GE 和网络同步
```

为什么错误：
- `SetStrength()` 会触发 `OnRep` 和 GE 计算
- 可能导致不必要的网络流量
- 可能触发属性变化的副作用

正确做法：
```cpp
//  使用 Init 方法（仅设置值，不触发副作用）
AttributeSet->InitStrength(NewValue);
```

### 陷阱 5：函数复杂度过高

错误示例：
```cpp
//  200+ 行的巨型函数
void SwitchCharacter(ECharacterGender NewGender)
{
    // 100 行验证逻辑
    // 50 行保存逻辑
    // 30 行切换逻辑
    // 20 行加载逻辑
    // 嵌套 5 层 if/for
}
```

为什么错误：
- 违反 Sonar 规则（≤80 行，≤3 嵌套）
- 难以测试和维护
- 认知复杂度过高

正确做法：
- 拆分为多个小函数
- 每个函数职责单一
- 主函数只做协调

### 陷阱 6：没有检查现有实现

错误示例：
```cpp
//  重新实现已存在的功能
void SwitchMesh(USkeletalMesh* NewMesh)
{
    // 自己写 Mesh 切换逻辑
    // 但 UMutableAppearanceComponent::SwitchGender() 已经实现了！
}
```

为什么错误：
- 浪费时间
- 可能引入 Bug
- 与现有系统不一致

正确做法：
- 实现前先列出相关文件夹的所有文件
- 阅读现有代码
- 复用现有功能

---

## 测试策略

### 单元测试

```cpp
// 测试切换逻辑
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSwitchCharacterTest,
    "Game.Character.SwitchCharacter",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter
)

bool FSwitchCharacterTest::RunTest(const FString& Parameters)
{
    // 1. 设置测试环境
    UWorld* World = CreateTestWorld();
    AShootPlayerState* PlayerState = CreateTestPlayerState(World);

    // 2. 初始状态检查
    TestEqual("Initial gender should be MALE",
        PlayerState->GetCurrentGender(), ECharacterGender::MALE);

    // 3. 执行切换
    bool bSuccess = PlayerState->SwitchToCharacter(ECharacterGender::FEMALE);
    TestTrue("Switch should succeed", bSuccess);

    // 4. 验证切换结果
    TestEqual("Gender should be FEMALE after switch",
        PlayerState->GetCurrentGender(), ECharacterGender::FEMALE);

    // 5. 验证数据保存
    // ... 检查 SaveGame 中的数据

    // 6. 清理
    CleanupTestWorld(World);

    return true;
}
```

### 集成测试

测试场景：

1. 基本切换
   - 从男主切换到女主
   - 从女主切换到男主
   - 连续切换多次

2. 数据持久化
   - 切换前修改属性值
   - 切换后验证旧角色数据已保存
   - 切换回来验证数据正确恢复

3. 技能管理
   - 男主学习技能后切换
   - 女主不应该有男主的技能
   - 切换回男主，技能应该恢复

4. 边界情况
   - 在战斗中尝试切换（应该失败）
   - 在过场动画中切换（应该失败）
   - 切换到相同性别（应该失败但不崩溃）

5. 网络测试（如果支持多人）
   - 服务器切换角色
   - 客户端正确接收通知
   - UI 正确更新

### 性能测试

测量指标：

- 切换操作的总耗时（目标：< 100ms）
- 内存分配（目标：无明显内存泄漏）
- 技能移除/添加的数量和耗时
- 属性初始化的耗时

优化建议：

- 使用对象池避免频繁 new/delete
- 批量操作技能而非逐个处理
- 异步加载资源（如果需要）

---

## 总结

### 关键要点

1. 共享组件架构
   - PlayerController、PlayerState、ASC、Character 都是复用的
   - 只切换配置，不销毁重建

2. 使用现有系统
   - `UMutableAppearanceComponent::SwitchGender()` - 外观
   - `FCharacterSnapshot` - 数据存储
   - `FShootGameplayTags` - Tag 管理
   - Lyra Interaction System - 交互（如果需要）

3. 遵循最佳实践
   - 函数 ≤80 行
   - 嵌套 ≤3 层
   - 每个函数职责单一
   - 优先理解现有代码，再动手实现

4. 避免致命错误
   -  角色切换做成 Ability
   -  销毁和重建 Character
   -  运行时 RequestGameplayTag
   -  重复实现已有功能
   -  不检查现有代码就开始写

### 实现前的检查清单

在开始实现之前，必须完成：

- [ ] 阅读 `COMMON_MISTAKES.md`（了解禁区）
- [ ] 阅读 `SESSION_SUMMARY.md`（了解失败案例）
- [ ] 列出 `Character/`, `AbilitySystem/`, `System/` 文件夹的所有文件
- [ ] 阅读 `UMutableAppearanceComponent` 的实现
- [ ] 阅读 `ShootSaveGame` 和 `FCharacterSnapshot` 的结构
- [ ] 理解 ASC 在 PlayerState 上的生命周期
- [ ] 画出组件关系图和数据流图
- [ ] 向用户确认理解是否正确
- [ ] 只有在以上全部完成后，才开始写代码

---

文档版本： 1.0
作者： Claude Code
审核： 待用户审核
状态： 设计阶段（未实现）

重要提醒： 本文档基于失败经验和深度反思编写。实现时请严格遵循本文档和相关错误文档，避免重蹈覆辙。
