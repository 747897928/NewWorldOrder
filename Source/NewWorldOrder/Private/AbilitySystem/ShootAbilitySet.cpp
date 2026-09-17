// Copyright ZhaoYiJie

#include "AbilitySystem/ShootAbilitySet.h"
#include "AbilitySystem/ShootAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/ShootGameplayAbility.h"
#include "Logging/StructuredLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ShootAbilitySet)

// ============================================================================
// FShootAbilitySet_GrantedHandles
// ============================================================================

void FShootAbilitySet_GrantedHandles::AddAbilitySpecHandle(const FGameplayAbilitySpecHandle& Handle)
{
	if (Handle.IsValid())
	{
		AbilitySpecHandles.Add(Handle);
	}
}

void FShootAbilitySet_GrantedHandles::AddGameplayEffectHandle(const FActiveGameplayEffectHandle& Handle)
{
	if (Handle.IsValid())
	{
		GameplayEffectHandles.Add(Handle);
	}
}

void FShootAbilitySet_GrantedHandles::AddAttributeSet(UAttributeSet* Set)
{
	if (Set)
	{
		GrantedAttributeSets.Add(Set);
	}
}

void FShootAbilitySet_GrantedHandles::TakeFromAbilitySystem(UShootAbilitySystemComponent* ShootASC)
{
	check(ShootASC);

	if (!ShootASC->IsOwnerActorAuthoritative())
	{
		// 必须在服务器上执行
		return;
	}

	// 移除所有授予的 Ability
	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		if (Handle.IsValid())
		{
			ShootASC->ClearAbility(Handle);
		}
	}

	// 移除所有授予的 GameplayEffect
	for (const FActiveGameplayEffectHandle& Handle : GameplayEffectHandles)
	{
		if (Handle.IsValid())
		{
			ShootASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	// 动态 AttributeSet 必须从 ASC 的 SpawnedAttributes 中注销，不能只清空本地引用。
	// PlayerState ASC 会跨 Pawn/Experience 存活；若只 Empty，旧属性集仍会参与属性查找和复制，
	// 下一次 Experience 再授予时还会叠出第二个同类实例。
	for (UAttributeSet* Set : GrantedAttributeSets)
	{
		if (IsValid(Set))
		{
			ShootASC->RemoveSpawnedAttribute(Set);
		}
	}
	GrantedAttributeSets.Empty();

	// 清空所有句柄
	AbilitySpecHandles.Empty();
	GameplayEffectHandles.Empty();
}

// ============================================================================
// UShootAbilitySet
// ============================================================================

UShootAbilitySet::UShootAbilitySet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UShootAbilitySet::GiveToAbilitySystem(UShootAbilitySystemComponent* ShootASC,
	FShootAbilitySet_GrantedHandles* OutGrantedHandles, UObject* SourceObject,
	int32 AbilityLevelOverride, FGameplayTag InputTagOverride) const
{
	check(ShootASC);

	if (!ShootASC->IsOwnerActorAuthoritative())
	{
		// 必须在服务器上执行
		return;
	}

	// ============================================================================
	// 1. 授予 Gameplay Abilities
	// ============================================================================
	for (int32 AbilityIndex = 0; AbilityIndex < GrantedGameplayAbilities.Num(); ++AbilityIndex)
	{
		const FShootAbilitySet_GameplayAbility& AbilityToGrant = GrantedGameplayAbilities[AbilityIndex];

		if (!IsValid(AbilityToGrant.Ability))
		{
			UE_LOGFMT(LogTemp, Error, "GrantedGameplayAbilities[{0}] on ability set [{1}] is not valid.",
				AbilityIndex, GetNameSafe(this));
			continue;
		}

		// 创建 AbilitySpec
		const int32 AbilityLevel = AbilityLevelOverride > 0
			? AbilityLevelOverride
			: AbilityToGrant.AbilityLevel;
		FGameplayAbilitySpec AbilitySpec(AbilityToGrant.Ability, AbilityLevel);
		AbilitySpec.SourceObject = SourceObject; // 重要！设置 SourceObject 为武器实例
		// SkillLoadout 是唯一会传入 InputTagOverride 的调用方：技能落入哪个槽，就在本次授予时使用哪个槽位标签。
		// Experience、武器、被动和机器人自身 AbilitySet 均不传覆盖值，继续使用各自固定标签或保持无输入。
		const FGameplayTag EffectiveInputTag = InputTagOverride.IsValid()
			? InputTagOverride
			: AbilityToGrant.InputTag;
		if (EffectiveInputTag.IsValid())
		{
			AbilitySpec.GetDynamicSpecSourceTags().AddTag(EffectiveInputTag);
		}

		// 授予 Ability
		const FGameplayAbilitySpecHandle AbilitySpecHandle = ShootASC->GiveAbility(AbilitySpec);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAbilitySpecHandle(AbilitySpecHandle);
		}
	}

	// ============================================================================
	// 2. 授予 Gameplay Effects
	// ============================================================================
	for (int32 EffectIndex = 0; EffectIndex < GrantedGameplayEffects.Num(); ++EffectIndex)
	{
		const FShootAbilitySet_GameplayEffect& EffectToGrant = GrantedGameplayEffects[EffectIndex];

		if (!IsValid(EffectToGrant.GameplayEffect))
		{
			UE_LOGFMT(LogTemp, Error, "GrantedGameplayEffects[{0}] on ability set [{1}] is not valid.",
				EffectIndex, GetNameSafe(this));
			continue;
		}

		// 创建 EffectContext
		const UGameplayEffect* GameplayEffect = EffectToGrant.GameplayEffect->GetDefaultObject<UGameplayEffect>();
		FGameplayEffectContextHandle EffectContext = ShootASC->MakeEffectContext();
		EffectContext.AddSourceObject(SourceObject);

		// 应用 Effect
		const FActiveGameplayEffectHandle GameplayEffectHandle = ShootASC->ApplyGameplayEffectToSelf(
			GameplayEffect,
			EffectToGrant.EffectLevel,
			EffectContext
		);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddGameplayEffectHandle(GameplayEffectHandle);
		}
	}

	// ============================================================================
	// 3. 授予 Attribute Sets
	// ============================================================================
	for (int32 SetIndex = 0; SetIndex < GrantedAttributes.Num(); ++SetIndex)
	{
		const FShootAbilitySet_AttributeSet& SetToGrant = GrantedAttributes[SetIndex];

		if (!IsValid(SetToGrant.AttributeSet))
		{
			UE_LOGFMT(LogTemp, Error, "GrantedAttributes[{0}] on ability set [{1}] is not valid.",
				SetIndex, GetNameSafe(this));
			continue;
		}

		// 获取或创建 AttributeSet
		UAttributeSet* NewSet = NewObject<UAttributeSet>(ShootASC->GetOwner(), SetToGrant.AttributeSet);
		ShootASC->AddAttributeSetSubobject(NewSet);

		if (OutGrantedHandles)
		{
			OutGrantedHandles->AddAttributeSet(NewSet);
		}
	}
}
