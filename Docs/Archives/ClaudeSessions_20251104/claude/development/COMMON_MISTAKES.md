# 常见编码错误和最佳实践

## 文档说明

本文档记录了在 NewWorldOrder 项目开发过程中，AI 助手（Claude Code）在尝试实现功能时犯下的所有错误，以及从这些错误中总结出的最佳实践。

文档来源：
- 基于实际的失败实现经验
- 用户的多次纠正和警告
- 编译错误和架构问题的深度分析
- 对 Unreal Engine 5、GAS（Gameplay Ability System）、Lyra 项目的理解

文档目的：
1. 避免重复相同的错误
2. 建立项目级别的编码规范
3. 记录 UE5 + GAS 开发中的常见陷阱
4. 为未来的开发（包括人类和 AI）提供参考

重要提醒：
- 这些错误都是真实发生过的，并导致了编译失败或架构问题
- 每个错误都经过用户的明确指出和纠正
- 遵循这些最佳实践可以避免大量的返工

---

## 文档内容

## 目录
1. [GameplayTag 使用错误](#gameplaytag-使用错误)
2. [GameplayAbility 设计错误](#gameplayability-设计错误)
3. [类型转换过度约束](#类型转换过度约束)
4. [头文件循环依赖](#头文件循环依赖)
5. [函数复杂度过高](#函数复杂度过高)
6. [UCLASS Config 配置错误](#uclass-config-配置错误)
7. [未检查现有代码就随意修改](#未检查现有代码就随意修改)

---

## 1. GameplayTag 使用错误

### 错误示例：在运行时使用 FGameplayTag::RequestGameplayTag

```cpp
//  错误：在运行时或构造函数中使用 RequestGameplayTag
void SomeFunction()
{
    FGameplayTag GenderTag = FGameplayTag::RequestGameplayTag(FName("Ability.Character.Male"));

    if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Status"))))
    {
        // ...
    }
}
```

### 为什么这是错误的？

1. 构造函数中的问题：如果在构造函数中使用 `RequestGameplayTag`，必须确保该 tag 已在 `DefaultGameplayTags.ini` 中定义，否则会报错
2. 运行时的问题：如果 tag 未定义，会返回无效标签，导致逻辑错误
3. 性能问题：每次调用都需要查询 GameplayTagsManager，效率低下
4. 维护问题：tag 字符串散布在代码各处，难以维护和重构

### 正确做法

方案 1：使用 FShootGameplayTags 单例（推荐）

```cpp
//  正确：在 ShootGameplayTags.h 中定义
struct FShootGameplayTags
{
    FGameplayTag Ability_Character_Male;
    FGameplayTag Ability_Character_Female;
    // ...
};

//  正确：在 ShootGameplayTags.cpp 中初始化
void FShootGameplayTags::InitializeNativeGameplayTags()
{
    GameplayTags.Ability_Character_Male = UGameplayTagsManager::Get().AddNativeGameplayTag(
        FName("Ability.Character.Male"),
        FString("Abilities specific to male protagonist")
    );

    GameplayTags.Ability_Character_Female = UGameplayTagsManager::Get().AddNativeGameplayTag(
        FName("Ability.Character.Female"),
        FString("Abilities specific to female protagonist")
    );
}

//  正确：在代码中使用
void SomeFunction()
{
    const FGameplayTag GenderTag = FShootGameplayTags::Get().Ability_Character_Male;

    if (AbilityTags.HasTag(GenderTag))
    {
        // ...
    }
}
```

方案 2：使用字符串前缀匹配（用于匹配父标签）

```cpp
//  正确：使用字符串前缀匹配父标签类别
for (const FGameplayTag& Tag : AbilitySpec.GetDynamicSpecSourceTags())
{
    const FString TagString = Tag.ToString();

    // Match Abilities.Status.* (Locked/Eligible/Unlocked/Equipped)
    if (TagString.StartsWith(TEXT("Abilities.Status")))
    {
        SavedAbility.AbilityStatus = Tag;
    }
    // Match Abilities.Type.* (Offensive/Passive/None)
    else if (TagString.StartsWith(TEXT("Abilities.Type")))
    {
        SavedAbility.AbilityType = Tag;
    }
    // Match InputTag.* (LMB/RMB/1/2/3/4/etc)
    else if (TagString.StartsWith(TEXT("InputTag")))
    {
        SavedAbility.AbilitySlot = Tag;
    }
}
```

### 最佳实践

1. 所有 GameplayTag 都应在 FShootGameplayTags 单例中定义
2. 使用有意义的命名层级：例如 `Ability.Character.Male` 和 `Ability.Character.Female` 用于区分男女主角技能
3. 在 InitializeNativeGameplayTags() 中初始化所有原生标签
4. 通过 FShootGameplayTags::Get() 访问标签
5. 需要匹配父标签类别时，使用字符串前缀匹配

---

## 2. GameplayAbility 设计错误

### 错误示例：将角色切换实现为 GameplayAbility

```cpp
//  错误：将角色切换做成 GameplayAbility
UCLASS()
class UGA_CharacterSwitch : public UShootGameplayAbility
{
    virtual void ActivateAbility(...) override
    {
        // 1. 移除旧角色的技能
        RefreshCharacterAbilities();  // 这里会移除 GA_CharacterSwitch 自己！

        // 2. 加载新角色的技能
        // ...

        // 3. EndAbility 永远不会执行，因为技能已被移除
        EndAbility();  //  永远执行不到这里
    }
};
```

### 为什么这是错误的？

自相矛盾的执行流程：

```
1. 用户激活 GA_CharacterSwitch
2. Ability 开始执行 ActivateAbility()
3. Ability 调用 RefreshCharacterAbilities()
4. RefreshCharacterAbilities() 移除旧性别的所有技能
5. GA_CharacterSwitch 本身被移除（它也是一个技能！）
6. Ability 执行被中断 - 失败！
```

关键问题：
- 技能在执行过程中移除了自己
- `EndAbility()` 永远无法正常执行
- 可能导致状态不一致和内存泄漏

### 正确做法：实现为普通函数

```cpp
//  正确：在 AShootCharacter.h 中声明
class AShootCharacter : public ACharacter
{
public:
    // Character gender switching - NOT a GameplayAbility!
    // This is a regular function because switching would remove abilities mid-execution
    UFUNCTION(BlueprintCallable, Category = "Character")
    void SwitchCharacterGender();
};

//  正确：在 AShootCharacter.cpp 中实现
void AShootCharacter::SwitchCharacterGender()
{
    // 1. 检查是否允许切换（仅在 HomeMap）
    if (!GameMode->IsCharacterSwitchAllowed())
        return;

    // 2. 保存当前角色数据到 SaveGame
    SaveCurrentCharacterData();

    // 3. 移除当前性别的技能
    RemoveCurrentGenderAbilities();

    // 4. 切换外观（使用 MutableAppearanceComponent）
    AppearanceComponent->SwitchGender(NewGender);

    // 5. 加载新角色的数据
    LoadNewCharacterData();

    // 6. 授予新性别的技能
    GrantNewGenderAbilities();

    // 7. 保存到磁盘
    SaveGameSubsystem->WriteSaveGame(SaveGame);
}
```

### 设计原则

共享组件架构：
- `ACharacter`：共用，不销毁/重建
- `PlayerController`：共用，跨地图持久化
- `PlayerState`：共用，跨地图持久化
- `AbilitySystemComponent`：共用，挂在 PlayerState 上
- `UShootAttributeSet`：共用，挂在 ASC 上

切换时只改变：
1. 外观：通过 `UMutableAppearanceComponent->SwitchGender()`
2. 属性：保存/加载 Primary Attributes (Strength/Intelligence/Resilience/Vigor)
3. 技能：移除旧性别技能，授予新性别技能

### 何时使用 GameplayAbility vs 普通函数

使用 GameplayAbility 的场景：
-  玩家主动触发的战斗技能（攻击、跳跃、换弹等）
-  被动技能（持续生效的 buff/debuff）
-  需要消耗资源（Health/Mana/Stamina）的能力
-  需要 Cooldown 和 Cost 检查的能力
-  需要通过 Input Tag 绑定的能力

使用普通函数的场景：
-  修改 ASC 本身（添加/移除技能）
-  系统级操作（角色切换、存档加载）
-  不需要 GAS 框架特性的功能
-  需要在技能系统外部调用的逻辑

---

## 3. 类型转换过度约束

### 错误示例：过度使用具体子类 Cast

```cpp
//  错误：过度约束到具体子类
void SomeGameplayAbility::ActivateAbility(...)
{
    // 后续代码只用到了 GetPlayerState()，这是 APawn 的方法
    AShootCharacter* Character = Cast<AShootCharacter>(ActorInfo->AvatarActor.Get());
    if (Character)
    {
        AShootPlayerState* PS = Character->GetPlayerState<AShootPlayerState>();
        // 没有使用任何 AShootCharacter 的特有能力！
    }
}
```

### 为什么这是错误的？

#### 1. 设计层面的耦合和可复用性
- 把 `AvatarActor` 一上来就 `Cast<AShootCharacter>`，等于隐含约定：这里只有 `AShootCharacter` 才能进来
- GAS 的设计理念：`OwnerActor`/`AvatarActor` 本来就可能是不同类型
  - `OwnerActor=PlayerState`，`AvatarActor=Character/Pawn`
  - 甚至可能用在载具、AI Pawn、或其他 `Character` 派生类上
- 把类型钉死 = 代码失去弹性

#### 2. 正确性与失败路径
- `Cast<T>` 失败会返回 `nullptr`，并非崩溃
- 但你无端提高了"失败"的概率：
  - 如果某次能力挂在非 `AShootCharacter` 的 Avatar 上
  - 这里就走空指针分支，后续逻辑被不必要地阻断

#### 3. 性能不是主要矛盾，但风格是
- UE 的 `Cast<T>` 是基于反射/`IsA()` 的安全转换
- 运行时环境下通常是 O(1)
- 但：当你不需要子类能力却做子类转换，会让读代码的人误以为"下面要用到 `AShootCharacter` 的特性"，造成误导

#### 4. 最小必要抽象（Least Power Principle）
工程实践里，永远用满足需求的最小抽象：

| 需求 | 应该使用的类型 |
|------|--------------|
| 只用到移动/角色属性 | `ACharacter*` |
| 只用到控制/占位 | `APawn*` |
| 只读位置/标签 | `AActor*` |
| 需要跨类型收口 | 接口（如 `IAbilitySystemInterface`） |
| 确实需要子类特有方法 | `AShootCharacter*` |

### 正确做法

若只用到 `ACharacter` 的能力：
```cpp
//  正确：使用 ACharacter
ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
if (Character)
{
    AShootPlayerState* PS = Character->GetPlayerState<AShootPlayerState>();
    // ...
}
```

若只需要 `APawn` 的能力：
```cpp
//  正确：使用 APawn
APawn* Pawn = Cast<APawn>(ActorInfo->AvatarActor.Get());
if (Pawn)
{
    AShootPlayerState* PS = Pawn->GetPlayerState<AShootPlayerState>();
    // ...
}
```

遵循 GAS 常见写法：
```cpp
//  正确：使用 GAS 内置方法
AActor* Avatar = GetAvatarActorFromActorInfo();
if (const APawn* Pawn = Cast<APawn>(Avatar))
{
    if (AShootPlayerState* PS = Pawn->GetPlayerState<AShootPlayerState>())
    {
        // ...
    }
}
```

只在确实需要子类特有能力时才 Cast 到子类：
```cpp
//  正确：确实需要 AShootCharacter 的特有方法
AShootCharacter* ShootCharacter = Cast<AShootCharacter>(InteractingActor);
if (ShootCharacter)
{
    // 调用 AShootCharacter 的特有方法
    ShootCharacter->SwitchCharacterGender();  // 这是 AShootCharacter 独有的
}
```

### 最佳实践

1. 用得上什么能力，就用多宽的类型去接
2. 失败面更小、复用性更高、语义更清晰
3. 和 GAS 的解耦理念一致
4. `Cast` 本身不慢，但过度 `Cast` 会让设计变脆
5. 后续如果要让同一套能力支持载具、AI Pawn、甚至非 `Character` 的 Avatar，这个小改动会省掉一堆未来的返工

---

## 总结

### 核心原则

1. GameplayTag 管理
   - 所有标签在 FShootGameplayTags 单例中定义
   - 禁止在运行时使用 RequestGameplayTag
   - 使用字符串前缀匹配父标签类别

2. GameplayAbility 设计
   - 不要在 Ability 中修改 ASC 的技能列表（可能移除自己）
   - 系统级操作使用普通函数，不要滥用 GameplayAbility
   - 共享组件不销毁/重建，只切换数据和状态

3. 类型转换
   - 使用满足需求的最小抽象
   - 避免过度约束到具体子类
   - 提高代码灵活性和可复用性

### 总检查清单

在提交代码前，请完整检查以下所有项：

GameplayTag 使用
- [ ] 是否使用了 `FGameplayTag::RequestGameplayTag`？如果是，改为使用 FShootGameplayTags 单例
- [ ] 所有新的 GameplayTag 是否已在 FShootGameplayTags 中定义和初始化？
- [ ] 是否使用字符串前缀匹配来识别父标签类别？

GameplayAbility 设计
- [ ] 是否在 GameplayAbility 中修改了 ASC 的技能列表？如果是，考虑改为普通函数
- [ ] 代码逻辑是否会导致自相矛盾的执行流程（如技能移除自己）？
- [ ] 是否将系统级操作（角色切换、存档加载等）做成了 GameplayAbility？

类型转换
- [ ] 是否 Cast 到了具体子类但只使用了父类的能力？如果是，使用更宽的类型
- [ ] 是否遵循了最小必要抽象原则？

头文件管理
- [ ] 头文件中所有指针/引用类型是否都使用了前置声明？
- [ ] 是否将不必要的 include 从 .h 移到了 .cpp？
- [ ] `.generated.h` 是否是头文件中最后一个 include？
- [ ] 是否避免了"大而全"的聚合头（如 `Engine.h`）？
- [ ] UPROPERTY 指针是否使用了 `class` 关键字？

函数复杂度
- [ ] 每个函数行数 ≤ 80 行（包含空行和注释）？
- [ ] 每个函数嵌套层级 ≤ 3 层？
- [ ] 每个函数只做一件事（单一职责）？
- [ ] 是否使用了提前返回/continue 降低嵌套？
- [ ] 是否将复杂逻辑提炼成了辅助函数？
- [ ] 函数名清晰描述其功能？
- [ ] 每个函数是否都有清晰的注释说明职责？

UCLASS Config 配置
- [ ] `UPROPERTY(Config)` 的类是否在 `UCLASS()` 中指定了 `config=` 参数？
- [ ] `config=` 参数是否与目标 .ini 文件匹配（Engine/Game/Editor）？
- [ ] 是否添加了 `defaultconfig` 以支持保存默认值？
- [ ] 地图/资源类型是否使用了 `FSoftObjectPath` 而非硬引用？
- [ ] .ini 文件节名格式是否正确（`/Script/<Module>.<Class>`）？
- [ ] 数组配置是否使用了 `+` 前缀？

---

## 4. 头文件循环依赖

### 错误示例：在头文件中过度 include

```cpp
//  错误：MyFeature.h
#include "CoreMinimal.h"
#include "MyAbilitySystemComponent.h"      // 应该前置声明！
#include "MyComplexStruct.h"               // 可能导致循环依赖
#include "GameFramework/Character.h"       // 不需要完整定义

UCLASS()
class UMyFeature : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY()
    UMyAbilitySystemComponent* ASC;  // 指针类型，不需要完整定义

    void UseOn(ACharacter* Target);  // 参数是指针，不需要完整定义
};
```

### 为什么这是错误的？

#### 1. 编译依赖爆炸
- 头文件 include 另一个头文件 → 依赖传递到所有引用方
- 导致编译时间大幅增加
- 任何被 include 的文件修改都会触发大量重新编译

#### 2. 循环依赖风险
```
A.h includes B.h
B.h includes C.h
C.h includes A.h
→ 编译错误！
```

#### 3. 破坏封装
- 不必要的 include 暴露实现细节
- 增加耦合，降低模块独立性

### 正确做法：前置声明 + .cpp 中 include

头文件 (.h)：尽量使用前置声明

```cpp
//  正确：MyFeature.h
#include "CoreMinimal.h"
#include "UObject/Object.h"

// 前置声明（Forward Declaration）
class UMyAbilitySystemComponent;
class ACharacter;
struct FMyComplexStruct;

UCLASS()
class UMyFeature : public UObject
{
    GENERATED_BODY()
public:
    // 指针/引用类型：只需前置声明
    UPROPERTY()
    class UMyAbilitySystemComponent* ASC;  // class 关键字 + 前置声明（UE 推荐写法）

    // 参数/返回值是指针/引用：只需前置声明
    void UseOn(ACharacter* Target);
};
```

实现文件 (.cpp)：在这里 include 完整定义

```cpp
//  正确：MyFeature.cpp
#include "MyFeature.h"

// 现在才 include 完整定义
#include "MyAbilitySystemComponent.h"
#include "GameFramework/Character.h"
#include "MyComplexStruct.h"

void UMyFeature::UseOn(ACharacter* Target)
{
    // 这里才需要完整定义，所以在 .cpp 中 include
    if (Target && ASC)
    {
        ASC->DoSomething();
        Target->Jump();
    }
}
```

### 何时必须 include 完整定义？

在头文件中必须 include 的情况：

1. 继承关系：
```cpp
#include "UObject/Object.h"  // 必须：因为继承需要完整定义
class UMyClass : public UObject { };
```

2. 按值包含（非指针/引用）：
```cpp
#include "MyStruct.h"  // 必须：因为按值存储需要知道大小
UPROPERTY()
FMyStruct Data;  // 按值存储
```

3. 模板参数：
```cpp
#include "Array.h"
TArray<FMyStruct> Items;  // 模板需要完整定义
```

4. USTRUCT 在 UPROPERTY 中按值使用：
```cpp
#include "MyStruct.h"  // 必须：UHT 需要解析完整布局
UPROPERTY()
FMyStruct Data;
```

可以使用前置声明的情况：

1. 指针/引用成员：
```cpp
class UMyClass;  // 前置声明
UPROPERTY()
class UMyClass* MyPtr;  //  只是指针，不需要完整定义
```

2. 函数参数/返回值（指针/引用）：
```cpp
class ACharacter;  // 前置声明
void ProcessCharacter(ACharacter* Character);  // 
ACharacter* GetCharacter();  // 
```

3. 智能指针（TObjectPtr/TWeakObjectPtr）：
```cpp
class UMyComponent;  // 前置声明
TObjectPtr<UMyComponent> Component;  // 
```

### UE 特殊规则

#### 1. `.generated.h` 必须是头文件最后一个 include
```cpp
//  正确顺序
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor.generated.h"  // 必须最后！

UCLASS()
class AMyActor : public AActor { };
```

#### 2. UPROPERTY 的 `class` 关键字写法（UE 推荐）
```cpp
//  UE 推荐：结合前置声明和 UPROPERTY
UPROPERTY()
class UMyComponent* Component;  // class 关键字放在这里
```

### 最佳实践总结

| 场景 | 做法 |
|------|------|
| 指针/引用成员 | 头文件前置声明，.cpp 中 include |
| 按值成员/继承 | 头文件中 include |
| 函数参数（指针/引用） | 头文件前置声明，.cpp 中 include |
| 模板参数 | 头文件中 include |
| 只使用 Cast/类型检查 | 头文件前置声明，.cpp 中 include |
| 需要访问成员/方法 | .cpp 中 include |

### 检查清单

- [ ] 头文件中所有指针/引用类型是否都使用了前置声明？
- [ ] 是否将不必要的 include 从 .h 移到了 .cpp？
- [ ] `.generated.h` 是否是头文件中最后一个 include？
- [ ] 是否避免了"大而全"的聚合头（如 `Engine.h`）？
- [ ] UPROPERTY 指针是否使用了 `class` 关键字？

---

## 5. 函数复杂度过高

### 错误示例：单个函数做太多事情

```cpp
//  错误：200+ 行的巨型函数，嵌套层级深
void AShootCharacter::SwitchCharacterGender()
{
    // 验证 GameMode
    AShootGameModeBase* GameMode = Cast<AShootGameModeBase>(UGameplayStatics::GetGameMode(this));
    if (!GameMode || !GameMode->IsCharacterSwitchAllowed())
    {
        UE_LOG(LogTemp, Warning, TEXT("..."));
        return;
    }

    // 验证 PlayerState
    AShootPlayerState* PS = GetPlayerState<AShootPlayerState>();
    if (!PS) { return; }

    // 验证 ASC
    UShootAbilitySystemComponent* ASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent);
    if (!ASC) { return; }

    // ... 还有 10+ 个验证步骤

    // 保存当前角色属性
    CurrentSnapshot->Strength = AttributeSet->GetStrength();
    CurrentSnapshot->Intelligence = AttributeSet->GetIntelligence();
    // ... 保存更多属性

    // 保存当前角色技能
    for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities)
    {
        if (AbilitySpec.Ability)
        {
            FGameplayTagContainer AbilityTags;
            AbilitySpec.Ability->GetOwnedGameplayTags(AbilityTags, nullptr);

            if (AbilityTags.HasTag(CurrentGenderTag))
            {
                // 嵌套层级 3+
                for (const FGameplayTag& Tag : AbilitySpec.GetDynamicSpecSourceTags())
                {
                    // 嵌套层级 4+
                    if (Tag.MatchesTag(...))
                    {
                        // ...
                    }
                }
            }
        }
    }

    // ... 还有移除技能、切换外观、加载新数据、授予新技能等 150+ 行代码
}
```

### Sonar 扫描规则（可维护性硬约束）

| 指标 | 阈值 | 说明 |
|------|------|------|
| 认知复杂度 | ≤ 15 | 超过必须拆解函数 |
| 嵌套层级 | ≤ 3 | if/for/while/switch/try 最多 3 层 |
| 函数行数 | ≤ 80 行 | 包含空行和注释 |
| 单一职责 | 必须 | 一个函数只做一件清晰的事 |

### 为什么这是问题？

#### 1. 可维护性差
- 难以理解函数的整体逻辑
- 修改一处可能影响其他逻辑
- 难以测试和调试

#### 2. 认知负担高
- 深层嵌套增加理解难度
- 多重职责混在一起
- 需要在脑海中维护大量上下文

#### 3. 复用性低
- 无法复用其中的某个步骤
- 修改需求时需要改动整个函数

#### 4. 代码审查困难
- Sonar 扫描不通过
- Code Review 难以进行
- 违反 Clean Code 原则

### 正确做法：拆分成多个小函数

重构策略：单一职责 + 提前返回 + 辅助函数

```cpp
//  正确：主函数 < 80 行，嵌套 ≤ 1，清晰的步骤
void AShootCharacter::SwitchCharacterGender()
{
    // Step 1: Validate all required components
    UShootAbilitySystemComponent* ASC = nullptr;
    const UShootAttributeSet* AttributeSet = nullptr;
    AShootPlayerState* PS = nullptr;
    UShootSaveGame* SaveGame = nullptr;

    if (!ValidateCharacterSwitchContext(ASC, AttributeSet, PS, SaveGame))
    {
        return; // Validation failed, error already logged
    }

    // Step 2: Determine genders
    const ECharacterGender CurrentGender = PS->GetCharacterGender();
    const ECharacterGender NewGender = (CurrentGender == ECharacterGender::MALE)
        ? ECharacterGender::FEMALE : ECharacterGender::MALE;

    // Step 3: Get current character snapshot
    FCharacterSnapshot* CurrentSnapshot = (CurrentGender == ECharacterGender::MALE)
        ? &SaveGame->MaleSnapshot : &SaveGame->FemaleSnapshot;

    // Step 4: Save current character's attributes
    CurrentSnapshot->Strength = AttributeSet->GetStrength();
    CurrentSnapshot->Intelligence = AttributeSet->GetIntelligence();
    CurrentSnapshot->Resilience = AttributeSet->GetResilience();
    CurrentSnapshot->Vigor = AttributeSet->GetVigor();
    CurrentSnapshot->AttributePoints = AttributeSet->GetAttributePoints();

    // Step 5: Save current character's abilities
    const FGameplayTag CurrentGenderTag = (CurrentGender == ECharacterGender::MALE)
        ? FShootGameplayTags::Get().Ability_Character_Male
        : FShootGameplayTags::Get().Ability_Character_Female;

    SaveCurrentCharacterAbilities(CurrentGenderTag, ASC, CurrentSnapshot);

    // Step 6: Remove current gender's abilities
    const int32 RemovedCount = RemoveAbilitiesByGenderTag(CurrentGenderTag, ASC);

    // Step 7: Switch appearance
    AppearanceComponent->SwitchGender(NewGender);
    SaveGame->CurrentGender = NewGender;

    // Step 8: Load new character's snapshot
    const FCharacterSnapshot* NewSnapshot = (NewGender == ECharacterGender::MALE)
        ? &SaveGame->MaleSnapshot : &SaveGame->FemaleSnapshot;

    // Step 9: Load new character's attributes
    LoadCharacterAttributes(NewSnapshot, ASC);

    // Step 10: Grant new character's abilities
    const int32 GrantedCount = GrantCharacterAbilities(NewSnapshot, ASC);

    // Step 11: Persist to disk
    GetGameInstance()->GetSubsystem<USaveGameSubsystem>()->WriteSaveGame(SaveGame);

    UE_LOG(LogTemp, Log, TEXT("[ShootCharacter] Character switch complete"));
}
```

辅助函数：各自 < 80 行，嵌套 ≤ 3，单一职责

```cpp
//  正确：验证函数，62 行，嵌套 1 层
bool AShootCharacter::ValidateCharacterSwitchContext(
    UShootAbilitySystemComponent*& OutASC,
    const UShootAttributeSet*& OutAttributeSet,
    AShootPlayerState*& OutPS,
    UShootSaveGame*& OutSaveGame)
{
    // 使用提前返回降低嵌套
    const AShootGameModeBase* GameMode = Cast<AShootGameModeBase>(
        UGameplayStatics::GetGameMode(this));
    if (!GameMode || !GameMode->IsCharacterSwitchAllowed())
    {
        UE_LOG(LogTemp, Warning, TEXT("Character switching not allowed"));
        return false;
    }

    OutPS = GetPlayerState<AShootPlayerState>();
    if (!OutPS)
    {
        UE_LOG(LogTemp, Error, TEXT("No PlayerState found"));
        return false;
    }

    OutASC = Cast<UShootAbilitySystemComponent>(AbilitySystemComponent);
    if (!OutASC)
    {
        UE_LOG(LogTemp, Error, TEXT("No ASC found"));
        return false;
    }

    // ... 其他验证

    return true;
}

//  正确：保存技能函数，50 行，嵌套 3 层
void AShootCharacter::SaveCurrentCharacterAbilities(
    const FGameplayTag& GenderTag,
    UShootAbilitySystemComponent* ASC,
    FCharacterSnapshot* Snapshot)
{
    Snapshot->SavedAbilities.Empty();

    const TArray<FGameplayAbilitySpec> ActivatableAbilities = ASC->GetActivatableAbilities();
    for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities)  // 嵌套 1
    {
        if (!AbilitySpec.Ability)  // 嵌套 2：提前 continue 降低嵌套
        {
            continue;
        }

        FGameplayTagContainer AbilityTags;
        AbilitySpec.Ability->GetOwnedGameplayTags(AbilityTags, nullptr);

        if (!AbilityTags.HasTag(GenderTag))  // 嵌套 2：提前 continue
        {
            continue;
        }

        FSavedAbility SavedAbility;
        SavedAbility.GameplayAbility = AbilitySpec.Ability->GetClass();
        SavedAbility.AbilityLevel = AbilitySpec.Level;

        // Extract metadata tags
        for (const FGameplayTag& Tag : AbilitySpec.GetDynamicSpecSourceTags())  // 嵌套 2
        {
            const FString TagString = Tag.ToString();

            if (TagString.StartsWith(TEXT("Abilities.Status")))  // 嵌套 3
            {
                SavedAbility.AbilityStatus = Tag;
            }
            else if (TagString.StartsWith(TEXT("Abilities.Type")))  // 嵌套 3
            {
                SavedAbility.AbilityType = Tag;
            }
            else if (TagString.StartsWith(TEXT("InputTag")))  // 嵌套 3
            {
                SavedAbility.AbilitySlot = Tag;
            }
        }

        Snapshot->SavedAbilities.Add(SavedAbility);
    }
}

//  正确：移除技能函数，28 行，嵌套 2 层
int32 AShootCharacter::RemoveAbilitiesByGenderTag(
    const FGameplayTag& GenderTag,
    UShootAbilitySystemComponent* ASC)
{
    TArray<FGameplayAbilitySpecHandle> AbilitiesToRemove;

    const TArray<FGameplayAbilitySpec> ActivatableAbilities = ASC->GetActivatableAbilities();
    for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities)  // 嵌套 1
    {
        if (!AbilitySpec.Ability)  // 嵌套 2：提前 continue
        {
            continue;
        }

        FGameplayTagContainer AbilityTags;
        AbilitySpec.Ability->GetOwnedGameplayTags(AbilityTags, nullptr);

        if (AbilityTags.HasTag(GenderTag))  // 嵌套 2
        {
            AbilitiesToRemove.Add(AbilitySpec.Handle);
        }
    }

    // Remove all collected abilities
    for (const FGameplayAbilitySpecHandle& Handle : AbilitiesToRemove)
    {
        ASC->ClearAbility(Handle);
    }

    return AbilitiesToRemove.Num();
}

//  正确：加载属性函数，7 行，嵌套 0 层
void AShootCharacter::LoadCharacterAttributes(
    const FCharacterSnapshot* Snapshot,
    UShootAbilitySystemComponent* ASC)
{
    ASC->SetNumericAttributeBase(UShootAttributeSet::GetStrengthAttribute(), Snapshot->Strength);
    ASC->SetNumericAttributeBase(UShootAttributeSet::GetIntelligenceAttribute(), Snapshot->Intelligence);
    ASC->SetNumericAttributeBase(UShootAttributeSet::GetResilienceAttribute(), Snapshot->Resilience);
    ASC->SetNumericAttributeBase(UShootAttributeSet::GetVigorAttribute(), Snapshot->Vigor);
    ASC->SetNumericAttributeBase(UShootAttributeSet::GetAttributePointsAttribute(), Snapshot->AttributePoints);
}

//  正确：授予技能函数，43 行，嵌套 2 层
int32 AShootCharacter::GrantCharacterAbilities(
    const FCharacterSnapshot* Snapshot,
    UShootAbilitySystemComponent* ASC)
{
    int32 GrantedCount = 0;

    for (const FSavedAbility& SavedAbility : Snapshot->SavedAbilities)  // 嵌套 1
    {
        if (!SavedAbility.GameplayAbility)  // 嵌套 2：提前 continue
        {
            continue;
        }

        FGameplayAbilitySpec NewAbilitySpec(SavedAbility.GameplayAbility, SavedAbility.AbilityLevel);

        // Restore metadata tags
        if (SavedAbility.AbilityStatus.IsValid())
        {
            NewAbilitySpec.GetDynamicSpecSourceTags().AddTag(SavedAbility.AbilityStatus);
        }
        if (SavedAbility.AbilityType.IsValid())
        {
            NewAbilitySpec.GetDynamicSpecSourceTags().AddTag(SavedAbility.AbilityType);
        }
        if (SavedAbility.AbilitySlot.IsValid())
        {
            NewAbilitySpec.GetDynamicSpecSourceTags().AddTag(SavedAbility.AbilitySlot);
        }

        ASC->GiveAbility(NewAbilitySpec);
        GrantedCount++;

        // Auto-activate passive abilities if equipped
        const bool bIsPassive = SavedAbility.AbilityType.MatchesTagExact(
            FShootGameplayTags::Get().Abilities_Type_Passive);
        const bool bIsEquipped = SavedAbility.AbilityStatus.MatchesTagExact(
            FShootGameplayTags::Get().Abilities_Status_Equipped);

        if (bIsPassive && bIsEquipped)  // 嵌套 2
        {
            ASC->TryActivateAbility(NewAbilitySpec.Handle);
        }
    }

    return GrantedCount;
}
```

### 降低复杂度的技巧

#### 1. 提前返回（Early Return）
```cpp
//  深层嵌套
if (Condition1)
{
    if (Condition2)
    {
        if (Condition3)
        {
            // 真正的逻辑
        }
    }
}

//  提前返回，降低嵌套
if (!Condition1) return;
if (!Condition2) return;
if (!Condition3) return;

// 真正的逻辑（嵌套 0 层）
```

#### 2. 提前 continue（循环中）
```cpp
//  深层嵌套
for (const auto& Item : Items)
{
    if (Item.IsValid())
    {
        if (Item.ShouldProcess())
        {
            // 处理逻辑
        }
    }
}

//  提前 continue，降低嵌套
for (const auto& Item : Items)
{
    if (!Item.IsValid()) continue;
    if (!Item.ShouldProcess()) continue;

    // 处理逻辑（嵌套 1 层）
}
```

#### 3. 提炼函数（Extract Method）
```cpp
//  一个函数做多件事
void ProcessData()
{
    // 验证数据（30 行）
    // 转换数据（40 行）
    // 保存数据（30 行）
    // 发送通知（20 行）
}

//  拆分成多个函数
void ProcessData()
{
    if (!ValidateData()) return;
    TransformData();
    SaveData();
    SendNotification();
}

bool ValidateData() { /* 30 行 */ }
void TransformData() { /* 40 行 */ }
void SaveData() { /* 30 行 */ }
void SendNotification() { /* 20 行 */ }
```

#### 4. 用多态替代复杂分支
```cpp
//  巨大的 switch/if-else
void ProcessAbility(EAbilityType Type)
{
    switch (Type)
    {
    case EAbilityType::Offensive:
        // 50 行处理逻辑
        break;
    case EAbilityType::Passive:
        // 50 行处理逻辑
        break;
    case EAbilityType::Defensive:
        // 50 行处理逻辑
        break;
    // ... 更多类型
    }
}

//  使用多态
class UAbilityBase
{
    virtual void Process() = 0;
};

class UOffensiveAbility : public UAbilityBase
{
    virtual void Process() override { /* 20 行 */ }
};

class UPassiveAbility : public UAbilityBase
{
    virtual void Process() override { /* 20 行 */ }
};

// 调用方
void ProcessAbility(UAbilityBase* Ability)
{
    if (Ability)
    {
        Ability->Process();  // 简洁！
    }
}
```

#### 5. 使用表驱动法
```cpp
//  长串 if-else
FString GetAbilityName(EAbilityType Type)
{
    if (Type == EAbilityType::Offensive) return TEXT("Offensive");
    else if (Type == EAbilityType::Passive) return TEXT("Passive");
    else if (Type == EAbilityType::Defensive) return TEXT("Defensive");
    // ... 10+ 个分支
}

//  表驱动
static const TMap<EAbilityType, FString> AbilityNames = {
    { EAbilityType::Offensive, TEXT("Offensive") },
    { EAbilityType::Passive, TEXT("Passive") },
    { EAbilityType::Defensive, TEXT("Defensive") },
    // ...
};

FString GetAbilityName(EAbilityType Type)
{
    const FString* Name = AbilityNames.Find(Type);
    return Name ? *Name : TEXT("Unknown");
}
```

### Sonar 扫描适用性

Sonar 规则完全适用于 UE C++：
- UE 官方也遵循 Clean Code 原则
- Epic 的代码库也遵循类似的复杂度约束
- 可维护性对大型游戏项目至关重要

UE 特殊情况的例外：
- 自动生成的代码（`.generated.h`）不受约束
- Blueprint 事件宏（`UFUNCTION(BlueprintImplementableEvent)`）可能稍长
- 但自己编写的逻辑代码必须遵守复杂度规则

### 检查清单

提交代码前，请检查每个函数：

- [ ] 函数行数 ≤ 80 行（包含空行和注释）？
- [ ] 嵌套层级 ≤ 3 层？
- [ ] 函数只做一件事（单一职责）？
- [ ] 是否使用了提前返回/continue 降低嵌套？
- [ ] 是否将复杂逻辑提炼成了辅助函数？
- [ ] 函数名清晰描述其功能？
- [ ] 每个函数是否都有清晰的注释说明职责？

---

## 6. UCLASS Config 配置错误

### 错误示例：使用 UPROPERTY(Config) 但未指定配置文件

```cpp
//  错误：编译错误 "Classes with config member variables need to specify config file"
UCLASS()  // 缺少 config= 参数！
class UGameFlowSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    // 使用了 Config 但 UCLASS 没有指定配置文件
    UPROPERTY(Config)
    FString MainMenuMapName = TEXT("MainMenuMap");

    UPROPERTY(Config)
    FString HomeMapName = TEXT("HomeMap");
};
```

### 编译错误信息

```
Error: Classes with config / globalconfig member variables need to specify config file.
```

### 为什么这是错误的？

UE 的配置系统要求：
1. UPROPERTY(Config) 表示该属性可以从 .ini 文件读取/写入
2. UCLASS() 必须指定目标配置文件，否则 UHT 不知道从哪个 .ini 文件读取

如果只写 `UPROPERTY(Config)` 而不在 `UCLASS()` 中指定 `config=`，编译器会报错。

### 正确做法：UCLASS 指定配置文件

写入 DefaultEngine.ini：

```cpp
//  正确：指定 config=Engine
UCLASS(config=Engine, defaultconfig)
class UGameFlowSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    // Map names (can be configured in DefaultEngine.ini)
    UPROPERTY(Config)
    FString MainMenuMapName = TEXT("MainMenuMap");

    UPROPERTY(Config)
    FString HomeMapName = TEXT("HomeMap");

    UPROPERTY(Config)
    FString LobbyMapName = TEXT("LobbyMap");
};
```

对应的 Config/DefaultEngine.ini 配置：

```ini
[/Script/NewWorldOrder.GameFlowSubsystem]
MainMenuMapName=MainMenuMap
HomeMapName=HomeMap
LobbyMapName=LobbyMap
```

写入 DefaultGame.ini：

```cpp
//  正确：指定 config=Game
UCLASS(config=Game, defaultconfig)
class UMyGameSettings : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditDefaultsOnly)
    int32 MaxPlayers = 4;

    UPROPERTY(Config, EditDefaultsOnly)
    float GameDifficulty = 1.0f;
};
```

对应的 Config/DefaultGame.ini 配置：

```ini
[/Script/YourModule.MyGameSettings]
MaxPlayers=4
GameDifficulty=1.0
```

### UCLASS Config 参数详解

| 参数 | 说明 | 目标文件 |
|------|------|---------|
| `config=Engine` | 引擎配置 | `Config/DefaultEngine.ini` |
| `config=Game` | 游戏配置 | `Config/DefaultGame.ini` |
| `config=Input` | 输入配置 | `Config/DefaultInput.ini` |
| `config=Editor` | 编辑器配置 | `Config/DefaultEditor.ini` |
| `config=EditorPerProjectUserSettings` | 编辑器用户配置 | `Config/DefaultEditorPerProjectUserSettings.ini` |

defaultconfig 参数：
- 添加 `defaultconfig` 可以将类的默认值保存到 Default*.ini
- 不加 `defaultconfig` 时，只能从 .ini 读取，不能保存默认值

### .ini 文件配置语法

节名格式：
```ini
[/Script/<模块名>.<类名>]
```

示例：
```ini
[/Script/NewWorldOrder.GameFlowSubsystem]
MainMenuMapName=MyCustomMenuMap
HomeMapName=MyCustomHomeMap
```

数组配置：
```ini
[/Script/YourModule.MySettings]
+AllowedMaps=/Game/Maps/Map1.Map1
+AllowedMaps=/Game/Maps/Map2.Map2
+AllowedMaps=/Game/Maps/Map3.Map3
```

清空并重新设置数组：
```ini
[/Script/YourModule.MySettings]
!AllowedMaps=ClearArray
+AllowedMaps=/Game/Maps/NewMap1.NewMap1
+AllowedMaps=/Game/Maps/NewMap2.NewMap2
```

### 何时使用 Config vs GlobalConfig

| Specifier | 说明 | 使用场景 |
|-----------|------|---------|
| `UPROPERTY(Config)` | 每个类实例可以有不同的配置 | 一般配置项 |
| `UPROPERTY(GlobalConfig)` | 所有类实例共享同一配置 | 全局设置（如编辑器偏好） |

示例：
```cpp
UCLASS(config=Game, defaultconfig)
class UMyGameSettings : public UObject
{
    GENERATED_BODY()
public:
    // 一般配置：每个实例可能不同
    UPROPERTY(Config, EditDefaultsOnly)
    int32 MaxPlayers = 4;

    // 全局配置：所有实例共享
    UPROPERTY(GlobalConfig, EditDefaultsOnly)
    bool bEnableDebugMode = false;
};
```

### 使用 FSoftObjectPath 避免硬引用

对于地图、资源等类型，使用 `FSoftObjectPath` 而不是硬引用：

```cpp
//  错误：硬引用导致编译依赖膨胀
UCLASS(config=Engine)
class UBadSettings : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(Config)
    UWorld* MainMenuMap;  // 需要 include 完整定义，增加依赖
};

//  正确：使用软引用
UCLASS(config=Engine, defaultconfig)
class UGoodSettings : public UObject
{
    GENERATED_BODY()
public:
    // 使用软引用，运行时加载
    UPROPERTY(Config, EditDefaultsOnly, meta=(AllowedClasses="/Script/Engine.World"))
    FSoftObjectPath MainMenuMapPath;

    // 或者使用 TSoftObjectPtr
    UPROPERTY(Config, EditDefaultsOnly)
    TSoftObjectPtr<UWorld> MainMenuMap;
};
```

在 .ini 中配置软引用路径：
```ini
[/Script/YourModule.GoodSettings]
MainMenuMapPath=/Game/Maps/MainMenu.MainMenu
```

运行时加载：
```cpp
void UGoodSettings::LoadMainMenuMap()
{
    if (MainMenuMapPath.IsValid())
    {
        // 异步加载
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            MainMenuMapPath,
            FStreamableDelegate::CreateUObject(this, &UGoodSettings::OnMapLoaded)
        );
    }
}
```

### 常见错误和解决方案

#### 错误 1：配置不生效

问题：
```cpp
UCLASS(config=Engine)  // 没有 defaultconfig
class UMySettings : public UObject
{
    UPROPERTY(Config)
    int32 Value = 100;
};
```

在 .ini 中设置了 `Value=200`，但运行时仍是 100。

原因：
- Config 系统没有正确初始化
- 没有调用 `LoadConfig()` 或 `ReloadConfig()`

解决：
```cpp
// 在需要时加载配置
UMySettings* Settings = NewObject<UMySettings>();
Settings->LoadConfig();  // 从 .ini 加载

// 或者使用 GConfig 直接读取
int32 Value = 100;
GConfig->GetInt(TEXT("/Script/YourModule.MySettings"), TEXT("Value"), Value,
                GEngineIni);
```

#### 错误 2：配置文件路径错误

问题：
```cpp
UCLASS(config=Engine)  // 声明使用 Engine
```

但在 `DefaultGame.ini` 中配置 → 不生效！

解决：
- `config=Engine` 对应 `DefaultEngine.ini`
- `config=Game` 对应 `DefaultGame.ini`
- UCLASS 和 .ini 文件必须匹配

#### 错误 3：数组配置语法错误

错误：
```ini
[/Script/YourModule.MySettings]
AllowedMaps=/Game/Maps/Map1.Map1  #  没有 + 号
AllowedMaps=/Game/Maps/Map2.Map2  #  会覆盖，不是追加
```

正确：
```ini
[/Script/YourModule.MySettings]
+AllowedMaps=/Game/Maps/Map1.Map1  #  使用 + 追加
+AllowedMaps=/Game/Maps/Map2.Map2  #  追加第二个元素
```

### 最佳实践

1. 总是配对使用：
   - `UPROPERTY(Config)` 必须配合 `UCLASS(config=...)`
   - 选择正确的 config 目标（Engine/Game/Editor 等）

2. 使用 defaultconfig：
   ```cpp
   UCLASS(config=Engine, defaultconfig)  // 推荐：可保存默认值
   ```

3. 地图/资源使用软引用：
   ```cpp
   UPROPERTY(Config)
   FSoftObjectPath MapPath;  //  不要用 UWorld* 等硬引用
   ```

4. 配置文件注释：
   ```ini
   [/Script/NewWorldOrder.GameFlowSubsystem]
   ; Main menu map name
   MainMenuMapName=MainMenuMap
   ; Home map (character selection)
   HomeMapName=HomeMap
   ```

5. 使用元数据提示：
   ```cpp
   UPROPERTY(Config, EditDefaultsOnly,
             meta=(AllowedClasses="/Script/Engine.World",
                   ToolTip="Main menu map to load"))
   FSoftObjectPath MainMenuMap;
   ```

### 检查清单

在使用 Config 属性时，检查以下项：

- [ ] `UPROPERTY(Config)` 的类是否在 `UCLASS()` 中指定了 `config=` 参数？
- [ ] `config=` 参数是否与实际 .ini 文件匹配（Engine/Game/Editor）？
- [ ] 是否添加了 `defaultconfig` 以支持保存默认值？
- [ ] 地图/资源类型是否使用了 `FSoftObjectPath` 或 `TSoftObjectPtr`？
- [ ] .ini 文件中的节名格式是否正确（`/Script/<Module>.<Class>`）？
- [ ] 数组配置是否使用了 `+` 前缀？

---

## 7. 未检查现有代码就随意修改

### 严重错误示例：未检查项目已有的交互系统

发生时间： 2025-10-31

错误描述：
在实现角色切换功能时，没有检查项目中已存在的 `Interaction` 文件夹下的完整交互系统，直接修改了 `InteractionInterface.cpp` 文件，导致：
1. 破坏了现有的交互系统
2. 导致 30+ 个编译错误
3. 需要完全回滚代码

项目实际情况：
项目使用的是 Lyra 交互系统（从 Epic Games 复制）：

Lyra 交互系统组件：
- `IInteractableTarget` - 核心交互目标接口
- `IInteractionInstigator` - 交互发起者接口
- `InteractionOption`, `InteractionQuery` - 交互选项和查询
- 完整的 AbilityTask 系统：
  - `AbilityTask_WaitForInteractableTargets`
  - `AbilityTask_GrantNearbyInteraction`
  - `AbilityTask_WaitForInteractableTargets_SingleLineTrace`
- `ShootGameplayAbility_Interact` - 交互技能
- `GameplayAbilityTargetActor_Interact` - 交互目标选择器
- `InteractionStatics` - 交互静态函数库

错误创建的文件（已删除）：
- `IInteractionInterface` - 这是 Claude 错误创建的，用户明确表示不需要此类
- 该接口与 Lyra 系统重复，已被删除
- 教训：不要在没有检查现有系统的情况下创建新接口

### 为什么这是严重错误？

#### 1. 违反基本开发原则

没有遵循"先理解，再修改"的原则：
- 在修改任何文件前，必须完整理解项目现有架构
- 必须检查相关文件夹下的所有现有代码
- 必须理解不同系统之间的关系

#### 2. 盲目修改导致破坏

错误行为：
```cpp
// InteractionInterface.cpp - 错误地添加了 _Implementation 函数
void IInteractionInterface::BeginFocus_Implementation()
{
    // BlueprintNativeEvent 的实现是自动生成的！
    // 手动添加会导致 "function already has body" 编译错误
}
```

正确认知：
- `BlueprintNativeEvent` 宏会自动生成 `_Implementation` 函数
- 接口的 .cpp 文件通常只包含复杂的实现逻辑
- 简单接口可能根本不需要 .cpp 文件

#### 3. 忽略用户明确指示

用户多次警告：
1. 第一次：提醒检查 GameInstance 相关逻辑
2. 第二次：指出没有看 Interaction 文件夹下的代码
3. 第三次（本次）：明确指出未检查 `IInteractableTarget` 和 Interaction 系统

### 正确做法：检查现有代码的流程

#### Step 1: 列出相关文件夹的所有文件

```bash
# 使用 find 命令列出所有相关文件
find /path/to/project -path "*/Interaction/*" -type f

# 或使用 Glob 工具
pattern: /Interaction//*
```

#### Step 2: 阅读核心接口和基类

```cpp
// 先阅读核心接口
Read: Interaction/IInteractableTarget.h
Read: Interaction/IInteractionInstigator.h
Read: Interaction/InteractionOption.h
Read: Interaction/InteractionQuery.h
```

#### Step 3: 理解系统架构

绘制或理解组件关系：
```
IInteractableTarget (Lyra 交互系统)
  ↓ 实现者
  ├─ NPC
  ├─ 可拾取物品
  └─ 交互对象

注意：不要创建新的交互接口！
项目已有完整的 Lyra 交互系统，直接使用即可。
```

#### Step 4: 检查是否有文档或注释

```cpp
// 查找文档文件
find /path/to/project -name "*.md" -o -name "README*"

// 查找代码注释
grep -r "LYRA" Interaction/
grep -r "Copyright Epic Games" Interaction/
```

#### Step 5: 询问用户确认

在不确定的情况下，始终先询问用户：
```
"我看到项目中有 Interaction 文件夹，里面包含完整的 Lyra 交互系统：
- IInteractableTarget（核心接口）
- IInteractionInstigator（发起者接口）
- 各种 AbilityTask 和交互选项类
我是否应该使用这个系统来实现角色切换交互？还是有其他方式？"
```

### 实际案例分析

应该做的：
1.  使用 `find` 或 `Glob` 列出 Interaction 文件夹所有文件
2.  阅读 `IInteractableTarget.h`（Lyra 系统核心）
3.  阅读其他 Lyra 交互系统文件（InteractionOption, InteractionQuery 等）
4.  查看现有 NPC 如何实现交互（实现了哪个接口？）
5.  理解 Lyra 交互系统的完整架构
6.  询问用户是否应该使用此系统实现角色切换

实际错误行为：
1.  没有列出 Interaction 文件夹的所有文件
2.  没有发现 Lyra 交互系统的存在
3.  错误创建了 `IInteractionInterface`（与 Lyra 系统重复）
4.  直接修改 `.cpp` 文件添加 `_Implementation`（不理解 BlueprintNativeEvent）
5.  忽略了用户之前的警告（这是第三次！）
6.  假设不存在的接口是用户创建的

### Lyra 交互系统简介

核心概念：

```cpp
// 1. 交互目标接口
class IInteractableTarget
{
    // 收集可用的交互选项
    virtual void GatherInteractionOptions(
        const FInteractionQuery& Query,
        FInteractionOptionBuilder& Builder
    ) = 0;
};

// 2. 交互选项
struct FInteractionOption
{
    TSubclassOf<UGameplayAbility> InteractionAbilityToGrant;
    TScriptInterface<IInteractableTarget> InteractableTarget;
    // ...
};

// 3. 交互流程
// Player → AbilityTask → Find IInteractableTarget → GatherOptions → Grant Ability → Execute
```

为什么 Lyra 系统强大：
- 基于 GAS（Gameplay Ability System）
- 支持复杂的交互条件判断
- 支持多个交互选项（一个对象多种交互方式）
- 完全可扩展和数据驱动

### 最佳实践总结

#### 1. 修改任何代码前的检查清单

- [ ] 列出相关文件夹的所有文件
- [ ] 阅读核心接口和基类
- [ ] 搜索相关的文档和注释
- [ ] 检查是否有第三方代码（Epic Games, Lyra, 其他插件）
- [ ] 理解现有系统的架构和用途
- [ ] 在不确定时询问用户

#### 2. 遇到多个相似系统时

- [ ] 列出所有相似的接口/类
- [ ] 对比它们的功能和使用场景
- [ ] 查找它们的实际使用者（哪些类实现了它们？）
- [ ] 询问用户为什么存在多个系统
- [ ] 确认应该使用哪个系统

#### 3. 修改接口或基类时

- [ ] 检查是否有 `BlueprintNativeEvent` 或 `BlueprintImplementableEvent`
- [ ] 检查 `.cpp` 文件是否已存在
- [ ] 检查是否有自动生成的代码（`GENERATED_BODY()`, `_Implementation`）
- [ ] 查找所有实现该接口的类
- [ ] 评估修改的影响范围

#### 4. 发现第三方代码时

- [ ] 查看 Copyright 信息（Epic Games, Lyra, etc.）
- [ ] 不要轻易修改第三方代码
- [ ] 优先扩展而非修改
- [ ] 记录第三方代码的来源和用途

### 教训和反思

核心教训：
1. 永远先理解，再修改 - 花时间理解比花时间修复bug更值得
2. 遵循用户指示 - 用户的警告往往指向关键问题
3. 检查整个文件夹 - 不要只看单个文件
4. 识别第三方代码 - Lyra, Epic 的代码有其设计理念
5. 不确定时询问 - 询问总比犯错好

回滚决策：
当发现以下情况时，应立即回滚并重新开始：
- 编译错误超过 10 个
- 破坏了现有功能
- 没有理解现有架构就进行了修改
- 忽略了用户的明确警告

### 检查清单

在修改项目代码前，必须完成：

- [ ] 列出相关文件夹的所有文件（使用 `find` 或 `Glob`）
- [ ] 阅读所有相关的头文件（.h）
- [ ] 理解现有系统的架构和用途
- [ ] 搜索相关的文档和注释
- [ ] 检查 Copyright 信息识别第三方代码
- [ ] 查找现有代码的使用者和实现者
- [ ] 在不确定的情况下询问用户
- [ ] 遵循用户之前给出的所有警告和指示

特别注意：
- Interaction 文件夹包含完整的 Lyra 交互系统
- 不要随意修改 `_Implementation` 函数（BlueprintNativeEvent）
- 项目可能同时存在多套系统服务于不同目的
- 用户的警告是宝贵的项目知识来源

---

*最后更新：2025-10-31*
*维护者：Claude Code*
