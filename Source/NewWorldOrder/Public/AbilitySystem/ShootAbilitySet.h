// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ActiveGameplayEffectHandle.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpecHandle.h"
#include "ShootAbilitySet.generated.h"

class UAttributeSet;
class UGameplayEffect;
class UShootAbilitySystemComponent;
class UShootGameplayAbility;
class UObject;

/**
 * FShootAbilitySet_GameplayAbility
 *
 * 【Ability 授予数据】AbilitySet 用来授予 Gameplay Ability 的数据
 *
 * 用途：
 *   - 定义要授予的 Ability 类
 *   - 设置 Ability 等级
 *   - 绑定输入 Tag（如果需要）
 *
 * 示例：
 *   - Ability: GA_Weapon_Fire_Rifle
 *   - AbilityLevel: 1
 *   - InputTag: InputTag.Weapon.Fire
 */
USTRUCT(BlueprintType)
struct FShootAbilitySet_GameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * 要授予的 Gameplay Ability
	 */
	// 内嵌在 DataAsset 中的配置结构需要编辑实例；否则 Python/VibeUE 无法可靠写入嵌套字段。
	UPROPERTY(EditAnywhere)
	TSubclassOf<UShootGameplayAbility> Ability = nullptr;

	/**
	 * Ability 等级
	 */
	UPROPERTY(EditAnywhere)
	int32 AbilityLevel = 1;

	/**
	 * 固定输入 Tag（用于处理输入）
	 *
	 * 核心或武器 Ability 可以配置固定标签；Match Skill 保持为空，由 SkillLoadout 在实际落槽时动态覆盖。
	 */
	UPROPERTY(EditAnywhere, Meta = (Categories = "InputTag"))
	FGameplayTag InputTag;
};

/**
 * FShootAbilitySet_GameplayEffect
 *
 * 【GameplayEffect 授予数据】AbilitySet 用来授予 Gameplay Effect 的数据
 *
 * 用途：
 *   - 定义要授予的 GameplayEffect 类
 *   - 设置 Effect 等级
 *
 * 示例：
 *   - GameplayEffect: GE_Weapon_AmmoRegeneration
 *   - EffectLevel: 1.0
 */
USTRUCT(BlueprintType)
struct FShootAbilitySet_GameplayEffect
{
	GENERATED_BODY()

public:
	/**
	 * 要授予的 Gameplay Effect
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> GameplayEffect = nullptr;

	/**
	 * Effect 等级
	 */
	UPROPERTY(EditAnywhere)
	float EffectLevel = 1.0f;
};

/**
 * FShootAbilitySet_AttributeSet
 *
 * 【AttributeSet 授予数据】AbilitySet 用来授予 Attribute Set 的数据
 *
 * 用途：
 *   - 定义要授予的 AttributeSet 类
 *
 * 示例：
 *   - AttributeSet: UWeaponAttributeSet（如果未来需要）
 */
USTRUCT(BlueprintType)
struct FShootAbilitySet_AttributeSet
{
	GENERATED_BODY()

public:
	/**
	 * 要授予的 Attribute Set
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<UAttributeSet> AttributeSet;
};

/**
 * FShootAbilitySet_GrantedHandles
 *
 * 【授予句柄存储】存储 AbilitySet 授予的内容的句柄
 *
 * 用途：
 *   - 记录授予的 Ability Spec 句柄（用于移除）
 *   - 记录授予的 GameplayEffect 句柄（用于移除）
 *   - 记录授予的 AttributeSet（用于移除）
 *
 * 生命周期：
 *   1. GiveToAbilitySystem() 时填充句柄
 *   2. 使用期间保持引用
 *   3. TakeFromAbilitySystem() 时移除所有授予的内容
 *
 * 重要性：
 *   - 装备武器时授予 Ability
 *   - 卸载武器时移除 Ability
 *   - 确保没有泄漏
 */
USTRUCT(BlueprintType)
struct FShootAbilitySet_GrantedHandles
{
	GENERATED_BODY()

public:
	/**
	 * 添加 Ability Spec 句柄
	 */
	void AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle);

	/**
	 * 添加 GameplayEffect 句柄
	 */
	void AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle);

	/**
	 * 添加 AttributeSet
	 */
	void AddAttributeSet(UAttributeSet* Set);

	/**
	 * 从 AbilitySystem 移除所有授予的内容
	 *
	 * @param ShootASC 目标 AbilitySystemComponent
	 *
	 * 执行流程：
	 *   1. 清除所有 Ability Specs
	 *   2. 移除所有 GameplayEffects
	 *   3. 移除所有 AttributeSets
	 */
	void TakeFromAbilitySystem(UShootAbilitySystemComponent* ShootASC);

protected:
	/**
	 * 授予的 Ability Spec 句柄
	 */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	/**
	 * 授予的 GameplayEffect 句柄
	 */
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> GameplayEffectHandles;

	/**
	 * 授予的 AttributeSet
	 */
	UPROPERTY()
	TArray<TObjectPtr<UAttributeSet>> GrantedAttributeSets;
};

/**
 * UShootAbilitySet
 *
 * 【Ability 集合】不可变的 DataAsset，用于授予一组 Gameplay Ability 和 Gameplay Effect
 *
 * 用途：
 *   - 将相关的 Ability、Effect、AttributeSet 打包成一个集合
 *   - 在装备武器时授予整个集合
 *   - 在卸载武器时移除整个集合
 *
 * 使用场景：
 *   1. 武器装备：
 *      - AbilitySet_Rifle：包含开火、装填、切换模式等 Ability
 *      - AbilitySet_Shotgun：包含霰弹枪特有的 Ability
 *   2. 技能装备：
 *      - AbilitySet_Shield：包含护盾激活、护盾破裂等 Ability
 *
 * 设计优势：
 *   - 集中管理相关 Ability
 *   - 易于创建新武器（复用 AbilitySet）
 *   - 自动管理授予/移除
 *
 * 示例用法：
 *   ```cpp
 *   // 装备武器时
 *   FShootAbilitySet_GrantedHandles GrantedHandles;
 *   AbilitySet->GiveToAbilitySystem(ASC, &GrantedHandles, WeaponInstance);
 *
 *   // 卸载武器时
 *   GrantedHandles.TakeFromAbilitySystem(ASC);
 *   ```
 */
UCLASS(BlueprintType, Const)
class UShootAbilitySet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UShootAbilitySet(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 将 AbilitySet 授予给指定的 AbilitySystemComponent
	 *
	 * @param ShootASC 目标 AbilitySystemComponent
	 * @param OutGrantedHandles 输出授予的句柄（用于后续移除）
	 * @param SourceObject Ability 的 SourceObject（通常是武器实例）
	 * @param InputTagOverride 调用者提供的动态输入 Tag；有效时覆盖本次 AbilitySet 授予中的输入标签
	 *
	 * 执行流程：
	 *   1. 遍历 GrantedGameplayAbilities，逐个授予 Ability
	 *      - 创建 AbilitySpec
	 *      - 设置 SourceObject（武器实例）
	 *      - GiveAbility(AbilitySpec)
	 *      - 记录 AbilitySpecHandle
	 *   2. 遍历 GrantedGameplayEffects，逐个应用 Effect
	 *      - 创建 EffectContext
	 *      - ApplyGameplayEffectToSelf()
	 *      - 记录 ActiveEffectHandle
	 *   3. 遍历 GrantedAttributes，逐个授予 AttributeSet
	 *      - GetOrCreateAttributeSubobject()
	 *      - 记录 AttributeSet
	 *
	 * 重要性：
	 *   - SourceObject 允许 Ability 访问武器实例数据（如扩散角度）
	 *   - OutGrantedHandles 用于后续移除授予的内容
	 */
	void GiveToAbilitySystem(UShootAbilitySystemComponent* ShootASC,
		FShootAbilitySet_GrantedHandles* OutGrantedHandles,
		UObject* SourceObject = nullptr,
		int32 AbilityLevelOverride = INDEX_NONE,
		FGameplayTag InputTagOverride = FGameplayTag()) const;

protected:
	/**
	 * 要授予的 Gameplay Ability
	 *
	 * 例如：
	 *   - GA_Weapon_Fire_Rifle
	 *   - GA_Weapon_Reload
	 *   - GA_Weapon_ToggleFireMode
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities", meta=(TitleProperty=Ability))
	TArray<FShootAbilitySet_GameplayAbility> GrantedGameplayAbilities;

	/**
	 * 要授予的 Gameplay Effect
	 *
	 * 例如：
	 *   - GE_Weapon_AmmoRegeneration（如果有）
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effects", meta=(TitleProperty=GameplayEffect))
	TArray<FShootAbilitySet_GameplayEffect> GrantedGameplayEffects;

	/**
	 * 要授予的 Attribute Set
	 *
	 * 通常武器不需要 AttributeSet，但预留接口
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Attribute Sets", meta=(TitleProperty=AttributeSet))
	TArray<FShootAbilitySet_AttributeSet> GrantedAttributes;
};
