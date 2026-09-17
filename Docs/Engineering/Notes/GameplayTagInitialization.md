# Gameplay Tag 初始化笔记 (2025-11-04)

## 问题回顾
- 在 GA 构造函数中直接调用 `FShootGameplayTags::Get().GameplayEvent_ReloadDone`，拿到的是默认空标签。
- 原因：`FShootGameplayTags::InitializeNativeGameplayTags()` 在 `UShootAssetManager::StartInitialLoading()` 中调用，此时所有 UObject 构造已完成；构造阶段访问单例数据为空。

## 正确做法
1. 需要由 C++ 直接引用的稳定 Native Tag，使用官方宏定义：
   ```cpp
   // ShootGameplayTags.h
   UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_ReloadDone);

   // ShootGameplayTags.cpp
   UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayEvent_ReloadDone, "GameplayEvent.ReloadDone", "Reload montage finished");
   ```
   这样标签在模块加载时注册，构造函数中可直接使用。
2. 只供配置、蓝图或 GameplayAbility CDO 使用的 Tag，唯一注册源放在
   `Config/DefaultGameplayTags.ini`。构造阶段通过
   `FGameplayTag::RequestGameplayTag(FName(TEXT("Tag.Path")), false)` 读取，不能依赖
   `UShootAssetManager::StartInitialLoading()` 之后才填充的 `FShootGameplayTags` 字段。
3. 运行时需要共享访问的 Native Tag 才进入 `FShootGameplayTags`；不要让同一个 Tag 同时由
   C++ 原生注册和 `DefaultGameplayTags.ini` 注册，避免出现两个数据源。
4. 更新 AGENTS.md 的 Gameplay Tag 约定，要求遵循宏注册或构造期使用 RequestGameplayTag 的兜底方案。

## Zombie 配置 Tag

`Ability.Skill.Zombie.Melee` 与 `Cooldown.AI.ZombieMelee` 是配置 Tag，不是
`FShootGameplayTags` 的字段。它们在 `Config/DefaultGameplayTags.ini` 中提前注册，
`UShootGA_ZombieMelee` 的 CDO 与 `AEnemyBotCharacter::TryActivateMeleeAttack` 都通过
`FGameplayTag::RequestGameplayTag(FName(TEXT("...")), false)` 获取。这样 GameplayAbility
CDO 在 AssetManager 初始化前创建时，AbilityTags 和 ActivationBlockedTags 仍能拿到有效值。

## 后续行动
- 继续检查其他 GameplayAbility 构造函数、静态成员是否访问了未初始化的标签，并替换为宏或
  `RequestGameplayTag`；不要把配置 Tag 复制进 `FShootGameplayTags`。
- 检查所有构造函数、静态成员是否访问了未初始化的标签，并替换为宏或 `RequestGameplayTag`。
- 2025-11-16：新增 `UI.Toast.ResourcePickup`（HUD 材料拾取提示）作为 Native Tag，UI Toast Widget 监听 `UGameplayMessageSubsystem` 时统一走 `RequestGameplayTag` 兜底。
