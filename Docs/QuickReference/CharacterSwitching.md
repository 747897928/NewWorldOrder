# Character Switching Quick Reference

版本: 1.0 | 最后更新: 2025-11-01

用途: AI日常开发快速查询（完整设计见 ../Tasks/CharacterSwitching/）

---

## 核心原则

共享组件架构: 角色切换是配置切换，不是对象切换
- 不销毁/重建: PlayerController, PlayerState, ASC, Character
- 只切换配置: Mesh, AnimClass, Abilities, Attributes

单一职责分离:
- UMutableAppearanceComponent: SwitchGender(NewGender)
- UAbilitySystemComponent: 移除旧技能，添加新技能
- UShootAttributeSet: 从SaveGame加载新属性值
- UShootSaveGame: 保存旧角色，加载新角色
- AShootPlayerState: 协调所有组件的切换

---

## 实现位置

AShootPlayerState（不是Character，不是GameplayAbility）

为什么？
- PlayerState持有ASC
- PlayerState生命周期覆盖整个游戏会话
- PlayerState负责网络同步

---

## 核心方法签名

```cpp
// ShootPlayerState.h
class AShootPlayerState : public APlayerState
{
public:
    UFUNCTION(BlueprintCallable, Category="Character")
    bool SwitchToCharacter(ECharacterGender NewGender);

    UFUNCTION(BlueprintPure, Category="Character")
    ECharacterGender GetCurrentGender() const { return CurrentGender; }

private:
    UPROPERTY(Replicated)
    ECharacterGender CurrentGender;

    // 内部实现（每个函数≤80行，≤3嵌套）
    bool ValidateSwitchPreconditions(ECharacterGender NewGender);
    bool SaveCurrentCharacterData();
    void SwitchAbilities(ECharacterGender OldGender, ECharacterGender NewGender);
    void SwitchAttributes(const FCharacterSnapshot& Snapshot);
    void SwitchAppearance(ECharacterGender TargetGender);
    void BroadcastCharacterSwitchEvent(ECharacterGender NewGender);
};
```

---

## 数据流

验证 → 保存 → 切换 → 加载 → 验证

1. ValidateSwitchPreconditions() - 检查是否可以切换
2. SaveCurrentCharacterData() - 保存到FCharacterSnapshot
3. SwitchAbilities() - 移除旧技能，添加新技能
4. SwitchAppearance() - 调用MutableAppearanceComponent->SwitchGender()
5. SwitchAttributes() - 使用InitAttribute()加载属性
6. BroadcastCharacterSwitchEvent() - 通知UI/音效/剧情

---

## 现有系统复用

外观切换（已实现，直接使用）:
```cpp
// Character/MutableAppearanceComponent.h/.cpp
UMutableAppearanceComponent::SwitchGender(ECharacterGender NewGender)
```

数据存储（已实现，直接使用）:
```cpp
// System/ShootSaveGame.h
struct FCharacterSnapshot
{
    float Strength, Intelligence, Resilience, Vigor;
    int32 AttributePoints, SkillPoints;
    TArray<FSavedAbility> SavedAbilities;
};

class UShootSaveGame
{
    ECharacterGender CurrentGender;
    FCharacterSnapshot MaleSnapshot;
    FCharacterSnapshot FemaleSnapshot;
};
```

GameplayTag（需要添加）:
```cpp
// AbilitySystem/ShootGameplayTags.h/.cpp
FGameplayTag Ability_Character_Male;      // "Ability.Character.Male"
FGameplayTag Ability_Character_Female;    // "Ability.Character.Female"
```

---

## 七大致命错误（绝对禁止）

详见: ../Tasks/CharacterSwitching/BestPractices_最佳实践.md

1. 角色切换实现为GameplayAbility - 技能会移除自己
2. 销毁和重建Character - 丢失所有引用
3. 运行时使用RequestGameplayTag - 性能差且不安全
4. 使用SetAttribute()修改属性 - 触发GE和网络同步，用InitAttribute()
5. 函数复杂度过高 - 违反Sonar规则（≤80行，≤3嵌套）
6. 头文件过度include - 导致循环依赖
7. 未检查现有代码就修改 - 破坏Lyra交互系统等

---

## 关键实现细节

技能切换:
```cpp
// 移除旧技能
const FGameplayTag OldGenderTag = FShootGameplayTags::Get().Ability_Character_Male;
for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
{
    if (Spec.Ability->AbilityTags.HasTag(OldGenderTag))
        ASC->ClearAbility(Spec.Handle);
}

// 添加新技能
for (const FSavedAbility& SavedAbility : NewSnapshot.SavedAbilities)
{
    FGameplayAbilitySpec NewSpec(SavedAbility.GameplayAbility, SavedAbility.AbilityLevel);
    ASC->GiveAbility(NewSpec);
}
```

属性切换（使用Init不是Set）:
```cpp
// 正确：使用InitAttribute()
AttributeSet->InitStrength(Snapshot.Strength);
AttributeSet->InitIntelligence(Snapshot.Intelligence);

// 错误：使用SetAttribute() - 会触发GE和网络同步
// AttributeSet->SetStrength(Snapshot.Strength);  // 绝对不要
```

外观切换（使用现有组件）:
```cpp
// 正确：复用现有MutableAppearanceComponent
MutableAppearanceComponent->SwitchGender(NewGender);

// 错误：销毁重建Character
// OldCharacter->Destroy(); NewCharacter = SpawnActor();  // 绝对不要
```

---

## Sonar规则检查清单

每个函数必须满足:
- [ ] 行数 ≤ 80 行（包含空行和注释）
- [ ] 嵌套层级 ≤ 3 层
- [ ] 认知复杂度 ≤ 15
- [ ] 单一职责（只做一件事）

降低复杂度技巧:
- 提前返回（Early Return）- if (!Condition) return;
- 提前continue（循环中）- if (!Valid) continue;
- 提炼函数（Extract Method）- 拆分大函数
- 表驱动法 - 用Map替代长串if-else

---

## 测试验证

基本切换:
- [ ] 男主 → 女主（无崩溃）
- [ ] 女主 → 男主（无崩溃）
- [ ] 连续切换100次（无内存泄漏）

数据持久化:
- [ ] 男主学习技能后切换，女主无此技能
- [ ] 切换回男主，技能正确恢复
- [ ] 属性值正确保存和恢复

边界情况:
- [ ] 切换到相同性别（失败但不崩溃）
- [ ] 战斗中切换（失败，如果有限制）
- [ ] SaveGame不存在时切换（正确处理）

性能:
- [ ] 切换耗时 < 100ms
- [ ] 无内存泄漏
- [ ] 技能移除/添加高效

---

## 实现前必读清单

详见: ../Tasks/CharacterSwitching/ImplementationChecklist_实现清单.md

开始编码前必须完成:
- [ ] 阅读Overview_总览.md
- [ ] 阅读DesignGuide_设计指南.md
- [ ] 阅读BestPractices_最佳实践.md
- [ ] 列出Character/, AbilitySystem/, System/, Interaction/所有文件
- [ ] 阅读MutableAppearanceComponent.h/.cpp
- [ ] 阅读ShootSaveGame.h
- [ ] 理解ASC在PlayerState上的生命周期
- [ ] 画出组件关系图和数据流图
- [ ] 向用户确认所有设计决策

---

## 跨文档引用

完整设计:
- ../Tasks/CharacterSwitching/Overview_总览.md
- ../Tasks/CharacterSwitching/DesignGuide_设计指南.md
- ../Tasks/CharacterSwitching/BestPractices_最佳实践.md
- ../Tasks/CharacterSwitching/ImplementationChecklist_实现清单.md

相关系统:
- ../SystemDesign/GameDesign/NumericalDesign/Skills/Overview_总览.md
- ../SystemDesign/GameDesign/NumericalDesign/Attributes/Overview_总览.md
