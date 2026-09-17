// Copyright NewWorldOrder Game. All Rights Reserved.

#include "AbilitySystem/Skills/ShootSkillLoadoutComponent.h"
#include "AbilitySystem/Skills/ShootSkillDefinition.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Player/ShootPlayerState.h"

namespace ShootSkillDebugCommands
{
	int32 ParsePlayerIndex(const TArray<FString>& Args)
	{
		return Args.IsValidIndex(0) ? FMath::Max(0, FCString::Atoi(*Args[0])) : 0;
	}

	UShootSkillLoadoutComponent* ResolveLoadout(UWorld* World, int32 PlayerIndex, APawn*& OutPawn)
	{
		OutPawn = nullptr;
		APlayerController* PlayerController = World
			? UGameplayStatics::GetPlayerController(World, PlayerIndex)
			: nullptr;
		OutPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		AShootPlayerState* PlayerState = PlayerController
			? PlayerController->GetPlayerState<AShootPlayerState>()
			: nullptr;
		return PlayerState ? PlayerState->GetSkillLoadoutComponent() : nullptr;
	}

	void DumpLoadout(UWorld* World, int32 PlayerIndex)
	{
		APawn* Pawn = nullptr;
		UShootSkillLoadoutComponent* Loadout = ResolveLoadout(World, PlayerIndex, Pawn);
		if (!Loadout)
		{
			UE_LOG(LogTemp, Warning, TEXT("Shoot.Skill: Player %d has no SkillLoadoutComponent."), PlayerIndex);
			return;
		}

		UE_LOG(LogTemp, Display, TEXT("Shoot.Skill: Player %d loadout"), PlayerIndex);
		for (int32 SlotIndex = 0; SlotIndex < Loadout->GetSlotCount(); ++SlotIndex)
		{
			const FShootSkillSlot Slot = Loadout->GetSlot(SlotIndex);
			UE_LOG(LogTemp, Display, TEXT("  Slot %d: %s, Level %d, InputTag %s"),
				SlotIndex,
				*GetNameSafe(Slot.SkillDefinition),
				Slot.Level,
				*Loadout->GetSlotInputTag(SlotIndex).ToString());
		}
	}

	void AcquireRandom(const TArray<FString>& Args, UWorld* World)
	{
		const int32 PlayerIndex = ParsePlayerIndex(Args);
		const int32 Count = Args.IsValidIndex(1) ? FMath::Clamp(FCString::Atoi(*Args[1]), 1, 64) : 1;
		APawn* Pawn = nullptr;
		UShootSkillLoadoutComponent* Loadout = ResolveLoadout(World, PlayerIndex, Pawn);
		if (!Loadout || !Pawn || !Pawn->HasAuthority())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Shoot.Skill.AcquireRandom must run in an authority game world for Player %d."), PlayerIndex);
			return;
		}

		for (int32 Iteration = 0; Iteration < Count; ++Iteration)
		{
			int32 SlotIndex = INDEX_NONE;
			EShootSkillAcquireResult Result = EShootSkillAcquireResult::InvalidRequest;
			if (!Loadout->AcquireRandomSkill(Pawn, SlotIndex, Result))
			{
				UE_LOG(LogTemp, Warning, TEXT("Shoot.Skill.AcquireRandom stopped at %d/%d, result=%d."),
					Iteration, Count, static_cast<int32>(Result));
				break;
			}
			UE_LOG(LogTemp, Display, TEXT("Shoot.Skill.AcquireRandom changed Slot %d, result=%d."),
				SlotIndex, static_cast<int32>(Result));
		}
		DumpLoadout(World, PlayerIndex);
	}

	void Clear(const TArray<FString>& Args, UWorld* World)
	{
		const int32 PlayerIndex = ParsePlayerIndex(Args);
		APawn* Pawn = nullptr;
		UShootSkillLoadoutComponent* Loadout = ResolveLoadout(World, PlayerIndex, Pawn);
		if (!Loadout || !Pawn || !Pawn->HasAuthority())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("Shoot.Skill.Clear must run in an authority game world for Player %d."), PlayerIndex);
			return;
		}

		// 只调用正式的 Match 清理入口，确保调试不会另造一条能力撤销实现。
		Loadout->ClearMatchSkills();
		DumpLoadout(World, PlayerIndex);
	}

	void Dump(const TArray<FString>& Args, UWorld* World)
	{
		DumpLoadout(World, ParsePlayerIndex(Args));
	}

	FAutoConsoleCommandWithWorldAndArgs AcquireRandomCommand(
		TEXT("Shoot.Skill.AcquireRandom"),
		TEXT("随机获取/升级 Match Skill。参数：[PlayerIndex=0] [Count=1]。仅 Authority PIE。"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&AcquireRandom),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs ClearCommand(
		TEXT("Shoot.Skill.Clear"),
		TEXT("清空玩家四槽 Match Skill 并撤销每槽 AbilitySet。参数：[PlayerIndex=0]。仅 Authority PIE。"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Clear),
		ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs DumpCommand(
		TEXT("Shoot.Skill.Dump"),
		TEXT("打印玩家四槽 Match Skill。参数：[PlayerIndex=0]。"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Dump),
		ECVF_Cheat);
}
