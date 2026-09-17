// Copyright ZhaoYiJie

#include "UI/ViewModel/AttributeViewModel.h"

#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystem/ShootAttributeSet.h"
#include "AbilitySystem/Data/AttributeInfo.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "Player/ShootPlayerState.h"
#include "ShootGameplayTags.h"

UAttributeViewModel::UAttributeViewModel()
{
	// 不要在构造函数中加载资源，只进行基本的初始化
	InitAttributeMappings();

	// 设置默认值
	SetHealth(100.f);
	SetMaxHealth(100.f);
	SetPlayerName(FText::GetEmpty());
	SetShieldValue(0.f);
}

void UAttributeViewModel::BeginDestroy()
{
	Cleanup();
	Super::BeginDestroy();
}

// 基础属性Getter/Setter实现
float UAttributeViewModel::GetHealth() const { return Health; }

void UAttributeViewModel::SetHealth(float NewHealth)
{
	//SET_PROPERTY_VALUE宏的作用与BROADCAST_FIELD_VALUE宏基本相同，不同的是SET_PROPERTY_VALUE宏会在赋值并广播之前检查值是否已更改。
	//这项检查在为Viewmodel创建Setter函数时很常见，将其包括在内是为了方便起见。
	if (UE_MVVM_SET_PROPERTY_VALUE(Health, NewHealth))
	{
		RefreshHealthPercent();
	}
}

float UAttributeViewModel::GetMaxHealth() const { return MaxHealth; }

void UAttributeViewModel::SetMaxHealth(float NewMaxHealth)
{
	if (UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, NewMaxHealth))
	{
		RefreshHealthPercent();
	}
}

FText UAttributeViewModel::GetPlayerName() const { return PlayerName; }

void UAttributeViewModel::SetPlayerName(FText NewPlayerName)
{
	UE_MVVM_SET_PROPERTY_VALUE(PlayerName, MoveTemp(NewPlayerName));
}

float UAttributeViewModel::GetHealthPercent() const { return HealthPercent; }

void UAttributeViewModel::RefreshHealthPercent()
{
	const float NewPercent = MaxHealth > UE_SMALL_NUMBER
		? FMath::Clamp(Health / MaxHealth, 0.f, 1.f)
		: 0.f;
	UE_MVVM_SET_PROPERTY_VALUE(HealthPercent, NewPercent);
}

float UAttributeViewModel::GetShieldValue() const { return ShieldValue; }

void UAttributeViewModel::SetShieldValue(float NewShieldValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(ShieldValue, NewShieldValue);
}

int32 UAttributeViewModel::GetCurrentLevel() const { return CurrentLevel; }

void UAttributeViewModel::SetCurrentLevel(int32 NewCurrentLevel)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentLevel, NewCurrentLevel);
}

float UAttributeViewModel::GetCurrentXPBarPercent() const { return CurrentXPBarPercent; }

void UAttributeViewModel::SetCurrentXPBarPercent(float NewCurrentXPBarPercent)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentXPBarPercent, NewCurrentXPBarPercent);
}

// 属性值Getter/Setter实现
float UAttributeViewModel::GetStrengthValue() const { return StrengthValue; }

void UAttributeViewModel::SetStrengthValue(float NewStrengthValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(StrengthValue, NewStrengthValue);
}

float UAttributeViewModel::GetVitalityValue() const { return VitalityValue; }

void UAttributeViewModel::SetVitalityValue(float NewVitalityValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(VitalityValue, NewVitalityValue);
}

float UAttributeViewModel::GetAgilityValue() const { return AgilityValue; }

void UAttributeViewModel::SetAgilityValue(float NewAgilityValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(AgilityValue, NewAgilityValue);
}

float UAttributeViewModel::GetPerceptionValue() const { return PerceptionValue; }

void UAttributeViewModel::SetPerceptionValue(float NewPerceptionValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(PerceptionValue, NewPerceptionValue);
}

float UAttributeViewModel::GetArmorValue() const { return ArmorValue; }

void UAttributeViewModel::SetArmorValue(float NewArmorValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(ArmorValue, NewArmorValue);
}

float UAttributeViewModel::GetArmorPenetrationValue() const { return ArmorPenetrationValue; }

void UAttributeViewModel::SetArmorPenetrationValue(float NewArmorPenetrationValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(ArmorPenetrationValue, NewArmorPenetrationValue);
}

float UAttributeViewModel::GetCriticalHitChanceValue() const { return CriticalHitChanceValue; }

void UAttributeViewModel::SetCriticalHitChanceValue(float NewCriticalHitChanceValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CriticalHitChanceValue, NewCriticalHitChanceValue);
}

float UAttributeViewModel::GetCriticalHitDamageValue() const { return CriticalHitDamageValue; }

void UAttributeViewModel::SetCriticalHitDamageValue(float NewCriticalHitDamageValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CriticalHitDamageValue, NewCriticalHitDamageValue);
}

float UAttributeViewModel::GetShieldCapacityValue() const { return ShieldCapacityValue; }

void UAttributeViewModel::SetShieldCapacityValue(float NewShieldCapacityValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(ShieldCapacityValue, NewShieldCapacityValue);
}

float UAttributeViewModel::GetDamageReductionValue() const { return DamageReductionValue; }

void UAttributeViewModel::SetDamageReductionValue(float NewDamageReductionValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(DamageReductionValue, NewDamageReductionValue);
}

int32 UAttributeViewModel::GetAttributePointsValue() const
{
	return AttributePointsValue;
}

void UAttributeViewModel::SetAttributePointsValue(int32 NewAttributePointsValue)
{
	UE_MVVM_SET_PROPERTY_VALUE(AttributePointsValue, NewAttributePointsValue);
}

void UAttributeViewModel::InitAttributeMappings()
{
	const FShootGameplayTags& GameplayTags = FShootGameplayTags::Get();

	// 初始化属性标签到setter的映射
	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Primary_Strength,
	                                 [this](float Value) { SetStrengthValue(Value); });
	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Primary_Vitality,
	                                 [this](float Value) { SetVitalityValue(Value); });
	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Primary_Agility,
	                                 [this](float Value) { SetAgilityValue(Value); });
	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Primary_Perception,
	                                 [this](float Value) { SetPerceptionValue(Value); });

	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Secondary_Armor,
	                                 [this](float Value) { SetArmorValue(Value); });
	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Secondary_ArmorPenetration,
	                                 [this](float Value) { SetArmorPenetrationValue(Value); });
	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Secondary_CriticalHitChance,
	                                 [this](float Value) { SetCriticalHitChanceValue(Value); });
	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Secondary_CriticalHitDamage,
	                                 [this](float Value) { SetCriticalHitDamageValue(Value); });
	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Secondary_ShieldCapacity,
	                                 [this](float Value) { SetShieldCapacityValue(Value); });
	AttributeTagToValueSetterMap.Add(GameplayTags.Attributes_Secondary_DamageReduction,
	                                 [this](float Value) { SetDamageReductionValue(Value); });

	// 初始化属性标签到getter的映射
	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Primary_Strength, [this]() { return GetStrengthValue(); });
	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Primary_Vitality,
	                                 [this]() { return GetVitalityValue(); });
	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Primary_Agility,
	                                 [this]() { return GetAgilityValue(); });
	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Primary_Perception, [this]() { return GetPerceptionValue(); });

	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Secondary_Armor, [this]() { return GetArmorValue(); });
	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Secondary_ArmorPenetration,
	                                 [this]() { return GetArmorPenetrationValue(); });
	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Secondary_CriticalHitChance,
	                                 [this]() { return GetCriticalHitChanceValue(); });
	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Secondary_CriticalHitDamage,
	                                 [this]() { return GetCriticalHitDamageValue(); });
	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Secondary_ShieldCapacity,
	                                 [this]() { return GetShieldCapacityValue(); });
	AttributeTagToValueGetterMap.Add(GameplayTags.Attributes_Secondary_DamageReduction,
	                                 [this]() { return GetDamageReductionValue(); });
}

void UAttributeViewModel::SetPlayerState(AShootPlayerState* InPlayerState)
{
	Cleanup();
	PlayerState = InPlayerState;

	if (PlayerState.IsValid())
	{
		AbilitySystemComponent = Cast<UShootAbilitySystemComponent>(PlayerState->GetAbilitySystemComponent());
		AttributeSet = Cast<UShootAttributeSet>(PlayerState->GetAttributeSet());
		BindCallbacksToDependencies();
	}
	else
	{
		AbilitySystemComponent = nullptr;
		AttributeSet = nullptr;
	}
}

AShootPlayerState* UAttributeViewModel::GetPlayerState()
{
	return PlayerState.Get();
}

UShootAbilitySystemComponent* UAttributeViewModel::GetShootAbilitySystemComponent()
{
	if (!AbilitySystemComponent.IsValid() && PlayerState.IsValid())
	{
		AbilitySystemComponent = Cast<UShootAbilitySystemComponent>(PlayerState->GetAbilitySystemComponent());
	}
	return AbilitySystemComponent.Get();
}

UShootAttributeSet* UAttributeViewModel::GetShootAttributeSet()
{
	if (!AttributeSet.IsValid() && PlayerState.IsValid())
	{
		AttributeSet = Cast<UShootAttributeSet>(PlayerState->GetAttributeSet());
	}
	return AttributeSet.Get();
}

void UAttributeViewModel::InitializeWithPlayerState(AShootPlayerState* InPlayerState)
{
	SetPlayerState(InPlayerState);

	if (PlayerState.IsValid() && AttributeSet.IsValid())
	{
		SetPlayerName(FText::FromString(PlayerState->GetPlayerName()));
		// 分屏第二个 Pawn 的 ASC 可能先存在、默认属性 GE 后到达。此时 0/0 不是死亡，不能先广播给 HUD。
		TryInitializeHealthSnapshot();
		SetShieldValue(AttributeSet->GetShield());
		SetCurrentLevel(PlayerState->GetPlayerLevel());
		SetAttributePointsValue(PlayerState->GetAttributePoints());

		// 初始化所有属性值
		for (const auto& Pair : AttributeTagToValueSetterMap)
		{
			const FGameplayTag& AttributeTag = Pair.Key;
			if (const TStaticFuncPtr<FGameplayAttribute()>* FuncPtr = AttributeSet->TagsToAttributes.Find(AttributeTag))
			{
				FGameplayAttribute Attribute = (*FuncPtr)();
				float InitialValue = Attribute.GetNumericValue(AttributeSet.Get());
				UpdateAttributeInfo(AttributeTag, InitialValue);
			}
		}
	}
	else
	{
		// 世界名牌可能在 PlayerState 尚未复制完成时先构造；等待 Character 的 OnRep_PlayerState 重新初始化。
		// 缺少数据源不等于角色死亡，因此这里不能广播 Health=0 触发 HUD 的 OnEliminated。
		SetPlayerName(FText::GetEmpty());
		SetShieldValue(0.f);
	}
}

void UAttributeViewModel::Cleanup()
{
	UnBindCallbacksToDependencies();
	bHasValidHealthSnapshot = false;

	PlayerState = nullptr;
	AbilitySystemComponent = nullptr;
	AttributeSet = nullptr;
}

void UAttributeViewModel::BindCallbacksToDependencies()
{
	if (!EnsureDependencies()) return;

	// 绑定玩家状态委托
	XPChangedHandle = PlayerState->OnXPChangedDelegate.AddUObject(this, &UAttributeViewModel::OnXPChanged);
	LevelChangedHandle = PlayerState->OnLevelChangedDelegate.AddUObject(this, &UAttributeViewModel::HandleLevelChanged);
	AttributePointsChangedHandle = PlayerState->OnAttributePointsChangedDelegate.AddLambda([this](int32 NewPoints)
	{
		// 处理属性点变化
		SetAttributePointsValue(NewPoints);
	});

	// 绑定基础属性委托
	HealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
	                      .AddLambda([this](const FOnAttributeChangeData& Data)
	                      {
		                      if (bHasValidHealthSnapshot)
		                      {
			                      SetHealth(Data.NewValue);
		                      }
		                      else if (Data.NewValue > 0.f)
		                      {
			                      TryInitializeHealthSnapshot();
		                      }
	                      });

	MaxHealthChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute())
	                      .AddLambda([this](const FOnAttributeChangeData& Data)
	                      {
		                      if (bHasValidHealthSnapshot)
		                      {
			                      SetMaxHealth(Data.NewValue);
		                      }
		                      else if (Data.NewValue > 0.f)
		                      {
			                      TryInitializeHealthSnapshot();
		                      }
	                      });

	// 调用链：ASC 的 Shield 复制/GE 修改 -> 本委托 -> FieldNotify ShieldValue -> W_Shieldbar。
	ShieldChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetShieldAttribute())
	                      .AddLambda([this](const FOnAttributeChangeData& Data)
	                      {
		                      SetShieldValue(Data.NewValue);
	                      });

	// 绑定其他属性委托
	for (const auto& Pair : AttributeTagToValueSetterMap)
	{
		const FGameplayTag& AttributeTag = Pair.Key;
		if (const TStaticFuncPtr<FGameplayAttribute()>* FuncPtr = AttributeSet->TagsToAttributes.Find(AttributeTag))
		{
			FGameplayAttribute Attribute = (*FuncPtr)();

			FDelegateHandle Handle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute)
			                                               .AddLambda([this, AttributeTag](
				                                               const FOnAttributeChangeData& Data)
				                                               {
					                                               UpdateAttributeInfo(AttributeTag, Data.NewValue);
				                                               });

			AttributeDelegateHandles.Add(AttributeTag, Handle);

			// 设置初始值
			float InitialValue = Attribute.GetNumericValue(AttributeSet.Get());
			UpdateAttributeInfo(AttributeTag, InitialValue);
		}
	}
}

void UAttributeViewModel::UnBindCallbacksToDependencies()
{
	// 移除玩家状态委托
	if (PlayerState.IsValid())
	{
		PlayerState->OnXPChangedDelegate.Remove(XPChangedHandle);
		PlayerState->OnLevelChangedDelegate.Remove(LevelChangedHandle);
		PlayerState->OnAttributePointsChangedDelegate.Remove(AttributePointsChangedHandle);
	}

	// 移除属性委托
	if (AbilitySystemComponent.IsValid() && AttributeSet.IsValid())
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			AttributeSet->GetHealthAttribute()).Remove(HealthChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			AttributeSet->GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			AttributeSet->GetShieldAttribute()).Remove(ShieldChangedHandle);

		for (const auto& Pair : AttributeDelegateHandles)
		{
			const FGameplayTag& AttributeTag = Pair.Key;
			if (const TStaticFuncPtr<FGameplayAttribute()>* FuncPtr = AttributeSet->TagsToAttributes.Find(AttributeTag))
			{
				FGameplayAttribute Attribute = (*FuncPtr)();
				AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).Remove(Pair.Value);
			}
		}
	}

	// 重置所有委托句柄
	XPChangedHandle.Reset();
	LevelChangedHandle.Reset();
	AttributePointsChangedHandle.Reset();
	HealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	ShieldChangedHandle.Reset();
	AttributeDelegateHandles.Empty();
}

bool UAttributeViewModel::EnsureDependencies()
{
	if (!PlayerState.IsValid() || !AbilitySystemComponent.IsValid() || !AttributeSet.IsValid())
	{
		// 尝试重新获取依赖
		if (PlayerState.IsValid())
		{
			AbilitySystemComponent = Cast<UShootAbilitySystemComponent>(PlayerState->GetAbilitySystemComponent());
			AttributeSet = Cast<UShootAttributeSet>(PlayerState->GetAttributeSet());
		}

		return PlayerState.IsValid() && AbilitySystemComponent.IsValid() && AttributeSet.IsValid();
	}

	return true;
}

bool UAttributeViewModel::TryInitializeHealthSnapshot()
{
	if (bHasValidHealthSnapshot || !AttributeSet.IsValid())
	{
		return bHasValidHealthSnapshot;
	}

	const float InitialHealth = AttributeSet->GetHealth();
	const float InitialMaxHealth = AttributeSet->GetMaxHealth();
	if (InitialHealth <= 0.f || InitialMaxHealth <= UE_SMALL_NUMBER)
	{
		return false;
	}

	// 先发布上限再发布当前值，避免比例绑定在旧 MaxHealth 上短暂计算错误。
	SetMaxHealth(InitialMaxHealth);
	SetHealth(InitialHealth);
	bHasValidHealthSnapshot = true;
	return true;
}

void UAttributeViewModel::UpdateAttributeInfo(const FGameplayTag& AttributeTag, float NewValue)
{
	if (AttributeTagToValueSetterMap.Contains(AttributeTag))
	{
		AttributeTagToValueSetterMap[AttributeTag](NewValue);
	}
}

void UAttributeViewModel::OnXPChanged(int32 NewXP)
{
	if (!PlayerState.IsValid()) return;

	const ULevelUpInfo* LevelUpInfo = PlayerState->LevelUpInfo;
	if (!LevelUpInfo) return;

	const int32 Level = LevelUpInfo->FindLevelForXP(NewXP);
	SetCurrentLevel(Level);
	const int32 MaxLevel = LevelUpInfo->LevelUpInformation.Num();

	if (Level <= MaxLevel && Level > 0)
	{
		const int32 LevelUpRequirement = LevelUpInfo->LevelUpInformation[Level].LevelUpRequirement;
		const int32 PreviousLevelUpRequirement = LevelUpInfo->LevelUpInformation[Level - 1].LevelUpRequirement;

		const int32 DeltaLevelRequirement = LevelUpRequirement - PreviousLevelUpRequirement;
		const int32 XPForThisLevel = NewXP - PreviousLevelUpRequirement;

		float XPBarPercent = static_cast<float>(XPForThisLevel) / static_cast<float>(DeltaLevelRequirement);
		SetCurrentXPBarPercent(XPBarPercent);
	}
}

void UAttributeViewModel::HandleLevelChanged(int32 NewLevel, bool bLevelUp)
{
	SetCurrentLevel(NewLevel);
	OnPlayerLevelChangedDelegate.Broadcast(NewLevel, bLevelUp);
}

void UAttributeViewModel::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
	if (UShootAbilitySystemComponent* ShootASC = GetShootAbilitySystemComponent())
	{
		ShootASC->UpgradeAttribute(AttributeTag);
	}
}
