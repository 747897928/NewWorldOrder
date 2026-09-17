// Copyright ZhaoYiJie

#pragma once

#include "Engine/DataAsset.h"

#include "ShootDamagePopStyleNiagara.generated.h"

class UNiagaraSystem;

/**
 * Niagara 伤害数字的数据驱动样式。
 * 具体 Niagara 资产由项目 DataAsset 配置，C++ 不硬编码 /Game 路径。
 */
UCLASS(BlueprintType, Const)
class NEWWORLDORDER_API UShootDamagePopStyleNiagara : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Niagara System 中接收 FVector4 数组的参数名；XYZ 为世界坐标，W 为伤害值。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage Pop")
	FName NiagaraArrayName = TEXT("DamageInfo");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage Pop")
	TObjectPtr<UNiagaraSystem> TextNiagara;
};
