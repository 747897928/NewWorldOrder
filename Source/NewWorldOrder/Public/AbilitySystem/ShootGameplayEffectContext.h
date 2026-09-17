// Copyright ZhaoYiJie

#pragma once

#include "GameplayEffectTypes.h"

#include "ShootGameplayEffectContext.generated.h"

/**
 * 项目自己的伤害上下文。
 *
 * 命中型武器只使用基类 HitResult；投射物额外把爆炸范围参数放在这里，
 * 由服务器上的 Damage Execution 读取。它不保存最终伤害值，最终伤害始终
 * 来自 Damage GE 的 BaseDamage 和统一 Execution。
 *
 * 范围参数只服务于服务器执行阶段，不参与客户端预测或复制，因此 NetSerialize
 * 只转发基类上下文，避免把一次性爆炸计算参数扩散到网络状态。
 */
USTRUCT()
struct FShootGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

	FShootGameplayEffectContext()
		: FGameplayEffectContext()
	{
	}

	FShootGameplayEffectContext(AActor* InInstigator, AActor* InEffectCauser)
		: FGameplayEffectContext(InInstigator, InEffectCauser)
	{
	}

	static FShootGameplayEffectContext* ExtractEffectContext(FGameplayEffectContextHandle Handle);

	void SetRadialDamageParameters(float InInnerRadius, float InOuterRadius, float InFalloff,
		bool bInApplyMaterialMultipliers)
	{
		bHasRadialDamage = true;
		DamageInnerRadius = InInnerRadius;
		DamageOuterRadius = InOuterRadius;
		DamageFalloff = InFalloff;
		bApplyMaterialMultipliers = bInApplyMaterialMultipliers;
	}

	bool HasRadialDamage() const { return bHasRadialDamage; }
	float GetDamageInnerRadius() const { return DamageInnerRadius; }
	float GetDamageOuterRadius() const { return DamageOuterRadius; }
	float GetDamageFalloff() const { return DamageFalloff; }
	bool ShouldApplyMaterialMultipliers() const { return bApplyMaterialMultipliers; }

	virtual FGameplayEffectContext* Duplicate() const override
	{
		FShootGameplayEffectContext* NewContext = new FShootGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			NewContext->AddHitResult(*GetHitResult(), true);
		}
		return NewContext;
	}

	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FShootGameplayEffectContext::StaticStruct();
	}

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess) override;

private:
	bool bHasRadialDamage = false;
	float DamageInnerRadius = 0.0f;
	float DamageOuterRadius = 0.0f;
	float DamageFalloff = 0.0f;
	bool bApplyMaterialMultipliers = false;
};

template<>
struct TStructOpsTypeTraits<FShootGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FShootGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true
	};
};
