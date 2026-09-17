// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "ShootAssetManager.generated.h"

/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API UShootAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UShootAssetManager& Get();

protected:
	virtual void StartInitialLoading() override;
};
