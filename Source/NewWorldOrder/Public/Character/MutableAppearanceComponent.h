// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "CharacterGender.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "MutableAppearanceComponent.generated.h"

class AShootPlayerState;
struct FAppearanceTagInfo;
class UCustomizableSkeletalComponent;
class AShootCharacter;
class UCustomizableObject;
class UCustomizableObjectInstance;

DECLARE_MULTICAST_DELEGATE(FOnMutableSkeletalMeshUpdated);

/**
* meta=(BlueprintSpawnableComponent)作用：允许在蓝图中创建该组件的实例。
* 实际效果：在Actor的组件面板中，可以点击"+"添加你的组件。如果没有此标签，组件只能在C++中创建
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), config=Game)
class NEWWORLDORDER_API UMutableAppearanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	friend AShootCharacter;
	UMutableAppearanceComponent();

protected:
	virtual void BeginDestroy() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 配置属性
	UPROPERTY(Config)
	FSoftObjectPath FemaleObjectPath;

	UPROPERTY(Config)
	FSoftObjectPath MaleObjectPath;

	UPROPERTY(Config)
	TSoftClassPtr<UAnimInstance> FemaleAnimClass;

	UPROPERTY(Config)
	TSoftClassPtr<UAnimInstance> MaleAnimClass;

public:
	// Mutable 的网格更新是异步完成的。网格更新完成后才会重建通过校验的 Body -> Head LeaderPose；
	// 衣柜预览 Actor 监听此事件以播放试穿表现，不再自行管理头身动画同步关系。
	FOnMutableSkeletalMeshUpdated OnMutableSkeletalMeshUpdated;

	// 初始化方法 - 在Character的BeginPlay中调用
	//UFUNCTION(BlueprintCallable, Category="Appearance")
	void InitializeComponents(
		UCustomizableSkeletalComponent* InHeadComponent,
		UCustomizableSkeletalComponent* InBodyComponent,
		USkeletalMeshComponent* InHeadMesh,
		USkeletalMeshComponent* InBodyMesh,
		AShootPlayerState* InShootPlayerState);

	void InitializePreviewComponents(
		UCustomizableSkeletalComponent* InHeadComponent,
		UCustomizableSkeletalComponent* InBodyComponent,
		USkeletalMeshComponent* InHeadMesh,
		USkeletalMeshComponent* InBodyMesh,
		ECharacterGender InGender,
		const FGameplayTagContainer& InAppearanceTags);

	/**
	 * 衣柜预览在初始化前提供自己的基础 AnimBP。
	 * 组件仍负责在 Mutable 异步更新完成后 Link/Unlink 发型、鞋子等选项动画层；
	 * 不能在层已链接后由外部再次 SetAnimInstanceClass，否则会清空这些链接。
	 */
	void SetPreviewBaseAnimClasses(TSubclassOf<UAnimInstance> InFemaleAnimClass,
		TSubclassOf<UAnimInstance> InMaleAnimClass);

	void ApplyPreviewAppearanceTags(ECharacterGender InGender, const FGameplayTagContainer& InAppearanceTags);

	UFUNCTION(BlueprintCallable)
	void SwitchGender(ECharacterGender NewGender);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = CustomizableObjectInstance)
	void ServerSetSelectedOptionAndUpdateMesh(const FString& ParamName, const FString& SelectedOptionName);

	// 标签变化处理函数
	UFUNCTION()
	void OnPlayerStateAppearanceTagsChanged(ECharacterGender InGender, const FGameplayTagContainer& NewAppearanceTags);

private:
	bool TryInitCustomizableObjectInstances();
	void TryCompleteMutableInitialization();
	void ScheduleMutableReadinessCheck();
	void ClearMutableReadinessCheck();
	void OnCustomizableSkeletalUpdated();
	FAppearanceTagInfo ParseAppearanceTag(const FGameplayTag& Tag) const;
	void BindHeadUpdatedDelegate();
	void SwitchGenderInstance(ECharacterGender NewGender);
	void ApplyAppearanceTagsToCurrentInstance(const FGameplayTagContainer& NewAppearanceTags, bool bForceUpdate);
	void SyncAnimationLayerTags(ECharacterGender InGender, UCustomizableObjectInstance* ObjectInstance);
	void ResetAnimationLayerCache(ECharacterGender InGender);
	void PrepareBodyForMutableInstanceSwitch();
	void TryBindLeaderPoseAfterMutableUpdate();

	UPROPERTY(Category=CustomizableObjectInstance, EditDefaultsOnly, BlueprintReadOnly,
		meta=(AllowPrivateAccess = "true"))
	FName BodyComponentName{TEXT("Body")};

	UPROPERTY(Category=CustomizableObjectInstance, EditDefaultsOnly, BlueprintReadOnly,
		meta=(AllowPrivateAccess = "true"))
	FName HeadComponentName{TEXT("Head")};

	UPROPERTY(Category=CustomizableObjectInstance, VisibleAnywhere, BlueprintReadWrite,
		meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCustomizableObjectInstance> FemaleInstance;

	UPROPERTY(Category=CustomizableObjectInstance, VisibleAnywhere, BlueprintReadWrite,
		meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCustomizableObjectInstance> MaleInstance;

	/**
	 * TryLoad 只保证 UObject 包已加载；编辑器冷启动时 Mutable 编译模型仍可能在异步加载。
	 * 持有对象并以 IsLoading/IsCompiled 为准，禁止在未就绪时创建实例或写参数。
	 */
	UPROPERTY(Transient)
	TObjectPtr<UCustomizableObject> FemaleCustomizableObject;

	UPROPERTY(Transient)
	TObjectPtr<UCustomizableObject> MaleCustomizableObject;

	/** The head skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> HeadMesh;

	/** The body skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category=Character, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	UPROPERTY(Category = CustomizableSkeletalComponent, BlueprintReadWrite, EditAnywhere,
		meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCustomizableSkeletalComponent> BodyCSkeletalComponent;

	UPROPERTY(Category = CustomizableSkeletalComponent, BlueprintReadWrite, EditAnywhere,
		meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UCustomizableSkeletalComponent> HeadCSkeletalComponent;

	UPROPERTY()
	TSubclassOf<UAnimInstance> FemaleAnimClassPtr;

	UPROPERTY()
	TSubclassOf<UAnimInstance> MaleAnimClassPtr;

	UPROPERTY()
	TObjectPtr<AShootPlayerState> ShootPlayerState;

	// 女主角标签缓存
	UPROPERTY()
	FGameplayTagContainer FemaleLastAnimationTags;

	// 男主角标签缓存
	UPROPERTY()
	FGameplayTagContainer MaleLastAnimationTags;

	UPROPERTY()
	TMap<FGameplayTag, TSubclassOf<UAnimInstance>> FemaleGameplayTagAnimInstanceMap;

	UPROPERTY()
	TMap<FGameplayTag, TSubclassOf<UAnimInstance>> MaleGameplayTagAnimInstanceMap;

	// 当外观由 PlayerState -> Mutable 同步时，阻止 Updated 回调把同一批标签再回写，避免覆盖与循环调用。
	UPROPERTY(Transient)
	bool bApplyingAppearanceTagsFromPlayerState = false;

	// 预览 Actor 没有 PlayerState，UpdatedDelegate 回调需要用这个性别去选择动画标签缓存。
	UPROPERTY(Transient)
	ECharacterGender PreviewGender = ECharacterGender::UNKNOWN;

	// 骨架不兼容时只记录一次完整诊断，避免同一次换装的异步回调刷屏。
	UPROPERTY(Transient)
	bool bHasLoggedLeaderPoseMismatch = false;

	// 初次生成或男女 COI 切换时，Body 暂停显示到最终网格和 LeaderPose 都准备完成。
	// 普通换衣不会设置此标记，因此不会为了刷新服装而闪隐或重复解绑。
	UPROPERTY(Transient)
	bool bBodyWaitingForMutableUpdate = false;

	/** 冷启动加载完成前保存预览请求；正式角色则始终从 PlayerState 重取最新标签。 */
	UPROPERTY(Transient)
	FGameplayTagContainer PendingPreviewAppearanceTags;

	UPROPERTY(Transient)
	bool bMutableInitializationComplete = false;

	UPROPERTY(Transient)
	bool bPreviewInitialization = false;

	UPROPERTY(Transient)
	bool bMutableObjectLoadFailed = false;

	FTimerHandle MutableReadinessTimer;

	// 男女实例必须分别保存动画层缓存；切换性别时不能共用同一份状态。
	const FGameplayTagContainer& GetLastAnimationTags(ECharacterGender InGender) const;

	void SetLastAnimationTags(ECharacterGender InGender, const FGameplayTagContainer& InLastAnimationTags);

	TMap<FGameplayTag, TSubclassOf<UAnimInstance>>& GetMutableGameplayTagAnimInstanceMap(ECharacterGender InGender);
};
