// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AnimationAssetFixerLibrary.generated.h"

/**
 * 项目维护的 AnimBlueprint 编辑器辅助工具。
 * 覆盖 AnimationLibrary.add_node_asset_override 处理不了的类型：
 * - AnimGraphNode_RotationOffsetBlendSpace（AimOffset）
 * - AnimGraphNode_SequenceEvaluator
 * 遍历蓝图所有图（含状态机子图/图层子图），对每个节点按前缀映射替换其引用的动画资产。
 */
UCLASS()
class ANIMATIONASSETFIXER_API UAnimationAssetFixerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 替换指定 AnimBlueprint 内所有图节点引用的动画资产。
	 * 同时通过 ReplaceReferredAnimations 更新原始序列化字段，并同步节点当前状态与未连接对象 Pin，
	 * 避免普通资产节点在编译时从对象 Pin 恢复旧引用；变量连接节点的编译 fallback 由
	 * RebuildLinkedAssetFallback 以“断开、结构编译、按 GUID 恢复连接”的受控流程处理。
	 * @param AnimBlueprintPath 蓝图路径（/Game/...）
	 * @param OldToNewPrefix 字符串数组：每项格式 "旧前缀|新前缀"，按前缀匹配替换
	 * @param bCompileAndSave 是否编译并保存蓝图
	 * @return 替换的节点数量（-1 = 加载蓝图失败）
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static int32 ReplaceNodeAssetReferences(const FString& AnimBlueprintPath, const TArray<FString>& OldToNewPrefix, bool bCompileAndSave);

	/**
	 * 列出指定 AnimBlueprint 所有图节点的资产引用。
	 * @param AnimBlueprintPath 蓝图路径
	 * @return 每项 "图名|节点类名|资产路径"；无资产的节点不返回
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static TArray<FString> ListNodeAssetReferences(const FString& AnimBlueprintPath);

	/**
	 * 列出 AnimBlueprint 全部子图中的节点和连线。
	 * VibeUE 的通用 Blueprint 图枚举不会返回 Linked Anim Layer 的内部子图，
	 * 因此动画迁移和故障定位统一从这里读取真实图结构，避免凭截图猜节点。
	 * @param GraphNameContains 可选图名过滤；空字符串表示全部图
	 * @return 每项 "图名|节点名|节点类|节点Guid|Pin方向:Pin名->连接节点.连接Pin"
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static TArray<FString> ListGraphNodes(const FString& AnimBlueprintPath, const FString& GraphNameContains);

	/**
	 * 导出 AnimGraph 节点内部的运行时结构。
	 * 通用蓝图 Pin 只能看到暴露参数，看不到 CopyBone/TwoBoneIK/FootPlacement 等节点保存的骨骼名；
	 * 该接口用于把 Lyra 原图与 CC 适配图逐项对照，禁止凭节点标题或截图猜配置。
	 * @return 每项 "图名|节点名|节点类|Node 结构文本"
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static TArray<FString> ListGraphNodeRuntimeProperties(
		const FString& AnimBlueprintPath,
		const FString& GraphNameContains,
		const FString& NodeClassNameContains);

	/**
	 * 重建一个由变量驱动的资产 Pin 的编译 fallback，同时保留原变量连接。
	 * 用于清理复制 AnimBP 后遗留在 AnimNodeData 中的旧骨架常量引用。
	 * 仅接受“精确一个节点、精确一个输入连接”的情况，避免批量猜测图拓扑。
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static bool RebuildLinkedAssetFallback(
		const FString& AnimBlueprintPath,
		const FString& GraphName,
		const FString& NodeClassNameContains,
		const FName AssetPinName,
		const FString& AnimationAssetPath);

	/**
	 * 批量设置指定子图、指定类名的 Skeletal Control 节点 Alpha。
	 * 这是 CC 骨架适配时的可逆 A/B 开关：先验证某类 Lyra 控制节点是否是畸变根因，
	 * 再决定保留、移除或按 CC 骨架重新标定，禁止把该函数放进运行时调用链。
	 * @return 修改的节点数量（-1 = 加载失败）
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static int32 SetSkeletalControlAlphas(
		const FString& AnimBlueprintPath,
		const FString& GraphNameContains,
		const FString& NodeClassNameContains,
		float Alpha,
		bool bCompileAndSave);

	/**
	 * 设置一个明确 Sequence Player 的循环标志。
	 *
	 * 一次性 StanceTransition / IdleBreak 使用自动剩余时间过渡时不能循环；
	 * 通用图工具看不到运行时 Node 结构，因此由项目编辑器辅助插件同步修改结构体和未连接 Pin。
	 * 函数要求图名和节点对象名精确匹配且只命中一个节点，任何歧义都会拒绝修改。
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static bool SetSequencePlayerLooping(
		const FString& AnimBlueprintPath,
		const FString& GraphName,
		const FString& NodeName,
		bool bLoopAnimation,
		bool bCompileAndSave);

	/** 列出 Skeleton 上的虚拟骨骼，每项格式为 "虚拟骨骼名|Source|Target"。 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static TArray<FString> ListVirtualBones(const FString& SkeletonPath);

	/**
	 * 幂等添加一个已明确 Source/Target 的命名虚拟骨骼。
	 * 同名但映射不同会拒绝修改，避免在 CC 骨架上静默覆盖错误地基。
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static bool EnsureNamedVirtualBone(
		const FString& SkeletonPath,
		const FName SourceBoneName,
		const FName TargetBoneName,
		const FName VirtualBoneName);

	/**
	 * 把 Lyra Linked Layer 的 GetMainAnimBPThreadSafe 返回类型和主 DynamicCast
	 * 一起迁移到同骨架的正式主 AnimBP。
	 *
	 * IK Retargeter 会复制 AnimBP 图和动画资产，却不会自动改写蓝图类类型引用；
	 * 若这里仍指向 Mannequin 主 AnimBP，运行时每帧 Cast 都会失败，整套移动、跳跃和 IK 数据随之失效。
	 * 本函数只接受同 Target Skeleton、图结构唯一明确的资产，任何歧义都拒绝修改。
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static bool RetargetMainAnimBlueprintType(
		const FString& ItemAnimBlueprintPath,
		const FString& MainAnimBlueprintPath);

	/**
	 * 对已经通过编辑器反射修改过嵌套 AnimGraph 节点的 AnimBlueprint 做一次结构化编译并保存。
	 *
	 * VibeUE ObjectTools 可以直接写入状态机过渡节点的 BlendProfileInterfaceWrapper，
	 * 但这类嵌套 UEdGraphNode 修改不会自动刷新 AnimBlueprintGeneratedClass 的烘焙数据；
	 * 只做普通编译时，旧 Skeleton 仍可能留在生成类的依赖表中。这个入口只负责刷新生成类，
	 * 不猜测或改变图节点内容。
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static bool ForceStructuralCompileAndSave(const FString& AnimBlueprintPath);

	/** 列出 Montage/Sequence 上的真实 Notify 类、时间和服务器触发设置。 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static TArray<FString> ListAnimNotifies(const FString& AnimationPath);

	/**
	 * 确保动画上存在且只存在一个指定类、指定时间的 Notify。
	 * 已有完全一致项时不改资产；同类重复或时间不一致时返回 false，由调用者先人工审计。
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static bool EnsureAnimNotify(
		const FString& AnimationPath,
		const FString& NotifyClassPath,
		float TriggerTime);

	/**
	 * 确保动画上存在且只存在一个指定名称、指定时间的 Skeleton Notify。
	 * Lyra Fire Montage 的 SaveAttack/ResetCombo 没有 Notify 对象，Python 公开 API 无法可靠复制，
	 * 因此由项目插件写入 FAnimNotifyEvent，并同步登记到目标 Skeleton；发现重复或时间不符时拒绝覆盖。
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static bool EnsureNamedAnimNotify(
		const FString& AnimationPath,
		const FName NotifyName,
		float TriggerTime);

	/**
	 * 列出 Montage 的 Slot、片段和动画引用。
	 * 每项格式为 "Slot索引|Slot名|片段索引|动画路径|起点|终点|播放倍率"，
	 * 用于在改正式资产前核对男女 Montage 的真实轨道，而不是依赖编辑器截图猜测。
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static TArray<FString> ListMontageSlotTracks(const FString& MontagePath);

	/**
	 * 从一个 Montage 中删除唯一匹配的 Slot 轨道。
	 * 仅当待删 Slot 精确命中一次、ExpectedRemainingSlot 精确命中一次且删除后仍有轨道时执行；
	 * 任一前提不成立都会拒绝修改，避免把 Equip Montage 的基础层和 Additive 层一起破坏。
	 */
	UFUNCTION(BlueprintCallable, Category = "AnimationAssetFixer")
	static bool RemoveExactMontageSlotTrack(
		const FString& MontagePath,
		const FName SlotName,
		const FName ExpectedRemainingSlot);
};
