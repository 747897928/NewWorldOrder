// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayCueManager.h"
#include "LyraGameplayCueManager.generated.h"

class FString;
class UClass;
class UObject;
class UWorld;
struct FObjectKey;

/**
 * ULyraGameplayCueManager Game-specific manager for gameplay cues - Lyra专用的GameplayCue管理器
 *
 * 核心功能：延迟加载GameplayCue资源，避免启动时加载所有Cue导致的性能问题
 * 传统问题：默认GameplayCueManager会在启动时扫描并加载所有Cue，导致内存占用高和启动卡顿
 * Lyra解决方案：按需加载，只在Cue被引用时才加载对应的资源
 * 
 */
UCLASS()
class ULyraGameplayCueManager : public UGameplayCueManager
{
	GENERATED_BODY()

public:
	ULyraGameplayCueManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 获取单例实例
	static ULyraGameplayCueManager* Get();

	//~UGameplayCueManager interface
	virtual void OnCreated() override;
	/** AssetRegistry 完成初始扫描后再建立 Runtime CueSet，避免编辑器启动早期得到空库。 */
	virtual bool ShouldDeferScanningRuntimeLibraries() const override;
	
	/**
	 * 核心函数：是否异步加载运行时对象库
	 * 返回false表示不在一开始就加载所有Cue，改为按需加载
	 */
	virtual bool ShouldAsyncLoadRuntimeObjectLibraries() const override;
	
	// 同步加载缺失的GameplayCue（通常返回false，使用异步）
	virtual bool ShouldSyncLoadMissingGameplayCues() const override;
	
	// 异步加载缺失的GameplayCue（返回true，提高响应性）
	virtual bool ShouldAsyncLoadMissingGameplayCues() const override;
	//~End of UGameplayCueManager interface

	// 调试命令：输出所有已加载的GameplayCue信息
	static void DumpGameplayCues(const TArray<FString>& Args);

	// 加载必须始终加载的Cue（代码引用的核心Cue）
	// When delay loading cues, this will load the cues that must be always loaded anyway
	void LoadAlwaysLoadedCues();

	// 更新GameplayCue主资源包（用于资源管理系统）
	// Updates the bundles for the singular gameplay cue primary asset
	void RefreshGameplayCuePrimaryAsset();

private:
	// 当GameplayTag加载完成时的回调
	void OnGameplayTagLoaded(const FGameplayTag& Tag);
	
	// 垃圾回收后的处理
	void HandlePostGarbageCollect();
	
	// 处理已加载的Tag队列
	void ProcessLoadedTags();
	
	// 预加载指定Tag对应的Cue
	void ProcessTagToPreload(const FGameplayTag& Tag, UObject* OwningObject);
	
	// Cue预加载完成的回调
	void OnPreloadCueComplete(FSoftObjectPath Path, TWeakObjectPtr<UObject> OwningObject, bool bAlwaysLoadedCue);
	
	// 注册已预加载的Cue
	void RegisterPreloadedCue(UClass* LoadedGameplayCueClass, UObject* OwningObject);
	
	// 地图加载后的清理
	void HandlePostLoadMap(UWorld* NewWorld);
	
	// 更新延迟加载的委托监听器
	void UpdateDelayLoadDelegateListeners();
	
	// 判断是否应该延迟加载GameplayCue
	bool ShouldDelayLoadGameplayCues() const;

private:
	// 预加载的Cue数据结构
	struct FLoadedGameplayTagToProcessData
	{
		FGameplayTag Tag;
		TWeakObjectPtr<UObject> WeakOwner; // 弱引用，避免阻止垃圾回收

		FLoadedGameplayTagToProcessData() {}
		FLoadedGameplayTagToProcessData(const FGameplayTag& InTag, const TWeakObjectPtr<UObject>& InWeakOwner) 
			: Tag(InTag), WeakOwner(InWeakOwner) {}
	};

private:
	// 客户端预加载的Cue（由于内容引用而预加载）
	// Cues that were preloaded on the client due to being referenced by content
	UPROPERTY(transient)
	TSet<TObjectPtr<UClass>> PreloadedCues;
	
	// 记录每个Cue被哪些对象引用（用于引用计数）
	TMap<FObjectKey, TSet<FObjectKey>> PreloadedCueReferencers;

	// 始终加载的Cue（代码引用或显式标记为始终加载）
	// Cues that were preloaded on the client and will always be loaded (code referenced or explicitly always loaded)
	UPROPERTY(transient)
	TSet<TObjectPtr<UClass>> AlwaysLoadedCues;

	// 待处理的已加载Tag队列（线程安全）
	TArray<FLoadedGameplayTagToProcessData> LoadedGameplayTagsToProcess;
	FCriticalSection LoadedGameplayTagsToProcessCS; // 线程安全锁
	
	// 标记是否需要在GC后处理已加载的Tag
	bool bProcessLoadedTagsAfterGC = false;
};
