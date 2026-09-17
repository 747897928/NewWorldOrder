// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "AppearanceTagInfo.generated.h"

// 标签解析结果结构体
USTRUCT(BlueprintType)
struct FAppearanceTagInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FString Gender;
    
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FString ComponentName;
    
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FString ParamName;
    
	UPROPERTY(BlueprintReadOnly, Category = "Appearance")
	FString SelectedOptionName;
    
	// 判断是否是有效的四级标签
	bool IsFullOption() const
	{
		return !Gender.IsEmpty() && 
			   !ComponentName.IsEmpty() && 
			   !ParamName.IsEmpty() && 
			   !SelectedOptionName.IsEmpty();
	}
};
