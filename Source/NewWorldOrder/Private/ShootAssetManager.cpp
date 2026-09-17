// Copyright ZhaoYiJie


#include "ShootAssetManager.h"

#include "AbilitySystemGlobals.h"
#include "ShootGameplayTags.h"
#include "Engine/Engine.h"

UShootAssetManager& UShootAssetManager::Get()
{
	check(GEngine);
	UShootAssetManager* AuraAssetManager = Cast<UShootAssetManager>(GEngine->AssetManager);
	return *AuraAssetManager;
}

void UShootAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	FShootGameplayTags::InitializeNativeGameplayTags();

	// This is required to use Target Data!
	//Engine\UE5\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Private\AbilitySystemGlobals.cpp:85
	//InitTargetDataScriptStructCache();
	UAbilitySystemGlobals::Get().InitGlobalData();
}
