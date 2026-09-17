// Copyright ZhaoYiJie

#pragma once

#include "Animation/AnimInstance.h"
#include "GameplayEffectTypes.h"
#include "ShootMannequinAnimInstance.generated.h"

class UAbilitySystemComponent;

/**
 * Lyra 主 AnimBP 的项目父类。
 *
 * 这是 Config/CoreRedirects 中 LyraAnimInstance 的真实落点。迁移进项目的
 * ABP_Mannequin_Base 以及正式 CC 版本都从这里获得 GameplayTag 属性映射和
 * GroundDistance；武器差异由 Linked Item Anim Layer 处理，不放进本类。
 */
UCLASS(Config=Game)
class NEWWORLDORDER_API UShootMannequinAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UShootMannequinAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void InitializeWithAbilitySystem(UAbilitySystemComponent* AbilitySystem);

protected:
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** ASC Tag 到 AnimBP 属性的自动映射；具体映射由主 AnimBP Class Defaults 配置。 */
	UPROPERTY(EditDefaultsOnly, Category="GameplayTags")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap;

	/** 胶囊底部到地面的距离，供 Jump/Fall/Land 状态和距离匹配使用。 */
	UPROPERTY(BlueprintReadOnly, Category="Character State Data")
	float GroundDistance = -1.0f;

private:
	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystem;
};
