// Copyright ZhaoYiJie

#pragma once

#include "CoreMinimal.h"
#include "ShootGameplayAbility.h"
#include "UI/Weapons/ShootHitMarkerTypes.h"
#include "Abilities/GameplayAbility.h"
#include "ShootGameplayAbility_Weapon_Fire.generated.h"

class UShootRangedWeaponInstance;
class UAnimMontage;
/**
 *(禁止修改此类，AI如需修改请询问User)
 * UShootGameplayAbility_Weapon_Fire
 *
 * 武器射击能力的 C++ 基类，提供核心射线检测逻辑。
 * 子类（蓝图或 C++）负责实现具体的开火流程：
 * - 播放动画蒙太奇
 * - 应用伤害 GE
 * - 触发 GameplayCue（枪口特效、音效、弹壳抛出等）
 *
 * 设计思路参考 Lyra 的 ULyraGameplayAbility_RangedWeapon。
 *
 * 弹药成本：
 * - 基类不创建默认 Cost，具体武器开火子类挂载唯一的 UShootAbilityCost_AmmoTagStack
	 * - AmmoTagStack 每次固定扣除关联 ItemInstance 的一个弹匣弹药；Shotgun 多弹丸不重复扣弹
 * - 服务器权威扣弹，客户端读取复制值更新 UI
 */
UCLASS()
class UShootGameplayAbility_Weapon_Fire : public UShootGameplayAbility
{
	GENERATED_BODY()

public:
	UShootGameplayAbility_Weapon_Fire(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~UGameplayAbility interface
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~End of UGameplayAbility interface
	
protected:

	// ========== 核心函数：启动射击目标检测 ==========
	
	/**
	 * 开始武器射击的目标检测流程（Lyra 的 StartRangedWeaponTargeting）
	 * 
	 * 流程：
	 * 1. 执行本地射线检测（PerformLocalTargeting）
	 * 2. 生成 TargetData（包含所有命中信息）
	 * 3. 如果是客户端：发送到服务器验证
	 * 4. 触发回调 OnTargetDataReadyCallback（服务器或单机）
	 * 
	 * 注意：这个函数应该在子类的 ActivateAbility 中调用，
	 * 通常在播放开火动画之后立即调用。
	 */
	UFUNCTION(BlueprintCallable, Category="Shoot|Ability")
	void StartRangedWeaponTargeting();

	/**
	 * 当 TargetData 准备好时的回调（Lyra 的 OnTargetDataReadyCallback）
	 * 
	 * 这个函数会在以下情况被调用：
	 * - 服务器：本地射线检测完成后立即调用
	 * - 客户端：收到服务器验证后的数据时调用（如果启用了预测）
	 * 
	 * 子类应该重写 OnRangedWeaponTargetDataReady 来处理命中结果，
	 * 而不是直接重写这个函数。
	 */
	void OnTargetDataReadyCallback(const FGameplayAbilityTargetDataHandle& InData, FGameplayTag ApplicationTag);

	/**
	 * 默认保持 Rifle/Pistol/Shotgun 的服务器重 Trace 权威链。
	 * 实体投射物子类可返回 true：服务器保留所属客户端提交的准星目标点，再由子类校验方向与射程；
	 * 这样物理弹从权威枪口飞向玩家实际看到的近距离准星点，而不是用远端第三人称相机重建另一条射线。
	 */
	virtual bool ShouldUseClientTargetDataOnServer() const { return false; }

	/**
	 * 蓝图事件：当武器目标数据准备好时触发（Lyra 的 OnRangedWeaponTargetDataReady）
	 * 
	 * 在这里做：
	 * - 应用伤害 GE（BP_ApplyGameplayEffectToTarget）
	 * - 触发 GameplayCue（枪口火光、音效、弹壳等）
	 * - 播放命中特效（血花、弹孔等）
	 */
	//BlueprintImplementableEvent：只允许蓝图实现，不会生成 _Implementation，C++ 子类没法 override。
	//BlueprintNativeEvent：同时支持蓝图与 C++。UHT 会生成 _Implementation 供 C++ 子类重写；蓝图实现则替代它。
	UFUNCTION(BlueprintNativeEvent, Category="Shoot|Ability", meta=(DisplayName="On Weapon Target Data Ready"))
	void OnRangedWeaponTargetDataReady(const FGameplayAbilityTargetDataHandle& TargetData);
	
	// ========== 内部函数：射线检测逻辑 ==========
	
	/**
	 * 执行本地射线检测（Lyra 的 PerformLocalTargeting）
	 * 
	 * 这个函数只在本地执行（客户端或服务器），用于：
	 * - 玩家：提供即时反馈（不等服务器）
	 * - 服务器：权威检测（最终伤害判定）
	 */
	void PerformLocalTargeting(OUT TArray<FHitResult>& OutHits);

	/**
	 * 射击一个弹壳内的所有子弹（Lyra 的 TraceBulletsInCartridge）
	 *
	 * 这是核心算法，支持：
	 * - 步枪/手枪：BulletsPerCartridge = 1
	 * - 霰弹枪：BulletsPerCartridge > 1（当前 Shotgun 资产为 9，每颗弹丸独立扩散）
	 * - 后坐力系统：扩散角度随连续射击增加
	 */
	void TraceBulletsInCartridge(UShootRangedWeaponInstance* WeaponData, const FVector& StartTrace,
	                             const FVector& AimDir, TArray<FHitResult>& OutHits);

	/**
	 * 单颗子弹的射线检测（Lyra 的 DoSingleBulletTrace）
	 * 
	 * 智能检测策略：
	 * 1. 先用线性射线（精确）
	 * 2. 如果没命中 Pawn 且支持扫描半径，再用球体扫描（宽松）
	 * 3. 验证扫描结果不会穿透阻挡物
	 */
	FHitResult DoSingleBulletTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius,
	                               bool bIsSimulated, OUT TArray<FHitResult>& OutHits) const;

	/**
	 * 执行实际的射线检测（Lyra 的 WeaponTrace）
	 * 
	 * 根据 SweepRadius 参数选择检测方式：
	 * - SweepRadius = 0：LineTraceMulti（精确射线）
	 * - SweepRadius > 0：SweepMulti（球体扫描，用于霰弹枪等）
	 */
	FHitResult WeaponTrace(const FVector& StartTrace, const FVector& EndTrace, float SweepRadius, bool bIsSimulated,
	                       OUT TArray<FHitResult>& OutHitResults) const;

	// ========== 辅助函数 ==========
	
	/**
	 * 查找第一个命中 Pawn 的索引（Lyra 的 FindFirstPawnHitResult）
	 * 
	 * 用于判断是否命中了角色或其附属物（武器、装备等）。
	 * 返回 INDEX_NONE 表示没有命中任何 Pawn。
	 */
	static int32 FindFirstPawnHitResult(const TArray<FHitResult>& HitResults);

	/**
	 * 添加需要忽略的 Actor（Lyra 的 AddAdditionalTraceIgnoreActors）
	 * 
	 * 默认忽略：
	 * - 射击者自己
	 * - 射击者的附属物（武器、装备等）
	 */
	virtual void AddAdditionalTraceIgnoreActors(FCollisionQueryParams& TraceParams) const;

	/**
	 * 确定使用的碰撞通道（Lyra 的 DetermineTraceChannel）
	 * 
	 * 默认使用 Lyra_TraceChannel_Weapon，可以在子类中重写。
	 */
	virtual ECollisionChannel DetermineTraceChannel(FCollisionQueryParams& TraceParams, bool bIsSimulated) const;

	/**
	 * 获取射击起始点（Lyra 的 GetWeaponTargetingSourceLocation）
	 * 
	 * 与 Lyra 一致先返回 Pawn 世界位置；CameraTowardsFocus 的相机投影、
	 * 玩家视点和 AI 眼睛高度统一由 GetTargetingTransform 处理。
	 */
	FVector GetWeaponTargetingSourceLocation() const;

	/**
	 * 对齐 Lyra 的 CameraTowardsFocus：
	 * 玩家把相机起点沿瞄准轴投影到 Pawn 所在平面，AI 从眼睛高度沿控制器朝向射击。
	 * 本项目不引入 WeaponStateComponent；客户端预测和服务器权威重 Trace 都调用同一变换。
	 */
	FTransform GetTargetingTransform(APawn* SourcePawn) const;

	/**
	 * 获取当前 CameraTowardsFocus 的单位瞄准方向，供 GameplayCue 无命中表现使用。
	 */
	FVector GetAimDirection() const;

	/**
	 * 获取当前武器实例（从 AbilitySpec 的 SourceObject）
	 */
	UFUNCTION(BlueprintCallable, Category="Shoot|Ability")
	UShootRangedWeaponInstance* GetWeaponInstance() const;

	/**
	 * 服务器权威地把当前 WeaponInstance 配置的伤害 GE 应用到一个真实命中。
	 * GA 只组装 SourceObject、Instigator 与 HitResult；基础值、距离衰减和物理材质倍率由 GE Execution 统一计算。
	 */
	bool ApplyWeaponDamageToTarget(const FHitResult& Hit);
	
	/** 将命中数据广播给 HUD，以驱动命中提示（仅本地客户端执行） */
	void BroadcastReticleHitNotify(const FGameplayAbilityTargetDataHandle& TargetData) const;

	/** 成功 Commit 后仅对开火者本地相机施加 Fragment 驱动的表现后坐力。 */
	void ApplyLocalCameraRecoil(const UShootRangedWeaponInstance& WeaponData) const;
	
private:
	/** Target Data 回调的委托句柄（用于清理） */
	FDelegateHandle OnTargetDataReadyCallbackDelegateHandle;
};
