// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ShootAbilitySystemLibrary.generated.h"

class UCharacterClassInfo;
class UAbilitySystemComponent;
class UShootSaveGame;
/**
 * 
 */
UCLASS()
class NEWWORLDORDER_API UShootAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, Category="ShootAbilitySystemLibrary|CharacterClassDefaults")
	static UCharacterClassInfo* GetCharacterClassInfo(const UObject* WorldContextObject);

	// 统一阵营判断入口：优先使用 TeamId，其次使用 Faction Tag
	UFUNCTION(BlueprintCallable, Category="ShootAbilitySystemLibrary|Team")
	static bool IsHostileActor(const AActor* SourceActor, const AActor* TargetActor);

	/**
	 * 统一解析 Actor、Pawn、Controller、PlayerState 与 Instigator 的队伍关系。
	 * Friendly/Hostile 表示双方都有明确归属；任一方无法解析时返回 Neutral。
	 */
	static ETeamAttitude::Type GetTeamAttitudeForActors(const AActor* SourceActor, const AActor* TargetActor);

	// 统一击杀归属判断：支持 Killer 本体、Instigator、Controller、Pawn 链路
	UFUNCTION(BlueprintCallable, Category="ShootAbilitySystemLibrary|Combat")
	static bool IsKillAttributedToActor(const AActor* KillerActor, const AActor* OwnerActor);
};
