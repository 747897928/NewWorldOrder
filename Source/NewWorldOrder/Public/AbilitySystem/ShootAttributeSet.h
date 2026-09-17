// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "ShootAttributeSet.generated.h"

// 这是一个非常常用的宏，用于为属性生成Getter和Setter函数。
// 例如：ATTRIBUTE_ACCESSORS(UShootAttributeSet, Health) 会生成：
//   GetHealthAttribute()  - 返回FGameplayAttribute类型的包装器，用于识别属性本身。
//   GetHealth()          - 返回当前属性值 (float)。
//   SetHealth()          - 设置属性值。
//   InitHealth()         - 初始化属性值。
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

USTRUCT()
struct FShootEffectContext
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY()
	TObjectPtr<AController> SourceController = nullptr;

	UPROPERTY()
	TObjectPtr<ACharacter> SourceCharacter = nullptr;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> TargetASC = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY()
	TObjectPtr<AController> TargetController = nullptr;

	UPROPERTY()
	TObjectPtr<ACharacter> TargetCharacter = nullptr;

	/** 原始命中位置和物理材质供服务器最终伤害结算后生成表现请求。 */
	UPROPERTY()
	FHitResult HitResult;

	UPROPERTY()
	bool bHasHitResult = false;

	UPROPERTY()
	FGameplayTagContainer SourceTags;

	UPROPERTY()
	FGameplayTagContainer TargetTags;
};

// 这是一个模板别名，用于简化复杂的函数指针类型声明。
// 在这段代码中，它用于后续的 TagsToAttributes 映射，目的是将一个GameplayTag映射到一个返回特定属性的函数。
// typedef is specific to the FGameplayAttribute() signature, but TStaticFunPtr is generic to any signature chosen
//typedef TBaseStaticDelegateInstance<FGameplayAttribute(), FDefaultDelegateUserPolicy>::FFuncPtr FAttributeFuncPtr;
// 这是一个类型别名（Type Alias），用于简化一个非常复杂的类型声明。
//TBaseStaticDelegateInstance<T, FDefaultDelegateUserPolicy>::FFuncPtr 是 Unreal Engine 委托系统内部的一个类型，它表示一个指向静态函数的指针。
//使用 using ... = ... 语法，我们创建了一个新的、简单的名字 TStaticFuncPtr<T> 来代表这个复杂的类型。
//TStaticFuncPtr<FGameplayAttribute()> 就等价于一个指向如下形式静态函数的指针：FGameplayAttribute GetStrengthAttribute()。
template <class T>
using TStaticFuncPtr = typename TBaseStaticDelegateInstance<T, FDefaultDelegateUserPolicy>::FFuncPtr;
/**
* UCLASS() 宏告诉UE生成这个类的反射数据，这样它就可以在编辑器、蓝图、反射系统中使用了。
* class NEWWORLDORDER_API 中的 `NEWWORLDORDER_API` 是你的项目模块的导出宏，确保其他模块能使用这个类。
 */
UCLASS()
class NEWWORLDORDER_API UShootAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UShootAttributeSet();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	TMap<FGameplayTag, TStaticFuncPtr<FGameplayAttribute()>> TagsToAttributes;

	//FGameplayAttribute(*)() 这个语法声明了一个类型：“指向一个函数的指针，该函数不接受任何参数并返回一个 FGameplayAttribute”。
	//这是一个标准的 C/C++ 函数指针类型声明
	//TMap<FGameplayTag, FGameplayAttribute(*)()> TagsToAttributes;
	
	/*
	 * Primary Attributes
	 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Strength, Category = "Primary Attributes")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, Strength);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Vitality, Category = "Primary Attributes")
	FGameplayAttributeData Vitality;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, Vitality);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Agility, Category = "Primary Attributes")
	FGameplayAttributeData Agility;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, Agility);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Perception, Category = "Primary Attributes")
	FGameplayAttributeData Perception;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, Perception);

	/*
	 * Secondary Attributes
	 */

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "Secondary Attributes")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, Armor);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ArmorPenetration, Category = "Secondary Attributes")
	FGameplayAttributeData ArmorPenetration;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, ArmorPenetration);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalHitChance, Category = "Secondary Attributes")
	FGameplayAttributeData CriticalHitChance;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, CriticalHitChance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalHitDamage, Category = "Secondary Attributes")
	FGameplayAttributeData CriticalHitDamage;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, CriticalHitDamage);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalHitResistance, Category = "Secondary Attributes")
	FGameplayAttributeData CriticalHitResistance;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, CriticalHitResistance);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "Vital Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ShieldCapacity, Category = "Secondary Attributes")
	FGameplayAttributeData ShieldCapacity;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, ShieldCapacity);

	// 当前护盾值：ShieldCapacity 是由体力计算出的最大护盾值，不能作为消耗对象复用。
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Shield, Category = "Vital Attributes")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, Shield);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageReduction, Category = "Secondary Attributes")
	FGameplayAttributeData DamageReduction;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, DamageReduction);

	/*
	 * Combat Buff Attributes（技能临时加成；武器不挂 ASC，统一在角色 ASC 上累加）
	 * 默认值：Multiplier 系列为 1.0，Bonus/加法系列为 0.0
	 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeedMultiplier, Category = "Combat Buff Attributes")
	FGameplayAttributeData MoveSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, MoveSpeedMultiplier);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FireRateMultiplier, Category = "Combat Buff Attributes")
	FGameplayAttributeData FireRateMultiplier;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, FireRateMultiplier);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ReloadSpeedMultiplier, Category = "Combat Buff Attributes")
	FGameplayAttributeData ReloadSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, ReloadSpeedMultiplier);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealingDoneMultiplier, Category = "Combat Buff Attributes")
	FGameplayAttributeData HealingDoneMultiplier;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, HealingDoneMultiplier);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HealingReceivedMultiplier, Category = "Combat Buff Attributes")
	FGameplayAttributeData HealingReceivedMultiplier;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, HealingReceivedMultiplier);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ShieldCapacityBonus, Category = "Combat Buff Attributes")
	FGameplayAttributeData ShieldCapacityBonus;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, ShieldCapacityBonus);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageReductionBonus, Category = "Combat Buff Attributes")
	FGameplayAttributeData DamageReductionBonus;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, DamageReductionBonus);

	/*
	 * Ultimate Charge Attributes
	 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_UltimateCharge, Category = "Ultimate")
	FGameplayAttributeData UltimateCharge;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, UltimateCharge);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_UltimateChargeMax, Category = "Ultimate")
	FGameplayAttributeData UltimateChargeMax;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, UltimateChargeMax);

	/*
	 * Vital Attributes
	 */

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Vital Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, Health);


	/*
	 * Meta Attributes
	 */

	UPROPERTY(BlueprintReadOnly, Category = "Meta Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UShootAttributeSet, IncomingDamage);

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth) const;

	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldStrength) const;

	UFUNCTION()
	void OnRep_Vitality(const FGameplayAttributeData& OldVitality) const;

	UFUNCTION()
	void OnRep_Agility(const FGameplayAttributeData& OldAgility) const;

	UFUNCTION()
	void OnRep_Perception(const FGameplayAttributeData& OldPerception) const;

	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldArmor) const;

	UFUNCTION()
	void OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration) const;

	UFUNCTION()
	void OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance) const;

	UFUNCTION()
	void OnRep_CriticalHitDamage(const FGameplayAttributeData& OldCriticalHitDamage) const;

	UFUNCTION()
	void OnRep_CriticalHitResistance(const FGameplayAttributeData& OldCriticalHitResistance) const;

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const;

	UFUNCTION()
	void OnRep_ShieldCapacity(const FGameplayAttributeData& OldShieldCapacity) const;

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldShield) const;

	UFUNCTION()
	void OnRep_DamageReduction(const FGameplayAttributeData& OldDamageReduction) const;

	UFUNCTION()
	void OnRep_MoveSpeedMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_FireRateMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_ReloadSpeedMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_HealingDoneMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_HealingReceivedMultiplier(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_ShieldCapacityBonus(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_DamageReductionBonus(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_UltimateCharge(const FGameplayAttributeData& OldValue) const;

	UFUNCTION()
	void OnRep_UltimateChargeMax(const FGameplayAttributeData& OldValue) const;

private:
	void HandleIncomingDamage(const FShootEffectContext& Context);
	void SendDamageNumberFeedback(const FShootEffectContext& Context, float ActualHealthDamage) const;
	void SendDamageTakenCue(const FShootEffectContext& Context, float ActualHealthDamage) const;
	void SetEffectContext(const FGameplayEffectModCallbackData& Data, FShootEffectContext& Context) const;
	bool bTopOffHealth = false;
};
