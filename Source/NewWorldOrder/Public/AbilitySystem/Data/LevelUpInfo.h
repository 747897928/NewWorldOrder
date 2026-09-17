// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelUpInfo.generated.h"

USTRUCT(BlueprintType)
struct FShootLevelUpInfo
{
	GENERATED_BODY()

	//多少经验才能升一级
	UPROPERTY(EditDefaultsOnly)
	int32 LevelUpRequirement = 0;

	//升一级奖励多少属性点
	UPROPERTY(EditDefaultsOnly)
	int32 AttributePointAward = 1;

	//升一级奖励多少技能点
	UPROPERTY(EditDefaultsOnly)
	int32 SpellPointAward = 1;
};
/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API ULevelUpInfo : public UDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly)
	TArray<FShootLevelUpInfo> LevelUpInformation;

	int32 FindLevelForXP(int32 XP) const;
	
};
