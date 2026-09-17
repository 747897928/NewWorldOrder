// Copyright ZhaoYiJie


#include "AbilitySystem/ShootAbilitySystemLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "ShootGameplayTags.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "GameModes/ShootGameModeBase.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Teams/LyraTeamAgentInterface.h"

namespace
{
	const AActor* ResolveOwnerAttributionActor(const AActor* InActor)
	{
		if (!InActor)
		{
			return nullptr;
		}

		if (const APawn* Pawn = Cast<APawn>(InActor))
		{
			return Pawn;
		}

		if (const AController* Controller = Cast<AController>(InActor))
		{
			if (const APawn* ControlledPawn = Controller->GetPawn())
			{
				return ControlledPawn;
			}
			return Controller;
		}

		return InActor;
	}

	bool TryGetTeamIdFromActor(const AActor* Actor, FGenericTeamId& OutTeamId)
	{
		if (!Actor)
		{
			return false;
		}

		if (const ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(Actor))
		{
			OutTeamId = TeamAgent->GetGenericTeamId();
			return OutTeamId != FGenericTeamId::NoTeam;
		}

		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			if (const ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(Pawn->GetPlayerState()))
			{
				OutTeamId = TeamAgent->GetGenericTeamId();
				return OutTeamId != FGenericTeamId::NoTeam;
			}

			if (const AController* Controller = Pawn->GetController())
			{
				if (const ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(Controller))
				{
					OutTeamId = TeamAgent->GetGenericTeamId();
					return OutTeamId != FGenericTeamId::NoTeam;
				}
			}
		}

		if (const AController* Controller = Cast<AController>(Actor))
		{
			if (const ILyraTeamAgentInterface* TeamAgent = Cast<ILyraTeamAgentInterface>(Controller))
			{
				OutTeamId = TeamAgent->GetGenericTeamId();
				return OutTeamId != FGenericTeamId::NoTeam;
			}
		}

		// 发射体、装备表现 Actor 等通常通过 Instigator 归属真正的队伍代理。
		if (const AActor* Instigator = Actor->GetInstigator(); Instigator && Instigator != Actor)
		{
			return TryGetTeamIdFromActor(Instigator, OutTeamId);
		}

		return false;
	}

	bool TryGetFactionTagFromActor(const AActor* Actor, FGameplayTag& OutFactionTag)
	{
		if (!Actor)
		{
			return false;
		}

		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(
			const_cast<AActor*>(Actor));
		if (!ASC)
		{
			return false;
		}

		const FShootGameplayTags& Tags = FShootGameplayTags::Get();
		if (Tags.Faction_Player.IsValid() && ASC->HasMatchingGameplayTag(Tags.Faction_Player))
		{
			OutFactionTag = Tags.Faction_Player;
			return true;
		}
		if (Tags.Faction_Enemy.IsValid() && ASC->HasMatchingGameplayTag(Tags.Faction_Enemy))
		{
			OutFactionTag = Tags.Faction_Enemy;
			return true;
		}

		return false;
	}
}

// 属性初始化已收敛为“每次进副本默认初始化”(InitializeDefaultAttributes);存档恢复路径已移除。

UCharacterClassInfo* UShootAbilitySystemLibrary::GetCharacterClassInfo(const UObject* WorldContextObject)
{
	const AShootGameModeBase* ShootGameModeBase = Cast<AShootGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (ShootGameModeBase == nullptr) return nullptr;
	return ShootGameModeBase->CharacterClassInfo;
}

bool UShootAbilitySystemLibrary::IsHostileActor(const AActor* SourceActor, const AActor* TargetActor)
{
	return GetTeamAttitudeForActors(SourceActor, TargetActor) == ETeamAttitude::Hostile;
}

ETeamAttitude::Type UShootAbilitySystemLibrary::GetTeamAttitudeForActors(
	const AActor* SourceActor, const AActor* TargetActor)
{
	if (!SourceActor || !TargetActor)
	{
		return ETeamAttitude::Neutral;
	}

	if (SourceActor == TargetActor)
	{
		return ETeamAttitude::Friendly;
	}

	FGenericTeamId SourceTeamId = FGenericTeamId::NoTeam;
	FGenericTeamId TargetTeamId = FGenericTeamId::NoTeam;
	if (TryGetTeamIdFromActor(SourceActor, SourceTeamId) && TryGetTeamIdFromActor(TargetActor, TargetTeamId))
	{
		return SourceTeamId == TargetTeamId ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
	}

	// TeamId 获取失败时回退到 Faction Tag，避免无 Controller 的目标被忽略
	FGameplayTag SourceFaction;
	FGameplayTag TargetFaction;
	if (TryGetFactionTagFromActor(SourceActor, SourceFaction) && TryGetFactionTagFromActor(TargetActor, TargetFaction))
	{
		return SourceFaction == TargetFaction ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
	}

	return ETeamAttitude::Neutral;
}

bool UShootAbilitySystemLibrary::IsKillAttributedToActor(const AActor* KillerActor, const AActor* OwnerActor)
{
	const AActor* ResolvedOwner = ResolveOwnerAttributionActor(OwnerActor);
	if (!ResolvedOwner || !KillerActor)
	{
		return false;
	}

	const AActor* ResolvedKiller = ResolveOwnerAttributionActor(KillerActor);
	if (ResolvedKiller == ResolvedOwner)
	{
		return true;
	}

	// 补充 Projectile/Weapon 等 Actor 的 Instigator 链路归属
	const AActor* InstigatorActor = KillerActor->GetInstigator();
	const AActor* ResolvedInstigator = ResolveOwnerAttributionActor(InstigatorActor);
	if (ResolvedInstigator == ResolvedOwner)
	{
		return true;
	}

	// 若 Instigator 是 Controller，再追一层 Pawn，兼容不同发射体写法
	if (const AController* InstigatorController = Cast<AController>(InstigatorActor))
	{
		if (const APawn* InstigatorPawn = InstigatorController->GetPawn())
		{
			return ResolveOwnerAttributionActor(InstigatorPawn) == ResolvedOwner;
		}
	}

	return false;
}
