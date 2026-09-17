// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AttributeInfo.generated.h"

USTRUCT(BlueprintType)
struct FShootAttributeInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AttributeTag = FGameplayTag();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText AttributeName = FText();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText AttributeDescription = FText();

	UPROPERTY(BlueprintReadOnly)
	float AttributeValue = 0.f;

	// 添加相等运算符重载
	bool operator==(const FShootAttributeInfo& Other) const
	{
		return AttributeTag == Other.AttributeTag &&
			AttributeName.EqualTo(Other.AttributeName) &&
			AttributeDescription.EqualTo(Other.AttributeDescription) &&
			FMath::IsNearlyEqual(AttributeValue, Other.AttributeValue);
	}
};

/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API UAttributeInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	FShootAttributeInfo FindAttributeInfoForTag(const FGameplayTag& AttributeTag, bool bLogNotFound = false) const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FShootAttributeInfo> AttributeInformation;
};
