// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/ShootGA_Weapon_Fire_Rifle.h"
#include "ShootGA_Weapon_Fire_Pistol.generated.h"

/**
 * 手枪开火能力。
 *
 * 手枪与步枪使用相同的服务器权威 Trace、伤害 GE 和本地后坐力调用链；差异只在
 * GameplayCue 标签与默认射击节奏。这样 AbilitySet 仍以 WeaponInstance 作为 SourceObject，
 * 不会另起一条绕过 RuntimeOnly QuickBar 的旧武器逻辑。
 */
UCLASS()
class UShootGA_Weapon_Fire_Pistol : public UShootGA_Weapon_Fire_Rifle
{
	GENERATED_BODY()

public:
	UShootGA_Weapon_Fire_Pistol(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
