#include "AbilitySystem/Abilities/ShootGA_Reload_ShotgunPerShell.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "BlueprintGameplayTagLibrary.h"
#include "Weapons/ShootRangedWeaponInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogGA_Reload_ShotgunPerShell, Log, All);

UShootGA_Reload_ShotgunPerShell::UShootGA_Reload_ShotgunPerShell()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	bServerRespectsRemoteAbilityCancellation = true;
	bRetriggerInstancedAbility = false;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ClientOrServer;
}

void UShootGA_Reload_ShotgunPerShell::PostInitProperties()
{
	Super::PostInitProperties();

	// GameplayAbility CDO 可能早于项目的 FShootGameplayTags 单例初始化；这里若直接读取单例，
	// CancelAbilitiesWithTag、NoFiring 和逐发事件 Tag 会被固化为空，导致开火既不能取消
	// 逐发换弹，空仓换弹也不会真正阻止开火。按稳定名字请求 Tag，确保 CDO 和实例都拿到值。
	TagAbilityWeaponNoFiring = FGameplayTag::RequestGameplayTag(
		FName("Ability.Weapon.NoFiring"), false);
	TagActivateFailMagazineFull = FGameplayTag::RequestGameplayTag(
		FName("Ability.ActivateFail.MagazineFull"), false);
	TagActivateFailNoSpareAmmo = FGameplayTag::RequestGameplayTag(
		FName("Ability.ActivateFail.NoSpareAmmo"), false);

	if (!InsertShellEventTag.IsValid())
	{
		InsertShellEventTag = FGameplayTag::RequestGameplayTag(
			FName("GameplayEvent.Reload.InsertShell"), false);
	}
	if (!ReloadDoneEventTag.IsValid())
	{
		ReloadDoneEventTag = FGameplayTag::RequestGameplayTag(
			FName("GameplayEvent.ReloadDone"), false);
	}

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FGameplayTagContainer AbilityAssetTags = GetAssetTags();
		AbilityAssetTags.AddTag(FGameplayTag::RequestGameplayTag(
			FName("Ability.Type.Action.Reload"), false));
		SetAssetTags(AbilityAssetTags);
		ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(
			FName("Event.Movement.Reload"), false));
		if (!StartupInputTag.IsValid())
		{
			StartupInputTag = FGameplayTag::RequestGameplayTag(FName("InputTag.R"), false);
		}
	}
}

bool UShootGA_Reload_ShotgunPerShell::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UShootRangedWeaponInstance* Weapon = GetWeaponInstance();
	if (!Weapon)
	{
		return false;
	}

	// 项目当前与 Lyra 一样读取 ItemInstance 上复制的 StatTags；GetPredicted* 只是旧兼容别名，
	// 这里不把它误认为本地弹药预测，避免把客户端显示值当成服务器权威状态。
	const int32 CurrentAmmo = Weapon->GetCurrentAmmo();
	const int32 CurrentReserve = Weapon->GetCurrentReserve();
	const bool bMagazineFull = CurrentAmmo >= Weapon->GetMagazineSize();
	const bool bNoReserve = CurrentReserve <= 0;
	if (!bMagazineFull && !bNoReserve)
	{
		return true;
	}

	if (OptionalRelevantTags)
	{
		if (bMagazineFull && TagActivateFailMagazineFull.IsValid())
		{
			OptionalRelevantTags->AddTag(TagActivateFailMagazineFull);
		}
		if (bNoReserve && TagActivateFailNoSpareAmmo.IsValid())
		{
			OptionalRelevantTags->AddTag(TagActivateFailNoSpareAmmo);
		}
	}
	return false;
}

void UShootGA_Reload_ShotgunPerShell::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedWeapon = GetWeaponInstance();
	ShellsInserted = 0;
	bEndSectionStarted = false;

	if (!GetAvatarActorFromActorInfo() || !CachedWeapon.IsValid())
	{
		UE_LOG(LogGA_Reload_ShotgunPerShell, Error, TEXT("Activate: missing character or weapon instance."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Live Coding 或较早创建的 Ability CDO 可能已经保留空事件 Tag；在真正创建等待任务前再解析一次，
	// 这样不会因为编辑器启动时序让动画 Notify 失去接收者。
	if (!InsertShellEventTag.IsValid())
	{
		InsertShellEventTag = FGameplayTag::RequestGameplayTag(
			FName("GameplayEvent.Reload.InsertShell"), false);
	}
	if (!ReloadDoneEventTag.IsValid())
	{
		ReloadDoneEventTag = FGameplayTag::RequestGameplayTag(
			FName("GameplayEvent.ReloadDone"), false);
	}

	InitialAmmo = CachedWeapon->GetCurrentAmmo();
	InitialReserve = CachedWeapon->GetCurrentReserve();
	SetFiringBlocked(InitialAmmo <= 0);

	UAnimMontage* MontageToPlay = CachedWeapon->GetCharacterReloadMontage();
	if (!MontageToPlay || !HasRequiredSections(MontageToPlay))
	{
		UE_LOG(LogGA_Reload_ShotgunPerShell, Error,
			TEXT("Activate: %s has no valid per-shell montage with sections %s/%s/%s."),
			*GetNameSafe(CachedWeapon.Get()), *SectionStartName.ToString(), *SectionLoopName.ToString(),
			*SectionEndName.ToString());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, MontageToPlay, MontagePlayRate, SectionStartName, true, 1.0f, 0.0f, false);
	MontageTask->OnCompleted.AddDynamic(this, &UShootGA_Reload_ShotgunPerShell::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &UShootGA_Reload_ShotgunPerShell::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UShootGA_Reload_ShotgunPerShell::OnMontageInterrupted);
	MontageTask->ReadyForActivation();

	InsertShellTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, InsertShellEventTag, nullptr, false, true);
	if (InsertShellTask)
	{
		InsertShellTask->EventReceived.AddDynamic(this, &UShootGA_Reload_ShotgunPerShell::OnInsertShellEvent);
		InsertShellTask->ReadyForActivation();
	}

	if (ReloadDoneEventTag.IsValid())
	{
		ReloadDoneTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this, ReloadDoneEventTag, nullptr, true, true);
		if (ReloadDoneTask)
		{
			ReloadDoneTask->EventReceived.AddDynamic(this, &UShootGA_Reload_ShotgunPerShell::OnReloadDoneEvent);
			ReloadDoneTask->ReadyForActivation();
		}
	}

	// Reload 是一次按下后持续逐发装填，不是按住式输入。等待 InputRelease 会让普通点击 R
	// 在首个 InsertShell 提交点之前直接跳到 End，表现为动画播放但弹药永远不增加。
	// 非空弹匣仍由 Fire GA 取消本能力；满仓、无备弹和显式 ReloadDone 进入收尾段。
	SetNextSection(SectionStartName, SectionLoopName);
}

void UShootGA_Reload_ShotgunPerShell::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	SetFiringBlocked(false);

	if (InsertShellTask)
	{
		InsertShellTask->EndTask();
		InsertShellTask = nullptr;
	}
	if (ReloadDoneTask)
	{
		ReloadDoneTask->EndTask();
		ReloadDoneTask = nullptr;
	}
	if (MontageTask)
	{
		MontageTask->EndTask();
		MontageTask = nullptr;
	}

	CachedWeapon.Reset();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UShootRangedWeaponInstance* UShootGA_Reload_ShotgunPerShell::GetWeaponInstance() const
{
	if (const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
	{
		return Cast<UShootRangedWeaponInstance>(Spec->SourceObject.Get());
	}
	return nullptr;
}

void UShootGA_Reload_ShotgunPerShell::OnInsertShellEvent(FGameplayEventData /*Payload*/)
{
	if (!CachedWeapon.IsValid() || !IsActive())
	{
		return;
	}

	const int32 PredictedAmmoBeforeInsert = FMath::Min(
		CachedWeapon->GetMagazineSize(), InitialAmmo + ShellsInserted);
	const int32 PredictedReserveBeforeInsert = FMath::Max(0, InitialReserve - ShellsInserted);
	if (PredictedAmmoBeforeInsert >= CachedWeapon->GetMagazineSize() || PredictedReserveBeforeInsert <= 0)
	{
		ContinueOrEndReload();
		return;
	}

	if (K2_HasAuthority())
	{
		const int32 AmmoBeforeInsert = CachedWeapon->GetCurrentAmmo();
		CachedWeapon->ReloadAmmo(1);
		if (CachedWeapon->GetCurrentAmmo() <= AmmoBeforeInsert)
		{
			ContinueOrEndReload();
			return;
		}
	}

	// 客户端只记录动画已经提交的发数，不直接修改 Inventory ItemInstance 的 StatTags。
	++ShellsInserted;

	// 空仓换弹只在第一发真正提交前禁止开火。服务器在 ReloadAmmo 成功后才到达这里；
	// 客户端则以同一个 InsertShell Notify 记录本地预测的提交点。这样 Fire GA 可以取消
	// 当前逐发换弹并立即使用已经装入的第一发，且不会影响弹匣式 Reload 或其它武器。
	const int32 EffectiveAmmoAfterInsert = K2_HasAuthority()
		? CachedWeapon->GetCurrentAmmo()
		: FMath::Min(CachedWeapon->GetMagazineSize(), InitialAmmo + ShellsInserted);
	if (EffectiveAmmoAfterInsert > 0)
	{
		SetFiringBlocked(false);
	}

	ContinueOrEndReload();
}

void UShootGA_Reload_ShotgunPerShell::OnReloadDoneEvent(FGameplayEventData /*Payload*/)
{
	// 外部结束事件表示玩家/动画主动结束换弹，仍必须经过护木收尾段；
	// 只有 Fire GA 的取消才允许跳过收尾并立即开火。
	RequestEndReload();
}

void UShootGA_Reload_ShotgunPerShell::OnMontageCompleted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UShootGA_Reload_ShotgunPerShell::OnMontageInterrupted()
{
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

void UShootGA_Reload_ShotgunPerShell::ContinueOrEndReload()
{
	if (!CachedWeapon.IsValid())
	{
		return;
	}

	const bool bAuthority = K2_HasAuthority();
	const int32 EffectiveAmmo = bAuthority
		? CachedWeapon->GetCurrentAmmo()
		: FMath::Min(CachedWeapon->GetMagazineSize(), InitialAmmo + ShellsInserted);
	const int32 EffectiveReserve = bAuthority
		? CachedWeapon->GetCurrentReserve()
		: FMath::Max(0, InitialReserve - ShellsInserted);
	const bool bFull = EffectiveAmmo >= CachedWeapon->GetMagazineSize();
	const bool bNoAmmo = EffectiveReserve <= 0;

	if (bFull || bNoAmmo)
	{
		RequestEndReload();
	}
	else
	{
		SetNextSection(SectionLoopName, SectionLoopName);
		JumpToSection(SectionLoopName);
	}
}

void UShootGA_Reload_ShotgunPerShell::RequestEndReload()
{
	if (!CachedWeapon.IsValid() || !IsActive() || bEndSectionStarted)
	{
		return;
	}

	bEndSectionStarted = true;
	// 同时设置 Start/Loop 的下一段，覆盖主动结束发生在任一动画段的情况；
	// JumpToSection 立即进入武器同步所跟随的角色 End 段，不能只结束 GA 留下半截护木动作。
	SetNextSection(SectionStartName, SectionEndName);
	SetNextSection(SectionLoopName, SectionEndName);
	JumpToSection(SectionEndName);
}

void UShootGA_Reload_ShotgunPerShell::SetFiringBlocked(const bool bShouldBlock)
{
	if (!TagAbilityWeaponNoFiring.IsValid())
	{
		TagAbilityWeaponNoFiring = FGameplayTag::RequestGameplayTag(
			FName("Ability.Weapon.NoFiring"), false);
	}

	if (bDidBlockFiring == bShouldBlock || !TagAbilityWeaponNoFiring.IsValid())
	{
		return;
	}

	AActor* OwningActor = GetOwningActorFromActorInfo();
	if (!OwningActor)
	{
		return;
	}

	const FGameplayTagContainer NoFiringTags =
		UBlueprintGameplayTagLibrary::MakeLiteralGameplayTagContainer(
			FGameplayTagContainer(TagAbilityWeaponNoFiring));
	if (bShouldBlock)
	{
		UAbilitySystemBlueprintLibrary::AddLooseGameplayTags(OwningActor, NoFiringTags, false);
	}
	else
	{
		UAbilitySystemBlueprintLibrary::RemoveLooseGameplayTags(OwningActor, NoFiringTags, false);
	}
	bDidBlockFiring = bShouldBlock;
}

bool UShootGA_Reload_ShotgunPerShell::HasRequiredSections(const UAnimMontage* Montage) const
{
	return Montage
		&& !SectionStartName.IsNone() && Montage->GetSectionIndex(SectionStartName) != INDEX_NONE
		&& !SectionLoopName.IsNone() && Montage->GetSectionIndex(SectionLoopName) != INDEX_NONE
		&& !SectionEndName.IsNone() && Montage->GetSectionIndex(SectionEndName) != INDEX_NONE;
}

void UShootGA_Reload_ShotgunPerShell::JumpToSection(FName Section)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (!Section.IsNone() && ASC->IsAnimatingAbility(this))
		{
			ASC->CurrentMontageJumpToSection(Section);
		}
	}
}

void UShootGA_Reload_ShotgunPerShell::SetNextSection(FName From, FName To)
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		if (!From.IsNone() && !To.IsNone() && ASC->IsAnimatingAbility(this))
		{
			ASC->CurrentMontageSetNextSectionName(From, To);
		}
	}
}
