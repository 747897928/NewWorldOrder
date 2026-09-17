// Copyright ZhaoYiJie


#include "Character/MutableAppearanceComponent.h"

#include "Character/AppearanceTagInfo.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MuCO/CustomizableObject.h"
#include "MuCO/CustomizableSkeletalComponent.h"
#include "MuCO/CustomizableObjectInstanceUsage.h" // UE 5.8+: UpdatedDelegate 从 CustomizableSkeletalComponent 移到了这里
#include "Player/ShootPlayerState.h"
#include "ShootLogChannels.h"
#include "TimerManager.h"

// Sets default values for this component's properties
UMutableAppearanceComponent::UMutableAppearanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Mutable 对象与正式角色 AnimBP 由 DefaultGame.ini 的 /Script/NewWorldOrder.MutableAppearanceComponent 提供。
	// 衣柜预览会在 InitializePreviewComponents 前注入自己的基础 AnimBP，构造函数不持有任何内容资产路径。
}

void UMutableAppearanceComponent::BeginDestroy()
{
	ClearMutableReadinessCheck();

	if (HeadCSkeletalComponent)
	{
		// UE 5.8+: UpdatedDelegate 已从 CustomizableSkeletalComponent 移到了 CustomizableObjectInstanceUsage
		// 通过 GetInstanceUsage() 获取中间层 UObject，解除委托绑定避免悬垂指针
		if (UCustomizableObjectInstanceUsage* Usage = HeadCSkeletalComponent->GetInstanceUsage())
		{
			Usage->UpdatedDelegate.Unbind();
		}
	}
	// 解绑委托
	if (ShootPlayerState)
	{
		ShootPlayerState->OnAppearanceTagsChanged.RemoveAll(this);
	}

	Super::BeginDestroy();
}

void UMutableAppearanceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearMutableReadinessCheck();
	Super::EndPlay(EndPlayReason);
}

void UMutableAppearanceComponent::InitializeComponents(UCustomizableSkeletalComponent* InHeadComponent,
                                                       UCustomizableSkeletalComponent* InBodyComponent,
                                                       USkeletalMeshComponent* InHeadMesh,
                                                       USkeletalMeshComponent* InBodyMesh,
                                                       AShootPlayerState* InShootPlayerState)
{
	// 重新初始化时先清理旧委托，避免 PossessedBy/OnRep_PlayerState 重入导致重复回调。
	if (ShootPlayerState)
	{
		ShootPlayerState->OnAppearanceTagsChanged.RemoveAll(this);
	}

	ShootPlayerState = InShootPlayerState;
	bPreviewInitialization = false;
	HeadCSkeletalComponent = InHeadComponent;
	BodyCSkeletalComponent = InBodyComponent;
	HeadMesh = InHeadMesh;
	BodyMesh = InBodyMesh;
	//Select the Mesh Component or Passthrough Mesh Component declared in the Customizable Object graph
	BodyCSkeletalComponent->SetComponentName(BodyComponentName);
	HeadCSkeletalComponent->SetComponentName(HeadComponentName);
	FemaleAnimClassPtr = FemaleAnimClass.LoadSynchronous();
	MaleAnimClassPtr = MaleAnimClass.LoadSynchronous();
	check(HeadCSkeletalComponent);
	// UE 5.8+: 通过 GetInstanceUsage() 绑定更新委托，替代旧版 CSkeletalComponent->UpdatedDelegate。
	// CustomizableObjectInstanceUsage 是 UE 5.8 新增的中间层 UObject，负责管理 COI 与 SkeletalMesh 的绑定生命周期。
	//
	// 只绑定 HeadCSkeletalComponent 的 UpdatedDelegate，不绑定 BodyCSkeletalComponent，原因：
	// - 组件层级：HeadCSkeletalComponent 挂载在 GetMesh() 上，BodyCSkeletalComponent 挂载在 BodyMesh 上，
	//   BodyMesh 是 GetMesh() 的子组件，通过 SetLeaderPoseComponent 跟随 GetMesh() 的姿态。
	//   详见 AShootCharacter 构造函数：HeadCSkeletalComponent->SetupAttachment(GetMesh()) 在第 110 行，
	//   BodyMesh->SetupAttachment(GetMesh()) 在第 93 行，BodyCSkeletalComponent->SetupAttachment(BodyMesh) 在第 103 行。
	// - Head 和 Body 两个 CSC 共享同一个 COI 实例（SwitchGender 中对两者都调了 SetCustomizableObjectInstance）。
	//   Mutable 在 UpdateSkeletalMeshAsync 完成后会遍历 COI 注册的所有 InstanceUsage，
	//   各自设置对应 ComponentName 的网格，这个网格设置流程与 UpdatedDelegate 绑定无关。
	// - UpdatedDelegate 仅用于触发 OnCustomizableSkeletalUpdated 回调（AnimBP 链接、GameplayTag 处理），
	//   这些逻辑只需执行一次，在 Head 的 InstanceUsage 上监听就够了。
	BindHeadUpdatedDelegate();
	// 先绑定 PlayerState 标签广播，再触发同步，避免初始化瞬间丢失一次关键更新。
	check(ShootPlayerState);
	ShootPlayerState->OnAppearanceTagsChanged.RemoveAll(this);
	ShootPlayerState->OnAppearanceTagsChanged.AddUObject(this, &ThisClass::OnPlayerStateAppearanceTagsChanged);

	// 编辑器冷启动时 CO 的 UObject 已加载，但编译模型可能仍在磁盘/DDC 异步恢复。
	// 统一从就绪入口继续，禁止在 IsCompiled=false 时写参数后留下永久默认外观。
	TryCompleteMutableInitialization();
}

void UMutableAppearanceComponent::InitializePreviewComponents(
	UCustomizableSkeletalComponent* InHeadComponent,
	UCustomizableSkeletalComponent* InBodyComponent,
	USkeletalMeshComponent* InHeadMesh,
	USkeletalMeshComponent* InBodyMesh,
	ECharacterGender InGender,
	const FGameplayTagContainer& InAppearanceTags)
{
	// 衣柜 UI 预览 Actor 没有真实 PlayerState，不能走正式角色的委托写回链路。
	// 这里只初始化 Mutable 组件和动画类，再按传入标签更新预览网格。
	ShootPlayerState = nullptr;
	bPreviewInitialization = true;
	HeadCSkeletalComponent = InHeadComponent;
	BodyCSkeletalComponent = InBodyComponent;
	HeadMesh = InHeadMesh;
	BodyMesh = InBodyMesh;
	PreviewGender = InGender;
	PendingPreviewAppearanceTags = InAppearanceTags;

	if (!HeadCSkeletalComponent || !BodyCSkeletalComponent || !HeadMesh || !BodyMesh)
	{
		return;
	}

	BodyCSkeletalComponent->SetComponentName(BodyComponentName);
	HeadCSkeletalComponent->SetComponentName(HeadComponentName);
	// 衣柜预览 Actor 会在进入这里之前注入预览专用基础 AnimBP。
	// 仅在外部没有注入时加载正式角色默认类，避免初始化把预览类覆盖回去。
	if (!FemaleAnimClassPtr)
	{
		FemaleAnimClassPtr = FemaleAnimClass.LoadSynchronous();
	}
	if (!MaleAnimClassPtr)
	{
		MaleAnimClassPtr = MaleAnimClass.LoadSynchronous();
	}

	// 预览 Actor 也必须监听 Mutable 的异步更新完成点。换装时网格会在 ApplyPreviewAppearanceTags 返回后才真正替换，
	// 因此 LeaderPose 和 AnimLayer 不能只在 ApplyAppearance 调用栈里设置。
	BindHeadUpdatedDelegate();
	TryCompleteMutableInitialization();
}

void UMutableAppearanceComponent::SetPreviewBaseAnimClasses(
	TSubclassOf<UAnimInstance> InFemaleAnimClass,
	TSubclassOf<UAnimInstance> InMaleAnimClass)
{
	FemaleAnimClassPtr = InFemaleAnimClass;
	MaleAnimClassPtr = InMaleAnimClass;
}

void UMutableAppearanceComponent::ApplyPreviewAppearanceTags(ECharacterGender InGender,
                                                            const FGameplayTagContainer& InAppearanceTags)
{
	if (!HeadCSkeletalComponent || !BodyCSkeletalComponent)
	{
		return;
	}

	PreviewGender = InGender;
	PendingPreviewAppearanceTags = InAppearanceTags;
	if (!bMutableInitializationComplete)
	{
		TryCompleteMutableInitialization();
		return;
	}

	// 预览换装会频繁进入这里，但它和正式角色一样只是在同一 COI 上改参数。
	// SwitchGenderInstance 内部必须守住生命周期边界：同性别换衣服不能重建基础 AnimInstance。
	SwitchGenderInstance(InGender);
	ApplyAppearanceTagsToCurrentInstance(InAppearanceTags, true);
}

void UMutableAppearanceComponent::BindHeadUpdatedDelegate()
{
	if (!HeadCSkeletalComponent)
	{
		return;
	}

	if (UCustomizableObjectInstanceUsage* Usage = HeadCSkeletalComponent->GetInstanceUsage())
	{
		// 同一个组件可能在正式角色初始化、预览初始化或角色切换时重复进入，先解除旧绑定再绑定当前对象。
		Usage->UpdatedDelegate.Unbind();
		Usage->UpdatedDelegate.BindUObject(this, &ThisClass::OnCustomizableSkeletalUpdated);
	}
}

bool UMutableAppearanceComponent::TryInitCustomizableObjectInstances()
{
	if (!FemaleCustomizableObject && FemaleObjectPath.IsValid())
	{
		FemaleCustomizableObject = Cast<UCustomizableObject>(FemaleObjectPath.TryLoad());
		if (!FemaleCustomizableObject)
		{
			UE_LOG(LogShoot, Error, TEXT("Failed to load female object: %s"),
			       *FemaleObjectPath.ToString());
			bMutableObjectLoadFailed = true;
		}
	}

	if (!MaleCustomizableObject && MaleObjectPath.IsValid())
	{
		MaleCustomizableObject = Cast<UCustomizableObject>(MaleObjectPath.TryLoad());
		if (!MaleCustomizableObject)
		{
			UE_LOG(LogShoot, Error, TEXT("Failed to load male object: %s"),
			       *MaleObjectPath.ToString());
			bMutableObjectLoadFailed = true;
		}
	}

	if (!FemaleCustomizableObject || !MaleCustomizableObject)
	{
		return false;
	}

#if WITH_EDITOR
	// IsLoading=false 且仍无编译模型时，按 Mutable 官方入口请求自动编译。
	// 冷启动正常命中磁盘/DDC 时不会进入编译；ConditionalAutoCompile 自身也会拒绝重复请求。
	if (!FemaleCustomizableObject->IsLoading() && !FemaleCustomizableObject->IsCompiled())
	{
		FemaleCustomizableObject->ConditionalAutoCompile();
	}
	if (!MaleCustomizableObject->IsLoading() && !MaleCustomizableObject->IsCompiled())
	{
		MaleCustomizableObject->ConditionalAutoCompile();
	}
#endif

	if (!FemaleCustomizableObject->IsCompiled() || !MaleCustomizableObject->IsCompiled())
	{
		return false;
	}

	if (!FemaleInstance)
	{
		FemaleInstance = NewObject<UCustomizableObjectInstance>(this);
		FemaleInstance->SetObject(FemaleCustomizableObject);
	}
	if (!MaleInstance)
	{
		MaleInstance = NewObject<UCustomizableObjectInstance>(this);
		MaleInstance->SetObject(MaleCustomizableObject);
	}

	return true;
}

void UMutableAppearanceComponent::TryCompleteMutableInitialization()
{
	if (bMutableInitializationComplete || bMutableObjectLoadFailed)
	{
		return;
	}

	if (!TryInitCustomizableObjectInstances())
	{
		ScheduleMutableReadinessCheck();
		return;
	}

	bMutableInitializationComplete = true;
	ClearMutableReadinessCheck();
	BindHeadUpdatedDelegate();

	if (ShootPlayerState)
	{
		const ECharacterGender CurrentGender = ShootPlayerState->GetCharacterGender();
		SwitchGenderInstance(CurrentGender);
		OnPlayerStateAppearanceTagsChanged(CurrentGender, ShootPlayerState->GetAppearanceTags(CurrentGender));
	}
	else if (bPreviewInitialization)
	{
		SwitchGenderInstance(PreviewGender);
		ApplyAppearanceTagsToCurrentInstance(PendingPreviewAppearanceTags, true);
	}
}

void UMutableAppearanceComponent::ScheduleMutableReadinessCheck()
{
	if (bMutableObjectLoadFailed || !GetWorld())
	{
		return;
	}

	FTimerManager& TimerManager = GetWorld()->GetTimerManager();
	if (!TimerManager.IsTimerActive(MutableReadinessTimer))
	{
		// 这里的 0.05 秒只是检查频率，不是“等固定时间后假定成功”。
		// 真正放行条件始终是 UCustomizableObject::IsCompiled()。
		TimerManager.SetTimer(
			MutableReadinessTimer,
			this,
			&ThisClass::TryCompleteMutableInitialization,
			0.05f,
			true);
	}
}

void UMutableAppearanceComponent::ClearMutableReadinessCheck()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(MutableReadinessTimer);
	}
}

void UMutableAppearanceComponent::OnCustomizableSkeletalUpdated()
{
	if (!HeadCSkeletalComponent) return;

	UCustomizableObjectInstance* ObjectInstance = HeadCSkeletalComponent->GetCustomizableObjectInstance();
	if (!ObjectInstance) return;
	// PlayerState -> Mutable 同步触发的更新，不应再反向写回 PlayerState，避免覆盖存档标签。
	const bool bSkipWritebackToPlayerState = bApplyingAppearanceTagsFromPlayerState;
	bApplyingAppearanceTagsFromPlayerState = false;

	const ECharacterGender CharacterGender = ShootPlayerState ? ShootPlayerState->GetCharacterGender() : PreviewGender;
	SyncAnimationLayerTags(CharacterGender, ObjectInstance);
	// Mutable 已在触发 Usage UpdatedDelegate 前替换同一 COI 的所有 Head/Body 网格。
	// 此处才可以针对最终骨架安全建立 Body -> Head 的动画跟随，不能放在角色构造或请求更新之前。
	TryBindLeaderPoseAfterMutableUpdate();
	OnMutableSkeletalMeshUpdated.Broadcast();

	if (!ShootPlayerState)
	{
		// 衣柜预览没有 PlayerState，更新到这里已经完成 AnimLayer 与 LeaderPose 通知，不能继续写存档。
		return;
	}

	// 获取标签容器引用
	const FGameplayTagContainer& AnimationGameplayTags = ObjectInstance->GetAnimationGameplayTags();
	// 更新PlayerState的标签
	if (GetOwner() && GetOwner()->HasAuthority() && !bSkipWritebackToPlayerState)
	{
		UE_LOG(LogShoot, Verbose,
		       TEXT("Mutable Writeback PlayerState Tags: Gender=%d TagsNum=%d"),
		       static_cast<int32>(CharacterGender), AnimationGameplayTags.Num());
		ShootPlayerState->ServerSetAppearanceTags(CharacterGender, AnimationGameplayTags);
	}
}

void UMutableAppearanceComponent::SyncAnimationLayerTags(ECharacterGender InGender,
                                                        UCustomizableObjectInstance* ObjectInstance)
{
	if (!ObjectInstance || InGender == ECharacterGender::UNKNOWN)
	{
		return;
	}

	const FGameplayTagContainer& AnimationGameplayTags = ObjectInstance->GetAnimationGameplayTags();
	const FGameplayTagContainer& LastAnimationTags = GetLastAnimationTags(InGender);
	// 计算标签变化 (安全方法)
	FGameplayTagContainer AddedTags = AnimationGameplayTags;
	AddedTags.RemoveTags(LastAnimationTags);

	FGameplayTagContainer RemovedTags = LastAnimationTags;
	RemovedTags.RemoveTags(AnimationGameplayTags);

	UE_LOG(LogShoot, Verbose,
	       TEXT("Mutable SyncAnimationLayerTags: Gender=%d AnimTagsNum=%d LastTagsNum=%d Added=%d Removed=%d"),
	       static_cast<int32>(InGender),
	       AnimationGameplayTags.Num(), LastAnimationTags.Num(), AddedTags.Num(), RemovedTags.Num());

	SetLastAnimationTags(InGender, AnimationGameplayTags);

	TMap<FGameplayTag, TSubclassOf<UAnimInstance>>& GameplayTagAnimInstanceMap =
		GetMutableGameplayTagAnimInstanceMap(InGender);
	// 处理移除标签
	for (const FGameplayTag& Tag : RemovedTags)
	{
		//如果未应用其slot的网格，ObjectInstance->GetAnimBP会返回空指针，只有应用了其slot的网格才能get到AnimBP，
		//所以我们只能从之前保存的FGameplayTag和TSubclassOf<UAnimInstance>的映射找对应的AnimBP去UnlinkAnimClassLayers
		if (const TSubclassOf<UAnimInstance>* AnimBPPtr = GameplayTagAnimInstanceMap.Find(Tag))
		{
			const TSubclassOf<UAnimInstance> AnimBP = *AnimBPPtr;
			if (UKismetSystemLibrary::IsValidClass(AnimBP))
			{
				//移除掉之前旧的link class layers
				if (HeadMesh)
				{
					HeadMesh->UnlinkAnimClassLayers(AnimBP);
				}
			}
			// 无论旧类当前是否仍有效，都移除缓存；否则失效资产会永久残留并干扰后续同标签重连。
			GameplayTagAnimInstanceMap.Remove(Tag);
		}
	}

	// 处理新增标签
	for (const FGameplayTag& Tag : AddedTags)
	{
		FAppearanceTagInfo Info = ParseAppearanceTag(Tag);
		if (Info.IsFullOption())
		{
			FName ComponentName = FName(Info.ComponentName);
			FName SelectedOptionName = FName(Info.SelectedOptionName);
			//如果未应用其slot的网格，ObjectInstance->GetAnimBP会返回空指针，只有应用了其slot的网格才能get到AnimBP
			TSubclassOf<UAnimInstance> AnimBP = ObjectInstance->GetAnimBP(ComponentName, SelectedOptionName);
			if (UKismetSystemLibrary::IsValidClass(AnimBP))
			{
				//link class layers
				if (HeadMesh)
				{
					HeadMesh->LinkAnimClassLayers(AnimBP);
					GameplayTagAnimInstanceMap.Add(Tag, AnimBP);
				}
			}
			else
			{
				// 走到这里说明 Mutable 已经返回了动画 GameplayTag，但对应选项没有取到 AnimBP。
				// 这通常是 CO 配置问题，或网格槽还没有真正生成；日志保留 Tag 方便排查鞋子、披风等通用动画层。
				UE_LOG(LogShoot, Warning,
				       TEXT("Mutable AnimLayer tag has no AnimBP: Tag=%s Component=%s Option=%s"),
				       *Tag.ToString(), *Info.ComponentName, *Info.SelectedOptionName);
			}
		}
	}

}

FAppearanceTagInfo UMutableAppearanceComponent::ParseAppearanceTag(const FGameplayTag& Tag) const
{
	FAppearanceTagInfo Info;
	TArray<FString> Parts;
	Tag.ToString().ParseIntoArray(Parts, TEXT("."), true);

	if (Parts.Num() >= 1) Info.Gender = Parts[0];
	if (Parts.Num() >= 2) Info.ComponentName = Parts[1];
	if (Parts.Num() >= 3) Info.ParamName = Parts[2];
	if (Parts.Num() >= 4) Info.SelectedOptionName = Parts[3];

	return Info;
}

void UMutableAppearanceComponent::OnPlayerStateAppearanceTagsChanged(ECharacterGender InGender,
                                                                     const FGameplayTagContainer& NewAppearanceTags)
{
	if (!ShootPlayerState) return;
	if (!HeadCSkeletalComponent) return;
	if (!bMutableInitializationComplete)
	{
		TryCompleteMutableInitialization();
		return;
	}

	// 只响应“当前激活主角”的标签广播，避免在恢复另一性别标签时污染当前实例。
	const ECharacterGender CurrentGender = ShootPlayerState->GetCharacterGender();
	if (InGender != CurrentGender)
	{
		UE_LOG(LogShoot, Verbose,
		       TEXT("Mutable OnPlayerStateAppearanceTagsChanged ignored: InGender=%d CurrentGender=%d NewTagsNum=%d"),
		       static_cast<int32>(InGender), static_cast<int32>(CurrentGender), NewAppearanceTags.Num());
		return;
	}

	UCustomizableObjectInstance* ObjectInstance = HeadCSkeletalComponent->GetCustomizableObjectInstance();
	bool bSwitchedGenderInstance = false;

	// 先确保当前 CustomizableObjectInstance 与目标性别一致，再应用参数标签。
	const bool bNeedMaleInstance = (InGender == ECharacterGender::MALE);
	const bool bNeedFemaleInstance = (InGender == ECharacterGender::FEMALE);
	if ((bNeedMaleInstance && ObjectInstance != MaleInstance)
		|| (bNeedFemaleInstance && ObjectInstance != FemaleInstance))
	{
		SwitchGender(InGender);
		bSwitchedGenderInstance = true;
		ObjectInstance = HeadCSkeletalComponent->GetCustomizableObjectInstance();
	}
	if (!ObjectInstance) return;

	UE_LOG(LogShoot, Verbose,
	       TEXT("Mutable OnPlayerStateAppearanceTagsChanged: InGender=%d CurrentGender=%d Switched=%d NewTagsNum=%d CurrentAnimTagsNum=%d"),
	       static_cast<int32>(InGender), static_cast<int32>(CurrentGender), bSwitchedGenderInstance ? 1 : 0,
	       NewAppearanceTags.Num(), ObjectInstance->GetAnimationGameplayTags().Num());

	ApplyAppearanceTagsToCurrentInstance(NewAppearanceTags, bSwitchedGenderInstance);
}

const FGameplayTagContainer& UMutableAppearanceComponent::GetLastAnimationTags(ECharacterGender InGender) const
{
	switch (InGender)
	{
	case ECharacterGender::MALE:
		return MaleLastAnimationTags;
	case ECharacterGender::FEMALE:
		return FemaleLastAnimationTags;
	default:
		return FemaleLastAnimationTags;
	}
}

void UMutableAppearanceComponent::SetLastAnimationTags(ECharacterGender InGender,
	                                                   const FGameplayTagContainer& InLastAnimationTags)
{
	switch (InGender)
	{
	case ECharacterGender::MALE:
		MaleLastAnimationTags = InLastAnimationTags;
		break;
	case ECharacterGender::FEMALE:
		FemaleLastAnimationTags = InLastAnimationTags;
		break;
	default:
		break;
	}
}

TMap<FGameplayTag, TSubclassOf<UAnimInstance>>& UMutableAppearanceComponent::GetMutableGameplayTagAnimInstanceMap(
	ECharacterGender InGender)
{
	switch (InGender)
	{
	case ECharacterGender::MALE:
		return MaleGameplayTagAnimInstanceMap;
	case ECharacterGender::FEMALE:
		return FemaleGameplayTagAnimInstanceMap;
	default:
		return FemaleGameplayTagAnimInstanceMap;
	}
}

void UMutableAppearanceComponent::ServerSetSelectedOptionAndUpdateMesh_Implementation(const FString& ParamName,
	const FString& SelectedOptionName)
{
	if (HeadCSkeletalComponent)
	{
		UCustomizableObjectInstance* ObjectInstance = HeadCSkeletalComponent->GetCustomizableObjectInstance();
		if (ObjectInstance)
		{
			ObjectInstance->SetIntParameterSelectedOption(ParamName, SelectedOptionName);
			ObjectInstance->UpdateSkeletalMeshAsync();
		}
	}
}

void UMutableAppearanceComponent::SwitchGender(ECharacterGender NewGender)
{
	if (!ShootPlayerState) return;
	if (!bMutableInitializationComplete)
	{
		TryCompleteMutableInitialization();
		return;
	}
	SwitchGenderInstance(NewGender);
	// 外观组件只负责“表现层切换”，角色身份状态统一由 PlayerState 切换流程维护。
}

void UMutableAppearanceComponent::SwitchGenderInstance(ECharacterGender NewGender)
{
	if (!HeadCSkeletalComponent || !BodyCSkeletalComponent || !HeadMesh)
	{
		return;
	}

	UCustomizableObjectInstance* TargetInstance = nullptr;
	TSubclassOf<UAnimInstance> TargetAnimClass;
	if (NewGender == ECharacterGender::MALE)
	{
		TargetInstance = MaleInstance;
		TargetAnimClass = MaleAnimClassPtr;
	}
	else if (NewGender == ECharacterGender::FEMALE)
	{
		TargetInstance = FemaleInstance;
		TargetAnimClass = FemaleAnimClassPtr;
	}

	if (!TargetInstance)
	{
		return;
	}

	// 换衣服只需要改 Mutable 参数。只有真正切换男女 COI 时才替换 CustomizableObjectInstance，
	// 避免同一角色每次装备服装都重走组件实例绑定生命周期。
	const bool bNeedsInstanceSwitch =
		BodyCSkeletalComponent->GetCustomizableObjectInstance() != TargetInstance ||
		HeadCSkeletalComponent->GetCustomizableObjectInstance() != TargetInstance;
	if (bNeedsInstanceSwitch)
	{
		// 只有男女 COI 真正切换时，Mutable 才会把网格暂时还原为另一套参考网格。
		// 普通换衣的 SetSkeletalMesh 会由引擎自动重建 LeaderBoneMap，不能提前解绑，否则异步空窗会闪出头身不同步。
		PrepareBodyForMutableInstanceSwitch();

		if (BodyCSkeletalComponent->GetCustomizableObjectInstance() != TargetInstance)
		{
			BodyCSkeletalComponent->SetCustomizableObjectInstance(TargetInstance);
		}
		if (HeadCSkeletalComponent->GetCustomizableObjectInstance() != TargetInstance)
		{
			HeadCSkeletalComponent->SetCustomizableObjectInstance(TargetInstance);
		}
	}

	if (UKismetSystemLibrary::IsValidClass(TargetAnimClass) && HeadMesh->GetAnimClass() != TargetAnimClass)
	{
		// 只有男女基础 AnimBP 真的变化时才调用 SetAnimInstanceClass。
		// UE 会在 NewClass != AnimClass 时 ClearAnimScriptInstance / InitAnim，重新初始化该组件内部的 AnimScriptInstance。
		// 同性别换衣服绝不能走这里，否则 Mutable 返回的鞋子、头发、披风等 AnimLayer 会被打断。
		HeadMesh->SetAnimInstanceClass(TargetAnimClass);
		ResetAnimationLayerCache(NewGender);
	}
}

void UMutableAppearanceComponent::ResetAnimationLayerCache(ECharacterGender InGender)
{
	if (InGender == ECharacterGender::UNKNOWN)
	{
		return;
	}

	SetLastAnimationTags(InGender, FGameplayTagContainer());
	GetMutableGameplayTagAnimInstanceMap(InGender).Reset();
}

void UMutableAppearanceComponent::ApplyAppearanceTagsToCurrentInstance(const FGameplayTagContainer& NewAppearanceTags,
                                                                      bool bForceUpdate)
{
	if (!HeadCSkeletalComponent)
	{
		return;
	}

	UCustomizableObjectInstance* ObjectInstance = HeadCSkeletalComponent->GetCustomizableObjectInstance();
	if (!ObjectInstance)
	{
		return;
	}

	const FGameplayTagContainer& AnimationGameplayTags = ObjectInstance->GetAnimationGameplayTags();

	FGameplayTagContainer AddedTags = NewAppearanceTags;
	AddedTags.RemoveTags(AnimationGameplayTags);

	FGameplayTagContainer RemovedTags = AnimationGameplayTags;
	RemovedTags.RemoveTags(NewAppearanceTags);

	if (AddedTags.Num() == 0 && RemovedTags.Num() == 0)
	{
		if (bForceUpdate)
		{
			bApplyingAppearanceTagsFromPlayerState = ShootPlayerState != nullptr;
			ObjectInstance->UpdateSkeletalMeshAsync();
		}
		return;
	}

	for (const FGameplayTag& Tag : RemovedTags)
	{
		FAppearanceTagInfo Info = ParseAppearanceTag(Tag);
		if (Info.IsFullOption())
		{
			const FString& ParamName = Info.ParamName;
			const FString SelectedOptionName = TEXT("None");
			ObjectInstance->SetIntParameterSelectedOption(ParamName, SelectedOptionName);
		}
	}

	for (const FGameplayTag& Tag : AddedTags)
	{
		FAppearanceTagInfo Info = ParseAppearanceTag(Tag);
		if (Info.IsFullOption())
		{
			const FString& ParamName = Info.ParamName;
			const FString& SelectedOptionName = Info.SelectedOptionName;
			ObjectInstance->SetIntParameterSelectedOption(ParamName, SelectedOptionName);
		}
	}

	// 正式角色的更新来自 PlayerState，需要在 UpdatedDelegate 中阻止反向写回。
	// 预览 Actor 没有 PlayerState，更新只刷新 UI 里的 Mutable 网格。
	bApplyingAppearanceTagsFromPlayerState = ShootPlayerState != nullptr;
	ObjectInstance->UpdateSkeletalMeshAsync();
}

void UMutableAppearanceComponent::PrepareBodyForMutableInstanceSwitch()
{
	if (BodyMesh)
	{
		// 初次生成和男女 COI 切换会短暂恢复 Mutable 的完整参考网格。先隐藏 Body，避免 Head 与 Body
		// 同时渲染同一个完整人物；普通换衣不进入这里，仍保留 LeaderPose 并由 UE 原子重建映射。
		BodyMesh->SetVisibility(false, true);
		bBodyWaitingForMutableUpdate = true;
		BodyMesh->SetLeaderPoseComponent(nullptr, false, false);
	}
}

void UMutableAppearanceComponent::TryBindLeaderPoseAfterMutableUpdate()
{
	if (!HeadMesh || !BodyMesh)
	{
		return;
	}

	USkeletalMesh* HeadSkeletalMesh = HeadMesh->GetSkeletalMeshAsset();
	USkeletalMesh* BodySkeletalMesh = BodyMesh->GetSkeletalMeshAsset();
	if (!HeadSkeletalMesh || !BodySkeletalMesh)
	{
		return;
	}

	const int32 HeadBoneCount = HeadSkeletalMesh->GetRefSkeleton().GetNum();
	const int32 BodyBoneCount = BodySkeletalMesh->GetRefSkeleton().GetNum();
	// Head 允许包含额外的面部骨骼，不能要求双方 Skeleton 指针或骨骼总数完全相同。
	// 引擎渲染代理真正要求的是 LeaderBoneMap 覆盖 Follower（Body）的全部参考骨骼；因此设置后立即核验，
	// 避免异步生成的 Body 网格带着不完整映射进入渲染线程并触发 Ensure。
	BodyMesh->SetLeaderPoseComponent(HeadMesh, true, false);
	const int32 LeaderBoneMapCount = BodyMesh->GetLeaderBoneMap().Num();
	if (LeaderBoneMapCount != BodyBoneCount)
	{
		BodyMesh->SetLeaderPoseComponent(nullptr, false, false);

		if (!bHasLoggedLeaderPoseMismatch)
		{
			UE_LOG(LogShoot, Error,
				TEXT("Mutable LeaderPose 未绑定：LeaderBoneMap 不完整。HeadMesh=%s BodyMesh=%s HeadSkeleton=%s BodySkeleton=%s HeadBones=%d BodyBones=%d LeaderBoneMap=%d"),
				*GetNameSafe(HeadSkeletalMesh),
				*GetNameSafe(BodySkeletalMesh),
				*GetNameSafe(HeadSkeletalMesh->GetSkeleton()),
				*GetNameSafe(BodySkeletalMesh->GetSkeleton()),
				HeadBoneCount,
				BodyBoneCount,
				LeaderBoneMapCount);
			bHasLoggedLeaderPoseMismatch = true;
		}
		return;
	}

	// 只有最终 Head/Body 网格已生成且 LeaderBoneMap 完整时才显示 Body。
	// 这同时覆盖正式角色和衣柜预览 Actor，不会让异步生成窗口暴露重复的完整兜底网格。
	if (bBodyWaitingForMutableUpdate)
	{
		BodyMesh->SetVisibility(true, true);
		bBodyWaitingForMutableUpdate = false;
	}

	bHasLoggedLeaderPoseMismatch = false;
}
