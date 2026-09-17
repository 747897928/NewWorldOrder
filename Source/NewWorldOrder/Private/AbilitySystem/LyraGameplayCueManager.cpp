
#include "AbilitySystem/LyraGameplayCueManager.h"

#include "Engine/AssetManager.h"
#include "ShootLogChannels.h"
#include "GameplayCueSet.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagsManager.h"
#include "UObject/UObjectThreadContext.h"
#include "Async/Async.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(LyraGameplayCueManager)

//////////////////////////////////////////////////////////////////////

// 编辑器下的三种加载模式（Lyra 通过命令行/控制台变量切换）
enum class ELyraEditorLoadMode
{
	// Editor 模式/PIE：启动时把所有 cue 预加载 -> 更慢的启动但 PIE 期间不会丢失 FX
	LoadUpfront,

	// 在非编辑器下随 tag 注册异步加载；在编辑器下延迟到 cue 被真正触发时异步加载（适合快速迭代）
	PreloadAsCuesAreReferenced_GameOnly,

	// 在任何情况下：当 tag 被引用时就异步加载（最懒的预加载）
	PreloadAsCuesAreReferenced
};

namespace LyraGameplayCueManagerCvars
{
	// 控制台命令：打印当前 cue（调试用）
	static FAutoConsoleCommand CVarDumpGameplayCues(
		TEXT("Lyra.DumpGameplayCues"),
		TEXT("Shows all assets that were loaded via LyraGameplayCueManager and are currently in memory."),
		FConsoleCommandWithArgsDelegate::CreateStatic(ULyraGameplayCueManager::DumpGameplayCues));

	// 默认编辑器加载模式（可通过控制台命令或 config 改）
	static ELyraEditorLoadMode LoadMode = ELyraEditorLoadMode::LoadUpfront;
}

const bool bPreloadEvenInEditor = true; // Lyra 里在 editor 下也可能控制性预加载（这里硬编码 true）

//////////////////////////////////////////////////////////////////////

// 这个小结构把一个 lambda 封装为可在 GameThread 执行的异步图任务
struct FGameplayCueTagThreadSynchronizeGraphTask : public FAsyncGraphTaskBase
{
	TFunction<void()> TheTask;
	FGameplayCueTagThreadSynchronizeGraphTask(TFunction<void()>&& Task) : TheTask(MoveTemp(Task)) { }
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) { TheTask(); }
	ENamedThreads::Type GetDesiredThread() { return ENamedThreads::GameThread; }
};

//////////////////////////////////////////////////////////////////////

ULyraGameplayCueManager::ULyraGameplayCueManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 构造函数通常不做太多逻辑，主要是继承链初始化
}

ULyraGameplayCueManager* ULyraGameplayCueManager::Get()
{
	// AbilitySystemGlobals 中保存了全局的 GameplayCueManager 实例
	// 这里做一个向下转换，方便其他模块直接用 ULyraGameplayCueManager::Get() 调用
	return Cast<ULyraGameplayCueManager>(UAbilitySystemGlobals::Get().GetGameplayCueManager());
}

void ULyraGameplayCueManager::OnCreated()
{
	Super::OnCreated();

	// 创建完成后更新监听器（依据 LoadMode 决定是否注册 tag/load 监听）
	UpdateDelayLoadDelegateListeners();
}

bool ULyraGameplayCueManager::ShouldDeferScanningRuntimeLibraries() const
{
	// 编辑器启动时 GameplayCueManager 往往早于 AssetRegistry 完成初始扫描。
	// 延迟后由引擎在 OnKnownGathersComplete 回调重新初始化，确保 /Game 下的 GCN 已进入 Runtime CueSet。
	return true;
}

void ULyraGameplayCueManager::LoadAlwaysLoadedCues()
{
	// 如果当前策略是“延迟加载”，则额外加载一组始终应该加载的 Cue（比如代码里硬编码的）
	if (ShouldDelayLoadGameplayCues())
	{
		UGameplayTagsManager& TagManager = UGameplayTagsManager::Get();
	
		//@TODO: Try to collect these by filtering GameplayCue. tags out of native gameplay tags?
		TArray<FName> AdditionalAlwaysLoadedCueTags;

		for (const FName& CueTagName : AdditionalAlwaysLoadedCueTags)
		{
			FGameplayTag CueTag = TagManager.RequestGameplayTag(CueTagName, /*ErrorIfNotFound=*/ false);
			if (CueTag.IsValid())
			{
				ProcessTagToPreload(CueTag, nullptr);
			}
			else
			{
				UE_LOG(LogShoot, Warning, TEXT("ULyraGameplayCueManager::AdditionalAlwaysLoadedCueTags contains invalid tag %s"), *CueTagName.ToString());
			}
		}
	}
}
bool ULyraGameplayCueManager::ShouldAsyncLoadRuntimeObjectLibraries() const
{
	switch (LyraGameplayCueManagerCvars::LoadMode)
	{
	case ELyraEditorLoadMode::LoadUpfront:
		return true;
	case ELyraEditorLoadMode::PreloadAsCuesAreReferenced_GameOnly:
#if WITH_EDITOR
		if (GIsEditor)
		{
			return false;
		}
#endif
		break;
	case ELyraEditorLoadMode::PreloadAsCuesAreReferenced:
		break;
	}

	return !ShouldDelayLoadGameplayCues();
}

// 不希望在缺失时做同步加载（会卡主线程）
bool ULyraGameplayCueManager::ShouldSyncLoadMissingGameplayCues() const
{
	return false;
}

// 允许在缺失时异步加载（更友好）
bool ULyraGameplayCueManager::ShouldAsyncLoadMissingGameplayCues() const
{
	return true;
}
void ULyraGameplayCueManager::DumpGameplayCues(const TArray<FString>& Args)
{
	ULyraGameplayCueManager* GCM = Cast<ULyraGameplayCueManager>(UAbilitySystemGlobals::Get().GetGameplayCueManager());
	if (!GCM)
	{
		UE_LOG(LogShoot, Error, TEXT("DumpGameplayCues failed. No ULyraGameplayCueManager found."));
		return;
	}

	const bool bIncludeRefs = Args.Contains(TEXT("Refs"));

	UE_LOG(LogShoot, Log, TEXT("=========== Dumping Always Loaded Gameplay Cue Notifies ==========="));
	for (UClass* CueClass : GCM->AlwaysLoadedCues)
	{
		UE_LOG(LogShoot, Log, TEXT("  %s"), *GetPathNameSafe(CueClass));
	}

	UE_LOG(LogShoot, Log, TEXT("=========== Dumping Preloaded Gameplay Cue Notifies ==========="));
	for (UClass* CueClass : GCM->PreloadedCues)
	{
		TSet<FObjectKey>* ReferencerSet = GCM->PreloadedCueReferencers.Find(CueClass);
		int32 NumRefs = ReferencerSet ? ReferencerSet->Num() : 0;
		UE_LOG(LogShoot, Log, TEXT("  %s (%d refs)"), *GetPathNameSafe(CueClass), NumRefs);
		if (bIncludeRefs && ReferencerSet)
		{
			for (const FObjectKey& Ref : *ReferencerSet)
			{
				UObject* RefObject = Ref.ResolveObjectPtr();
				UE_LOG(LogShoot, Log, TEXT("    ^- %s"), *GetPathNameSafe(RefObject));
			}
		}
	}

	// 列出 runtime 库中在内存里已 load 的 cue（但又不在 AlwaysLoaded/Preloaded 列表中的）
	UE_LOG(LogShoot, Log, TEXT("=========== Dumping Gameplay Cue Notifies loaded on demand ==========="));
	int32 NumMissingCuesLoaded = 0;
	if (GCM->RuntimeGameplayCueObjectLibrary.CueSet)
	{
		for (const FGameplayCueNotifyData& CueData : GCM->RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueData)
		{
			if (CueData.LoadedGameplayCueClass && !GCM->AlwaysLoadedCues.Contains(CueData.LoadedGameplayCueClass) && !GCM->PreloadedCues.Contains(CueData.LoadedGameplayCueClass))
			{
				NumMissingCuesLoaded++;
				UE_LOG(LogShoot, Log, TEXT("  %s"), *CueData.LoadedGameplayCueClass->GetPathName());
			}
		}
	}

	UE_LOG(LogShoot, Log, TEXT("=========== Gameplay Cue Notify summary ==========="));
	UE_LOG(LogShoot, Log, TEXT("  ... %d cues in always loaded list"), GCM->AlwaysLoadedCues.Num());
	UE_LOG(LogShoot, Log, TEXT("  ... %d cues in preloaded list"), GCM->PreloadedCues.Num());
	UE_LOG(LogShoot, Log, TEXT("  ... %d cues loaded on demand"), NumMissingCuesLoaded);
	UE_LOG(LogShoot, Log, TEXT("  ... %d cues in total"), GCM->AlwaysLoadedCues.Num() + GCM->PreloadedCues.Num() + NumMissingCuesLoaded);
}
void ULyraGameplayCueManager::OnGameplayTagLoaded(const FGameplayTag& Tag)
{
	// 加锁把 Tag 与当前序列化上下文中的 OwningObject（如果有）记录下来，稍后在游戏线程处理
	FScopeLock ScopeLock(&LoadedGameplayTagsToProcessCS);
	bool bStartTask = LoadedGameplayTagsToProcess.Num() == 0;
	FUObjectSerializeContext* LoadContext = FUObjectThreadContext::Get().GetSerializeContext();
	UObject* OwningObject = LoadContext ? LoadContext->SerializedObject : nullptr;
	LoadedGameplayTagsToProcess.Emplace(Tag, OwningObject);

	// 只有第一个 tag 启动任务（避免大量 push 时发太多任务）
	if (bStartTask)
	{
		// 创建一个 GraphTask 并安排在 GameThread 执行
		TGraphTask<FGameplayCueTagThreadSynchronizeGraphTask>::CreateTask().ConstructAndDispatchWhenReady([]()
			{
				if (GIsRunning)
				{
					if (ULyraGameplayCueManager* StrongThis = Get())
					{
						// Unreal 在 GC 时会禁止某些静态 UObject 查找操作，所以如果正在 GC，就延迟到 GC 完毕再处理
						if (IsGarbageCollecting())
						{
							StrongThis->bProcessLoadedTagsAfterGC = true;
						}
						else
						{
							StrongThis->ProcessLoadedTags();
						}
					}
				}
			});
	}
}

// CH: HandlePostGarbageCollect 在 GC 后运行，用于延迟处理之前因 GC 而被推迟的 Tag 处理逻辑。
void ULyraGameplayCueManager::HandlePostGarbageCollect()
{
	if (bProcessLoadedTagsAfterGC)
	{
		ProcessLoadedTags();
	}
	bProcessLoadedTagsAfterGC = false;
}

void ULyraGameplayCueManager::ProcessLoadedTags()
{
	TArray<FLoadedGameplayTagToProcessData> TaskLoadedGameplayTagsToProcess;
	{
		// 把队列取走，减少锁粒度
		FScopeLock TaskScopeLock(&LoadedGameplayTagsToProcessCS);
		TaskLoadedGameplayTagsToProcess = LoadedGameplayTagsToProcess;
		LoadedGameplayTagsToProcess.Empty();
	}

	// 只有在游戏运行时且 RuntimeCueSet 存在时处理
	if (GIsRunning)
	{
		if (RuntimeGameplayCueObjectLibrary.CueSet)
		{
			for (const FLoadedGameplayTagToProcessData& LoadedTagData : TaskLoadedGameplayTagsToProcess)
			{
				if (RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueDataMap.Contains(LoadedTagData.Tag))
				{
					if (!LoadedTagData.WeakOwner.IsStale())
					{
						// 根据 tag 去预加载 Cue（或标记）
						ProcessTagToPreload(LoadedTagData.Tag, LoadedTagData.WeakOwner.Get());
					}
				}
			}
		}
		else
		{
			UE_LOG(LogShoot, Warning, TEXT("ULyraGameplayCueManager::OnGameplayTagLoaded processed loaded tag(s) but RuntimeGameplayCueObjectLibrary.CueSet was null. Skipping processing."));
		}
	}
}

void ULyraGameplayCueManager::ProcessTagToPreload(const FGameplayTag& Tag, UObject* OwningObject)
{
	switch (LyraGameplayCueManagerCvars::LoadMode)
	{
	case ELyraEditorLoadMode::LoadUpfront:
		// 如果是 upfront 模式，这里不做延迟加载的处理（因为上层在启动时已全部加载）
			return;
	case ELyraEditorLoadMode::PreloadAsCuesAreReferenced_GameOnly:
#if WITH_EDITOR
		if (GIsEditor)
		{
			// editor 下不处理（允许 editor 在触发时再加载，以便快速迭代）
			return;
		}
#endif
		break;
	case ELyraEditorLoadMode::PreloadAsCuesAreReferenced:
		break;
	}

	check(RuntimeGameplayCueObjectLibrary.CueSet);

	int32* DataIdx = RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueDataMap.Find(Tag);
	if (DataIdx && RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueData.IsValidIndex(*DataIdx))
	{
		const FGameplayCueNotifyData& CueData = RuntimeGameplayCueObjectLibrary.CueSet->GameplayCueData[*DataIdx];

		// 先尝试在内存中找 UClass*（可能已有）
		UClass* LoadedGameplayCueClass = FindObject<UClass>(nullptr, *CueData.GameplayCueNotifyObj.ToString());
		if (LoadedGameplayCueClass)
		{
			// 已经加载：直接注册
			RegisterPreloadedCue(LoadedGameplayCueClass, OwningObject);
		}
		else
		{
			// 没加载：发起异步加载，完成后回调 OnPreloadCueComplete
			bool bAlwaysLoadedCue = OwningObject == nullptr;
			TWeakObjectPtr<UObject> WeakOwner = OwningObject;
			// CH: 重点 —— 使用 StreamableManager 异步加载软引用对象（GameplayCueNotifyObj）
			//     StreamableManager.RequestAsyncLoad(...) 会在后台加载资源（不会卡主线程），加载完成后调用绑定的委托(OnPreloadCueComplete)。
			//     参数解释（常用）:
			//       - FSoftObjectPath: 软引用路径
			//       - FStreamableDelegate: 完成回调（这里绑定到 OnPreloadCueComplete）
			//       - Priority: 加载优先级
			//       - bManageActiveHandle: 是否管理返回的 Handle（false 表示不用保留 handle）
			//       - bAsyncLoadOnly: 是否仅允许异步加载（false 表示允许同步降级）
			//       - Tag: 用于跟踪调试的tag
			//
			//     副作用/注意：
			//       - 异步加载会消耗 IO / CPU（解压、构建对象），可能会在第一次触发 Cue 时出现延迟（如果尚未加载完）。
			//       - 如果客户端 AssetManager/StreamableManager 配置不当，可能会导致重复加载或资源未释放。
			StreamableManager.RequestAsyncLoad(
				CueData.GameplayCueNotifyObj,
				FStreamableDelegate::CreateUObject(this, &ThisClass::OnPreloadCueComplete, CueData.GameplayCueNotifyObj, WeakOwner, bAlwaysLoadedCue),
				FStreamableManager::DefaultAsyncLoadPriority,
				false, false, TEXT("GameplayCueManager")
			);
		}
	}
}

void ULyraGameplayCueManager::OnPreloadCueComplete(FSoftObjectPath Path, TWeakObjectPtr<UObject> OwningObject, bool bAlwaysLoadedCue)
{
	// 仅当 OwningObject 仍然有效或者这是“始终加载”项，才注册（避免无用加载）
	if (bAlwaysLoadedCue || OwningObject.IsValid())
	{
		if (UClass* LoadedGameplayCueClass = Cast<UClass>(Path.ResolveObject()))
		{
			RegisterPreloadedCue(LoadedGameplayCueClass, OwningObject.Get());
		}
	}
}

void ULyraGameplayCueManager::RegisterPreloadedCue(UClass* LoadedGameplayCueClass, UObject* OwningObject)
{
	check(LoadedGameplayCueClass);

	const bool bAlwaysLoadedCue = OwningObject == nullptr;
	if (bAlwaysLoadedCue)
	{
		// 如果标记为始终加载，则放入 AlwaysLoaded 集合
		AlwaysLoadedCues.Add(LoadedGameplayCueClass);
		PreloadedCues.Remove(LoadedGameplayCueClass);
		PreloadedCueReferencers.Remove(LoadedGameplayCueClass);
	}
	else if ((OwningObject != LoadedGameplayCueClass) && (OwningObject != LoadedGameplayCueClass->GetDefaultObject()) && !AlwaysLoadedCues.Contains(LoadedGameplayCueClass))
	{
		// 正常预加载：把类加入预加载集合，并记录引用者（OwningObject）
		PreloadedCues.Add(LoadedGameplayCueClass);
		TSet<FObjectKey>& ReferencerSet = PreloadedCueReferencers.FindOrAdd(LoadedGameplayCueClass);
		ReferencerSet.Add(OwningObject);
	}
}

void ULyraGameplayCueManager::HandlePostLoadMap(UWorld* NewWorld)
{
	// 如果 runtime cue set 有记录，先把 AlwaysLoaded/Preloaded 类从 CueSet 标记移除（让 AssetManager 管理 clean）
	if (RuntimeGameplayCueObjectLibrary.CueSet)
	{
		for (UClass* CueClass : AlwaysLoadedCues)
		{
			RuntimeGameplayCueObjectLibrary.CueSet->RemoveLoadedClass(CueClass);
		}

		for (UClass* CueClass : PreloadedCues)
		{
			RuntimeGameplayCueObjectLibrary.CueSet->RemoveLoadedClass(CueClass);
		}
	}

	// 清理那些引用已失效的预加载引用集合
	for (auto CueIt = PreloadedCues.CreateIterator(); CueIt; ++CueIt)
	{
		TSet<FObjectKey>& ReferencerSet = PreloadedCueReferencers.FindChecked(*CueIt);
		for (auto RefIt = ReferencerSet.CreateIterator(); RefIt; ++RefIt)
		{
			if (!RefIt->ResolveObjectPtr())
			{
				RefIt.RemoveCurrent();
			}
		}
		if (ReferencerSet.Num() == 0)
		{
			PreloadedCueReferencers.Remove(*CueIt);
			CueIt.RemoveCurrent();
		}
	}
}

void ULyraGameplayCueManager::UpdateDelayLoadDelegateListeners()
{
	UGameplayTagsManager::Get().OnGameplayTagLoadedDelegate.RemoveAll(this);
	FCoreUObjectDelegates::GetPostGarbageCollect().RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	switch (LyraGameplayCueManagerCvars::LoadMode)
	{
	case ELyraEditorLoadMode::LoadUpfront:
		return;
	case ELyraEditorLoadMode::PreloadAsCuesAreReferenced_GameOnly:
#if WITH_EDITOR
		if (GIsEditor)
		{
			return;
		}
#endif
		break;
	case ELyraEditorLoadMode::PreloadAsCuesAreReferenced:
		break;
	}

	// 注册：当新的 GameplayTag 被加载时，会回调 OnGameplayTagLoaded
	UGameplayTagsManager::Get().OnGameplayTagLoadedDelegate.AddUObject(this, &ThisClass::OnGameplayTagLoaded);

	// 注册 GC 完成后的回调（如果在 GC 时延迟了 tag 处理，GC 后触发实际处理）
	FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &ThisClass::HandlePostGarbageCollect);

	// 注册地图加载后回调（清理预加载集合）
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandlePostLoadMap);
}

bool ULyraGameplayCueManager::ShouldDelayLoadGameplayCues() const
{
	const bool bClientDelayLoadGameplayCues = true;
	// 不在 dedicated server 上启用延迟加载（服务端通常要权威、及时）
	return !IsRunningDedicatedServer() && bClientDelayLoadGameplayCues;
}

const FPrimaryAssetType UFortAssetManager_GameplayCueRefsType = TEXT("GameplayCueRefs");
const FName UFortAssetManager_GameplayCueRefsName = TEXT("GameplayCueReferences");
const FName UFortAssetManager_LoadStateClient = FName(TEXT("Client"));

void ULyraGameplayCueManager::RefreshGameplayCuePrimaryAsset()
{
	TArray<FSoftObjectPath> CuePaths;
	UGameplayCueSet* RuntimeGameplayCueSet = GetRuntimeCueSet();
	if (RuntimeGameplayCueSet)
	{
		// 从 CueSet 获取所有 soft object paths（即当 cue 是 soft ref 时可以拿到路径）
		RuntimeGameplayCueSet->GetSoftObjectPaths(CuePaths);
	}

	// 把这些路径加入一个 bundle（标记为 client 下的 bundle）
	FAssetBundleData BundleData;
	BundleData.AddBundleAssetsTruncated(UFortAssetManager_LoadStateClient, CuePaths);

	// 构造一个 PrimaryAssetId 并交给 AssetManager 动态记录——这一步是给打包/加载系统做提示
	FPrimaryAssetId PrimaryAssetId = FPrimaryAssetId(UFortAssetManager_GameplayCueRefsType, UFortAssetManager_GameplayCueRefsName);
	UAssetManager::Get().AddDynamicAsset(PrimaryAssetId, FSoftObjectPath(), BundleData);
}
