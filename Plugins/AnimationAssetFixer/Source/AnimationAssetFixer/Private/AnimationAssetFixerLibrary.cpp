// Copyright Epic Games, Inc. All Rights Reserved.

#include "AnimationAssetFixerLibrary.h"
#include "AnimGraphNode_AssetPlayerBase.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "BlueprintEditorLibrary.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_EditablePinBase.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Variable.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/Skeleton.h"
#include "AnimationBlueprintLibrary.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

namespace
{
bool SaveAssetPackage(UObject* Asset)
{
	UPackage* Package = Asset ? Asset->GetPackage() : nullptr;
	if (!Package)
	{
		return false;
	}

	Package->MarkPackageDirty();
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	const FString Filename = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());
	return UPackage::SavePackage(Package, Asset, *Filename, SaveArgs);
}

void CompileAndSaveAnimBlueprint(UAnimBlueprint* AnimBlueprint)
{
	FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);
	UBlueprintEditorLibrary::CompileBlueprint(AnimBlueprint);
	SaveAssetPackage(AnimBlueprint);
}

void CompileAndSaveStructurallyModifiedAnimBlueprint(UAnimBlueprint* AnimBlueprint)
{
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	UBlueprintEditorLibrary::CompileBlueprint(AnimBlueprint);
	SaveAssetPackage(AnimBlueprint);
}

int32 RefreshLegacyMainAnimPropertyAccessPins(UAnimBlueprint* AnimBlueprint, UClass* MainAnimClass)
{
	if (!AnimBlueprint || !MainAnimClass)
	{
		return 0;
	}

	int32 RefreshedNodeCount = 0;
	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			// IK Retargeter 会复制 Property Access 的已解析输出类型。函数返回类型改成 CC 主 AnimBP 后，
			// 这些节点仍可能保留同名 Mannequin 类；UE 重建已连接 Pin 时又会刻意沿用旧类型，最终编译失败。
			if (!Node || Node->GetClass()->GetFName() != TEXT("K2Node_PropertyAccess"))
			{
				continue;
			}

			UEdGraphPin* ValuePin = Node->FindPin(TEXT("Value"), EGPD_Output);
			UClass* ResolvedClass = ValuePin
				? Cast<UClass>(ValuePin->PinType.PinSubCategoryObject.Get())
				: nullptr;
			if (!ValuePin || !ResolvedClass || ResolvedClass == MainAnimClass ||
				ResolvedClass->GetFName() != MainAnimClass->GetFName())
			{
				continue;
			}

			Node->Modify();
			ValuePin->Modify();
			ValuePin->PinType.PinCategory = UEdGraphSchema_K2::PC_Wildcard;
			ValuePin->PinType.PinSubCategory = NAME_None;
			ValuePin->PinType.PinSubCategoryObject = nullptr;
			Node->ReconstructNode();
			++RefreshedNodeCount;
		}
	}
	return RefreshedNodeCount;
}

TSet<UClass*> FindLegacyMainAnimClasses(UAnimBlueprint* AnimBlueprint, UClass* MainAnimClass)
{
	TSet<UClass*> Result;
	if (!AnimBlueprint || !MainAnimClass)
	{
		return Result;
	}

	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UClass* ReferencedClass = nullptr;
			if (const UK2Node_Variable* VariableNode = Cast<UK2Node_Variable>(Node))
			{
				ReferencedClass = VariableNode->VariableReference.GetMemberParentClass();
			}
			else if (const UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
			{
				ReferencedClass = CallNode->FunctionReference.GetMemberParentClass();
			}
			else if (Node && Node->GetClass()->GetFName() == TEXT("K2Node_PropertyAccess"))
			{
				const UEdGraphPin* ValuePin = Node->FindPin(TEXT("Value"), EGPD_Output);
				ReferencedClass = ValuePin
					? Cast<UClass>(ValuePin->PinType.PinSubCategoryObject.Get())
					: nullptr;
			}

			if (ReferencedClass && ReferencedClass != MainAnimClass &&
				ReferencedClass->GetFName() == MainAnimClass->GetFName())
			{
				Result.Add(ReferencedClass);
			}
		}
	}
	return Result;
}

int32 RetargetLegacyMainAnimMemberReferences(
	UAnimBlueprint* AnimBlueprint,
	const TSet<UClass*>& LegacyMainClasses,
	UClass* MainAnimClass)
{
	if (!AnimBlueprint || LegacyMainClasses.IsEmpty() || !MainAnimClass)
	{
		return 0;
	}

	int32 RetargetedNodeCount = 0;
	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UK2Node_Variable* VariableNode = Cast<UK2Node_Variable>(Node))
			{
				if (!LegacyMainClasses.Contains(VariableNode->VariableReference.GetMemberParentClass()))
				{
					continue;
				}
				FProperty* NewProperty = FindFProperty<FProperty>(MainAnimClass, VariableNode->VariableReference.GetMemberName());
				if (!NewProperty)
				{
					UE_LOG(LogTemp, Error,
						TEXT("[AnimationAssetFixer] CC 主 AnimBP 缺少变量: %s"),
						*VariableNode->VariableReference.GetMemberName().ToString());
					continue;
				}
				VariableNode->Modify();
				VariableNode->VariableReference.SetFromField<FProperty>(NewProperty, false);
				VariableNode->ReconstructNode();
				++RetargetedNodeCount;
			}
			else if (UK2Node_CallFunction* CallNode = Cast<UK2Node_CallFunction>(Node))
			{
				if (!LegacyMainClasses.Contains(CallNode->FunctionReference.GetMemberParentClass()))
				{
					continue;
				}
				UFunction* NewFunction = MainAnimClass->FindFunctionByName(CallNode->FunctionReference.GetMemberName());
				if (!NewFunction)
				{
					UE_LOG(LogTemp, Error,
						TEXT("[AnimationAssetFixer] CC 主 AnimBP 缺少函数: %s"),
						*CallNode->FunctionReference.GetMemberName().ToString());
					continue;
				}
				CallNode->Modify();
				CallNode->FunctionReference.SetFromField<UFunction>(NewFunction, false);
				CallNode->ReconstructNode();
				++RetargetedNodeCount;
			}
		}
	}
	return RetargetedNodeCount;
}
}

int32 UAnimationAssetFixerLibrary::ReplaceNodeAssetReferences(const FString& AnimBlueprintPath, const TArray<FString>& OldToNewPrefix, bool bCompileAndSave)
{
	// 解析 OldToNewPrefix：每项格式 "旧前缀|新前缀"
	TArray<TPair<FString, FString>> PrefixMap;
	for (const FString& Entry : OldToNewPrefix)
	{
		FString OldPrefix, NewPrefix;
		if (Entry.Split(TEXT("|"), &OldPrefix, &NewPrefix))
		{
			PrefixMap.Add(TPair<FString, FString>(OldPrefix, NewPrefix));
		}
	}

	UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintPath);
	if (!AnimBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载 ABP: %s"), *AnimBlueprintPath);
		return -1;
	}

	// 收集蓝图全部图（含 AnimGraph 及状态机/图层子图）
	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);

	int32 ChangedCount = 0;
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UAnimGraphNode_AssetPlayerBase* AssetPlayer = Cast<UAnimGraphNode_AssetPlayerBase>(Node);
			if (!AssetPlayer)
			{
				continue;
			}

			UAnimationAsset* CurrentAsset = AssetPlayer->GetAnimationAsset();
			if (!CurrentAsset)
			{
				continue;
			}

			const FString CurrentPath = CurrentAsset->GetPathName();
			UAnimationAsset* Replacement = nullptr;
			for (const auto& Pair : PrefixMap)
			{
				if (CurrentPath.StartsWith(Pair.Key))
				{
					const FString NewPath = Pair.Value + CurrentPath.RightChop(Pair.Key.Len());
					Replacement = LoadObject<UAnimationAsset>(nullptr, *NewPath);
					if (Replacement)
					{
						UE_LOG(LogTemp, Log, TEXT("[AnimationAssetFixer] %s: %s -> %s"),
							*Node->GetName(), *CurrentPath, *NewPath);
					}
					break;
				}
			}

			if (Replacement)
			{
				// RotationOffsetBlendSpace/SequenceEvaluator 可以把资产属性暴露成对象 Pin。
				// 只写 Node 结构会在编译时被 Pin 的旧 DefaultObject 覆盖，造成“内存替换成功、重启后回滚”。
				// 因此必须同时更新所有未连接、仍指向旧资产的输入 Pin。
				AssetPlayer->Modify();
				for (UEdGraphPin* Pin : AssetPlayer->Pins)
				{
					if (Pin && Pin->Direction == EGPD_Input && Pin->LinkedTo.IsEmpty() && Pin->DefaultObject == CurrentAsset)
					{
						Pin->Modify();
						Pin->DefaultObject = Replacement;
					}
				}
				TMap<UAnimationAsset*, UAnimationAsset*> ReplacementMap;
				ReplacementMap.Add(CurrentAsset, Replacement);
				AssetPlayer->ReplaceReferredAnimations(ReplacementMap);
				AssetPlayer->SetAnimationAsset(Replacement);
				ChangedCount++;
			}
		}
	}

	if (ChangedCount > 0)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(AnimBlueprint);
	}

	if (ChangedCount > 0 && bCompileAndSave)
	{
		CompileAndSaveAnimBlueprint(AnimBlueprint);
	}

	UE_LOG(LogTemp, Log, TEXT("[AnimationAssetFixer] %s: 替换 %d 个节点"), *AnimBlueprintPath, ChangedCount);
	return ChangedCount;
}

TArray<FString> UAnimationAssetFixerLibrary::ListNodeAssetReferences(const FString& AnimBlueprintPath)
{
	TArray<FString> Result;

	UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintPath);
	if (!AnimBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载 ABP: %s"), *AnimBlueprintPath);
		return Result;
	}

	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UAnimGraphNode_AssetPlayerBase* AssetPlayer = Cast<UAnimGraphNode_AssetPlayerBase>(Node);
			if (!AssetPlayer)
			{
				continue;
			}
			UAnimationAsset* Asset = AssetPlayer->GetAnimationAsset();
			if (!Asset)
			{
				continue;
			}
			Result.Add(FString::Printf(TEXT("%s|%s|%s"), *Graph->GetName(), *Node->GetClass()->GetName(), *Asset->GetPathName()));
		}
	}
	return Result;
}

TArray<FString> UAnimationAssetFixerLibrary::ListGraphNodes(const FString& AnimBlueprintPath, const FString& GraphNameContains)
{
	TArray<FString> Result;
	UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintPath);
	if (!AnimBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载 ABP: %s"), *AnimBlueprintPath);
		return Result;
	}

	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph || (!GraphNameContains.IsEmpty() && !Graph->GetName().Contains(GraphNameContains)))
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			TArray<FString> PinDescriptions;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (!Pin)
				{
					continue;
				}
				TArray<FString> Links;
				for (const UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					if (LinkedPin && LinkedPin->GetOwningNode())
					{
						Links.Add(FString::Printf(TEXT("%s.%s"), *LinkedPin->GetOwningNode()->GetName(), *LinkedPin->GetName()));
					}
				}
				PinDescriptions.Add(FString::Printf(
					TEXT("%s:%s=%s->%s"),
					Pin->Direction == EGPD_Input ? TEXT("In") : TEXT("Out"),
					*Pin->GetName(),
					*Pin->DefaultValue,
					*FString::Join(Links, TEXT(","))));
			}

			Result.Add(FString::Printf(
				TEXT("%s|%s|%s|%s|%s"),
				*Graph->GetName(),
				*Node->GetName(),
				*Node->GetClass()->GetName(),
				*Node->NodeGuid.ToString(),
				*FString::Join(PinDescriptions, TEXT(";"))));
		}
	}
	return Result;
}

TArray<FString> UAnimationAssetFixerLibrary::ListGraphNodeRuntimeProperties(
	const FString& AnimBlueprintPath,
	const FString& GraphNameContains,
	const FString& NodeClassNameContains)
{
	TArray<FString> Result;
	UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintPath);
	if (!AnimBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载 ABP: %s"), *AnimBlueprintPath);
		return Result;
	}

	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph || (!GraphNameContains.IsEmpty() && !Graph->GetName().Contains(GraphNameContains)))
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node || (!NodeClassNameContains.IsEmpty() && !Node->GetClass()->GetName().Contains(NodeClassNameContains)))
			{
				continue;
			}

			FStructProperty* RuntimeNodeProperty = FindFProperty<FStructProperty>(Node->GetClass(), TEXT("Node"));
			if (!RuntimeNodeProperty)
			{
				continue;
			}

			FString RuntimeNodeText;
			void* RuntimeNode = RuntimeNodeProperty->ContainerPtrToValuePtr<void>(Node);
			RuntimeNodeProperty->ExportTextItem_Direct(
				RuntimeNodeText, RuntimeNode, nullptr, Node, PPF_None);
			Result.Add(FString::Printf(
				TEXT("%s|%s|%s|%s"),
				*Graph->GetName(),
				*Node->GetName(),
				*Node->GetClass()->GetName(),
				*RuntimeNodeText));
		}
	}
	return Result;
}

bool UAnimationAssetFixerLibrary::RebuildLinkedAssetFallback(
	const FString& AnimBlueprintPath,
	const FString& GraphName,
	const FString& NodeClassNameContains,
	const FName AssetPinName,
	const FString& AnimationAssetPath)
{
	UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintPath);
	UAnimationAsset* Replacement = LoadObject<UAnimationAsset>(nullptr, *AnimationAssetPath);
	if (!AnimBlueprint || !Replacement)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] AnimBlueprint 或替换动画无效: %s | %s"),
			*AnimBlueprintPath, *AnimationAssetPath);
		return false;
	}

	UEdGraph* TargetGraph = nullptr;
	UAnimGraphNode_AssetPlayerBase* TargetNode = nullptr;
	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph || Graph->GetName() != GraphName)
		{
			continue;
		}
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UAnimGraphNode_AssetPlayerBase* Candidate = Cast<UAnimGraphNode_AssetPlayerBase>(Node);
			if (Candidate && Node->GetClass()->GetName().Contains(NodeClassNameContains))
			{
				if (TargetNode)
				{
					UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] %s/%s 匹配到多个节点，拒绝修改"),
						*GraphName, *NodeClassNameContains);
					return false;
				}
				TargetGraph = Graph;
				TargetNode = Candidate;
			}
		}
	}

	UEdGraphPin* AssetPin = TargetNode ? TargetNode->FindPin(AssetPinName) : nullptr;
	if (!TargetGraph || !TargetNode || !AssetPin || AssetPin->Direction != EGPD_Input || AssetPin->LinkedTo.Num() != 1)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s/%s/%s 必须精确匹配一个节点和一个输入连接"),
			*GraphName, *NodeClassNameContains, *AssetPinName.ToString());
		return false;
	}

	UEdGraphPin* OriginalLinkedPin = AssetPin->LinkedTo[0];
	UEdGraphNode* OriginalLinkedNode = OriginalLinkedPin ? OriginalLinkedPin->GetOwningNode() : nullptr;
	if (!OriginalLinkedNode)
	{
		return false;
	}
	const FGuid TargetNodeGuid = TargetNode->NodeGuid;
	const FGuid LinkedNodeGuid = OriginalLinkedNode->NodeGuid;
	const FName LinkedPinName = OriginalLinkedPin->PinName;

	TargetNode->Modify();
	AssetPin->Modify();
	AssetPin->BreakAllPinLinks();
	AssetPin->DefaultObject = Replacement;
	TargetNode->SetAnimationAsset(Replacement);
	CompileAndSaveStructurallyModifiedAnimBlueprint(AnimBlueprint);

	// 结构性编译后按 GUID 重新定位节点和 Pin，禁止继续使用可能失效的旧指针。
	TargetGraph = nullptr;
	TargetNode = nullptr;
	UEdGraphNode* LinkedNode = nullptr;
	AllGraphs.Reset();
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph || Graph->GetName() != GraphName)
		{
			continue;
		}
		TargetGraph = Graph;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (Node && Node->NodeGuid == TargetNodeGuid)
			{
				TargetNode = Cast<UAnimGraphNode_AssetPlayerBase>(Node);
			}
			if (Node && Node->NodeGuid == LinkedNodeGuid)
			{
				LinkedNode = Node;
			}
		}
	}

	AssetPin = TargetNode ? TargetNode->FindPin(AssetPinName) : nullptr;
	UEdGraphPin* LinkedPin = LinkedNode ? LinkedNode->FindPin(LinkedPinName) : nullptr;
	const UEdGraphSchema* Schema = TargetGraph ? TargetGraph->GetSchema() : nullptr;
	if (!AssetPin || !LinkedPin || !Schema || !Schema->TryCreateConnection(LinkedPin, AssetPin))
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 恢复 %s.%s -> %s.%s 连接失败"),
			*GetNameSafe(LinkedNode), *LinkedPinName.ToString(), *GetNameSafe(TargetNode), *AssetPinName.ToString());
		return false;
	}

	TargetNode->Modify();
	TargetNode->SetAnimationAsset(Replacement);
	CompileAndSaveStructurallyModifiedAnimBlueprint(AnimBlueprint);
	return AssetPin->LinkedTo.Num() == 1;
}

int32 UAnimationAssetFixerLibrary::SetSkeletalControlAlphas(
	const FString& AnimBlueprintPath,
	const FString& GraphNameContains,
	const FString& NodeClassNameContains,
	float Alpha,
	bool bCompileAndSave)
{
	UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintPath);
	if (!AnimBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载 ABP: %s"), *AnimBlueprintPath);
		return -1;
	}

	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	int32 ChangedCount = 0;
	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph || (!GraphNameContains.IsEmpty() && !Graph->GetName().Contains(GraphNameContains)))
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UAnimGraphNode_SkeletalControlBase* SkeletalControl = Cast<UAnimGraphNode_SkeletalControlBase>(Node);
			if (!SkeletalControl || (!NodeClassNameContains.IsEmpty() && !Node->GetClass()->GetName().Contains(NodeClassNameContains)))
			{
				continue;
			}

			FStructProperty* RuntimeNodeProperty = FindFProperty<FStructProperty>(Node->GetClass(), TEXT("Node"));
			FFloatProperty* AlphaProperty = RuntimeNodeProperty
				? FindFProperty<FFloatProperty>(RuntimeNodeProperty->Struct, TEXT("Alpha"))
				: nullptr;
			if (!RuntimeNodeProperty || !AlphaProperty)
			{
				UE_LOG(LogTemp, Warning, TEXT("[AnimationAssetFixer] %s 找不到运行时 Alpha 属性"), *Node->GetName());
				continue;
			}

			Node->Modify();
			void* RuntimeNode = RuntimeNodeProperty->ContainerPtrToValuePtr<void>(Node);
			AlphaProperty->SetPropertyValue_InContainer(RuntimeNode, ClampedAlpha);

			// Alpha 暴露成图 Pin 时，编译器会使用 Pin 默认值覆盖结构体值；两处必须同步。
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Input && Pin->PinName == TEXT("Alpha") && Pin->LinkedTo.IsEmpty())
				{
					Pin->Modify();
					Pin->DefaultValue = FString::SanitizeFloat(ClampedAlpha);
				}
			}
			ChangedCount++;
		}
	}

	if (ChangedCount > 0 && bCompileAndSave)
	{
		CompileAndSaveAnimBlueprint(AnimBlueprint);
	}
	UE_LOG(LogTemp, Log, TEXT("[AnimationAssetFixer] %s: 设置 %d 个 SkeletalControl Alpha=%g"), *AnimBlueprintPath, ChangedCount, ClampedAlpha);
	return ChangedCount;
}

bool UAnimationAssetFixerLibrary::SetSequencePlayerLooping(
	const FString& AnimBlueprintPath,
	const FString& GraphName,
	const FString& NodeName,
	bool bLoopAnimation,
	bool bCompileAndSave)
{
	UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintPath);
	if (!AnimBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载 ABP: %s"), *AnimBlueprintPath);
		return false;
	}

	UEdGraphNode* TargetNode = nullptr;
	TArray<UEdGraph*> AllGraphs;
	AnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph || Graph->GetName() != GraphName)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node || Node->GetName() != NodeName ||
				!Node->GetClass()->GetName().Contains(TEXT("SequencePlayer")))
			{
				continue;
			}

			if (TargetNode)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[AnimationAssetFixer] %s.%s 中节点名 %s 不唯一"),
					*AnimBlueprintPath, *GraphName, *NodeName);
				return false;
			}
			TargetNode = Node;
		}
	}

	if (!TargetNode)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s.%s 找不到 Sequence Player %s"),
			*AnimBlueprintPath, *GraphName, *NodeName);
		return false;
	}

	FStructProperty* RuntimeNodeProperty = FindFProperty<FStructProperty>(TargetNode->GetClass(), TEXT("Node"));
	FBoolProperty* LoopProperty = RuntimeNodeProperty
		? FindFProperty<FBoolProperty>(RuntimeNodeProperty->Struct, TEXT("bLoopAnimation"))
		: nullptr;
	if (!RuntimeNodeProperty || !LoopProperty)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s.%s.%s 找不到 bLoopAnimation"),
			*AnimBlueprintPath, *GraphName, *NodeName);
		return false;
	}

	TargetNode->Modify();
	void* RuntimeNode = RuntimeNodeProperty->ContainerPtrToValuePtr<void>(TargetNode);
	LoopProperty->SetPropertyValue_InContainer(RuntimeNode, bLoopAnimation);

	// 若未来该属性暴露为 Pin，编译器会用 Pin 默认值覆盖 Node 结构；两处保持一致。
	for (UEdGraphPin* Pin : TargetNode->Pins)
	{
		if (Pin && Pin->Direction == EGPD_Input && Pin->LinkedTo.IsEmpty() &&
			(Pin->PinName == TEXT("bLoopAnimation") || Pin->PinName == TEXT("LoopAnimation")))
		{
			Pin->Modify();
			Pin->DefaultValue = bLoopAnimation ? TEXT("true") : TEXT("false");
		}
	}

	if (bCompileAndSave)
	{
		CompileAndSaveAnimBlueprint(AnimBlueprint);
	}

	UE_LOG(LogTemp, Log,
		TEXT("[AnimationAssetFixer] %s.%s.%s bLoopAnimation=%s"),
		*AnimBlueprintPath, *GraphName, *NodeName, bLoopAnimation ? TEXT("true") : TEXT("false"));
	return true;
}

TArray<FString> UAnimationAssetFixerLibrary::ListVirtualBones(const FString& SkeletonPath)
{
	TArray<FString> Result;
	const USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, *SkeletonPath);
	if (!Skeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载 Skeleton: %s"), *SkeletonPath);
		return Result;
	}

	for (const FVirtualBone& VirtualBone : Skeleton->GetVirtualBones())
	{
		Result.Add(FString::Printf(
			TEXT("%s|%s|%s"),
			*VirtualBone.VirtualBoneName.ToString(),
			*VirtualBone.SourceBoneName.ToString(),
			*VirtualBone.TargetBoneName.ToString()));
	}
	return Result;
}

bool UAnimationAssetFixerLibrary::EnsureNamedVirtualBone(
	const FString& SkeletonPath,
	const FName SourceBoneName,
	const FName TargetBoneName,
	const FName VirtualBoneName)
{
	USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, *SkeletonPath);
	if (!Skeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载 Skeleton: %s"), *SkeletonPath);
		return false;
	}

	for (const FVirtualBone& Existing : Skeleton->GetVirtualBones())
	{
		if (Existing.VirtualBoneName == VirtualBoneName)
		{
			const bool bExactMatch = Existing.SourceBoneName == SourceBoneName && Existing.TargetBoneName == TargetBoneName;
			if (!bExactMatch)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[AnimationAssetFixer] %s 已存在，但映射为 %s -> %s；拒绝覆盖为 %s -> %s"),
					*VirtualBoneName.ToString(),
					*Existing.SourceBoneName.ToString(),
					*Existing.TargetBoneName.ToString(),
					*SourceBoneName.ToString(),
					*TargetBoneName.ToString());
			}
			return bExactMatch;
		}
	}

	const FReferenceSkeleton& ReferenceSkeleton = Skeleton->GetReferenceSkeleton();
	if (ReferenceSkeleton.FindBoneIndex(SourceBoneName) == INDEX_NONE || ReferenceSkeleton.FindBoneIndex(TargetBoneName) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] 无法创建 %s：Source=%s 或 Target=%s 不在 Skeleton 中"),
			*VirtualBoneName.ToString(), *SourceBoneName.ToString(), *TargetBoneName.ToString());
		return false;
	}

	if (!Skeleton->AddNewNamedVirtualBone(SourceBoneName, TargetBoneName, VirtualBoneName))
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 添加虚拟骨骼失败: %s"), *VirtualBoneName.ToString());
		return false;
	}
	return SaveAssetPackage(Skeleton);
}

bool UAnimationAssetFixerLibrary::RetargetMainAnimBlueprintType(
	const FString& ItemAnimBlueprintPath,
	const FString& MainAnimBlueprintPath)
{
	UAnimBlueprint* ItemAnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *ItemAnimBlueprintPath);
	UAnimBlueprint* MainAnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *MainAnimBlueprintPath);
	if (!ItemAnimBlueprint || !MainAnimBlueprint || !MainAnimBlueprint->GeneratedClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] Linked Layer 或主 AnimBP 无效: %s | %s"),
			*ItemAnimBlueprintPath, *MainAnimBlueprintPath);
		return false;
	}

	// 男女正式层必须返回同一目标骨架的主 AnimBP。跨骨架强改类型只会把编译错误推迟到运行时。
	if (!ItemAnimBlueprint->TargetSkeleton ||
		ItemAnimBlueprint->TargetSkeleton != MainAnimBlueprint->TargetSkeleton)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] Target Skeleton 不一致，拒绝迁移主 AnimBP 类型: %s | %s"),
			*GetPathNameSafe(ItemAnimBlueprint->TargetSkeleton),
			*GetPathNameSafe(MainAnimBlueprint->TargetSkeleton));
		return false;
	}

	UEdGraph* FunctionGraph = nullptr;
	TArray<UEdGraph*> AllGraphs;
	ItemAnimBlueprint->GetAllGraphs(AllGraphs);
	for (UEdGraph* Graph : AllGraphs)
	{
		if (Graph && Graph->GetName() == TEXT("GetMainAnimBPThreadSafe"))
		{
			if (FunctionGraph)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[AnimationAssetFixer] %s 存在多个 GetMainAnimBPThreadSafe 图，拒绝猜测。"),
					*ItemAnimBlueprintPath);
				return false;
			}
			FunctionGraph = Graph;
		}
	}
	if (!FunctionGraph)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s 缺少 GetMainAnimBPThreadSafe。"),
			*ItemAnimBlueprintPath);
		return false;
	}

	TArray<UK2Node_FunctionResult*> ResultNodes;
	FunctionGraph->GetNodesOfClass(ResultNodes);
	UClass* OldReturnClass = nullptr;
	UK2Node_FunctionResult* SuccessResultNode = nullptr;
	for (UK2Node_FunctionResult* ResultNode : ResultNodes)
	{
		UEdGraphPin* ReturnPin = ResultNode ? ResultNode->FindPin(TEXT("ReturnValue"), EGPD_Input) : nullptr;
		UClass* ReturnClass = ReturnPin
			? Cast<UClass>(ReturnPin->PinType.PinSubCategoryObject.Get())
			: nullptr;
		if (!ReturnPin || !ReturnClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[AnimationAssetFixer] %s 的 Result 节点缺少对象 ReturnValue。"),
				*ItemAnimBlueprintPath);
			return false;
		}
		if (OldReturnClass && OldReturnClass != ReturnClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[AnimationAssetFixer] %s 的多个 Result 返回类型不一致，拒绝修改。"),
				*ItemAnimBlueprintPath);
			return false;
		}
		OldReturnClass = ReturnClass;
		if (!ReturnPin->LinkedTo.IsEmpty())
		{
			if (SuccessResultNode || ReturnPin->LinkedTo.Num() != 1)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[AnimationAssetFixer] %s 的成功返回连线不唯一，拒绝修改。"),
					*ItemAnimBlueprintPath);
				return false;
			}
			SuccessResultNode = ResultNode;
		}
	}

	UClass* NewReturnClass = MainAnimBlueprint->GeneratedClass;
	const TSet<UClass*> LegacyMainClasses = FindLegacyMainAnimClasses(ItemAnimBlueprint, NewReturnClass);
	TArray<UK2Node_DynamicCast*> CastNodes;
	FunctionGraph->GetNodesOfClass(CastNodes);
	UK2Node_DynamicCast* MainCastNode = nullptr;
	for (UK2Node_DynamicCast* CastNode : CastNodes)
	{
		// 允许从上次中断留下的“Result 仍旧、Cast 已新”状态继续，保证工具可断点重入。
		if (CastNode && (CastNode->TargetType == OldReturnClass || CastNode->TargetType == NewReturnClass))
		{
			if (MainCastNode)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[AnimationAssetFixer] %s 存在多个指向旧主 AnimBP 的 Cast，拒绝修改。"),
					*ItemAnimBlueprintPath);
				return false;
			}
			MainCastNode = CastNode;
		}
	}
	if (!OldReturnClass || !SuccessResultNode || !MainCastNode)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s 的主 Cast/成功 Result 拓扑不完整。"),
			*ItemAnimBlueprintPath);
		return false;
	}

	if (OldReturnClass == NewReturnClass && MainCastNode->TargetType == NewReturnClass)
	{
		const int32 RetargetedMemberCount = RetargetLegacyMainAnimMemberReferences(
			ItemAnimBlueprint, LegacyMainClasses, NewReturnClass);
		const int32 RefreshedNodeCount = RefreshLegacyMainAnimPropertyAccessPins(ItemAnimBlueprint, NewReturnClass);
		if (RetargetedMemberCount > 0 || RefreshedNodeCount > 0 || ItemAnimBlueprint->Status != BS_UpToDate)
		{
			FBlueprintEditorUtils::RefreshAllNodes(ItemAnimBlueprint);
			CompileAndSaveStructurallyModifiedAnimBlueprint(ItemAnimBlueprint);
		}
		UE_LOG(LogTemp, Log,
			TEXT("[AnimationAssetFixer] %s: 迁移主 AnimBP 成员引用=%d，刷新 Property Access=%d"),
			*ItemAnimBlueprintPath, RetargetedMemberCount, RefreshedNodeCount);
		return ItemAnimBlueprint->Status == BS_UpToDate;
	}

	// 先改所有 Result Pin 的签名，再重建 Cast；这样重建后的新类型输出可以按原 Pin 名恢复连线。
	for (UK2Node_FunctionResult* ResultNode : ResultNodes)
	{
		ResultNode->Modify();
		bool bUpdatedUserPin = false;
		for (const TSharedPtr<FUserPinInfo>& UserPin : ResultNode->UserDefinedPins)
		{
			if (UserPin.IsValid() && UserPin->PinName == TEXT("ReturnValue"))
			{
				UserPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
				UserPin->PinType.PinSubCategory = NAME_None;
				UserPin->PinType.PinSubCategoryObject = NewReturnClass;
				bUpdatedUserPin = true;
			}
		}
		if (!bUpdatedUserPin)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[AnimationAssetFixer] %s 的 Result 节点缺少 ReturnValue UserDefinedPin。"),
				*ItemAnimBlueprintPath);
			return false;
		}
		UEdGraphPin* ReturnPin = ResultNode->FindPin(TEXT("ReturnValue"), EGPD_Input);
		ReturnPin->Modify();
		ReturnPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
		ReturnPin->PinType.PinSubCategory = NAME_None;
		ReturnPin->PinType.PinSubCategoryObject = NewReturnClass;
	}

	MainCastNode->Modify();
	MainCastNode->TargetType = NewReturnClass;
	MainCastNode->ReconstructNode();
	for (UK2Node_FunctionResult* ResultNode : ResultNodes)
	{
		ResultNode->ReconstructNode();
	}

	const int32 RetargetedMemberCount = RetargetLegacyMainAnimMemberReferences(
		ItemAnimBlueprint, LegacyMainClasses, NewReturnClass);
	// 函数签名改变后，刷新本蓝图内所有 GetMainAnimBPThreadSafe 调用点；否则调用节点仍保留旧返回类。
	FBlueprintEditorUtils::RefreshAllNodes(ItemAnimBlueprint);
	const int32 RefreshedNodeCount = RefreshLegacyMainAnimPropertyAccessPins(ItemAnimBlueprint, NewReturnClass);

	UEdGraphPin* CastResultPin = MainCastNode->GetCastResultPin();
	UEdGraphPin* SuccessReturnPin = SuccessResultNode->FindPin(TEXT("ReturnValue"), EGPD_Input);
	const UEdGraphSchema* Schema = FunctionGraph->GetSchema();
	if (!CastResultPin || !SuccessReturnPin || !Schema)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s 重建主 Cast 后缺少返回 Pin。"),
			*ItemAnimBlueprintPath);
		return false;
	}
	if (!SuccessReturnPin->LinkedTo.Contains(CastResultPin) &&
		!Schema->TryCreateConnection(CastResultPin, SuccessReturnPin))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s 无法恢复主 Cast 到 ReturnValue 的连接。"),
			*ItemAnimBlueprintPath);
		return false;
	}

	CompileAndSaveStructurallyModifiedAnimBlueprint(ItemAnimBlueprint);
	UE_LOG(LogTemp, Log,
		TEXT("[AnimationAssetFixer] %s: GetMainAnimBPThreadSafe %s -> %s，迁移成员引用=%d，刷新 Property Access=%d"),
		*ItemAnimBlueprintPath, *GetPathNameSafe(OldReturnClass), *GetPathNameSafe(NewReturnClass),
		RetargetedMemberCount, RefreshedNodeCount);
	return ItemAnimBlueprint->Status == BS_UpToDate;
}

bool UAnimationAssetFixerLibrary::ForceStructuralCompileAndSave(const FString& AnimBlueprintPath)
{
	UAnimBlueprint* AnimBlueprint = LoadObject<UAnimBlueprint>(nullptr, *AnimBlueprintPath);
	if (!AnimBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载需要结构化编译的 ABP: %s"), *AnimBlueprintPath);
		return false;
	}

	// 通过 ObjectTools 修改嵌套状态机节点时，节点字段已经更新，但生成类的烘焙缓存未必失效。
	// 这里明确走结构化编译，确保 AnimBlueprintGeneratedClass 不再保留旧 Skeleton/Profile 依赖。
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
	const bool bCompiled = UBlueprintEditorLibrary::CompileBlueprint(AnimBlueprint);
	const bool bSaved = bCompiled && SaveAssetPackage(AnimBlueprint);
	const bool bUpToDate = AnimBlueprint->Status == BS_UpToDate;
	UE_LOG(LogTemp, Log,
		TEXT("[AnimationAssetFixer] %s: 结构化编译=%s 保存=%s 状态=%s"),
		*AnimBlueprintPath,
		bCompiled ? TEXT("成功") : TEXT("失败"),
		bSaved ? TEXT("成功") : TEXT("失败"),
		bUpToDate ? TEXT("UpToDate") : TEXT("非 UpToDate"));
	return bCompiled && bSaved && bUpToDate;
}

TArray<FString> UAnimationAssetFixerLibrary::ListAnimNotifies(const FString& AnimationPath)
{
	TArray<FString> Result;
	const UAnimSequenceBase* Animation = LoadObject<UAnimSequenceBase>(nullptr, *AnimationPath);
	if (!Animation)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载动画: %s"), *AnimationPath);
		return Result;
	}

	TArray<FAnimNotifyEvent> Events;
	UAnimationBlueprintLibrary::GetAnimationNotifyEvents(Animation, Events);
	for (int32 Index = 0; Index < Events.Num(); ++Index)
	{
		const FAnimNotifyEvent& Event = Events[Index];
		const FString NotifyIdentity = Event.Notify
			? GetPathNameSafe(Event.Notify->GetClass())
			: (!Event.NotifyName.IsNone() ? FString::Printf(TEXT("SkeletonNotify:%s"), *Event.NotifyName.ToString()) : TEXT("None"));
		Result.Add(FString::Printf(
			TEXT("%d|%s|%.6f|DedicatedServer=%d"),
			Index,
			*NotifyIdentity,
			UAnimationBlueprintLibrary::GetAnimNotifyEventTriggerTime(Event),
			Event.bTriggerOnDedicatedServer));
	}
	return Result;
}

bool UAnimationAssetFixerLibrary::EnsureAnimNotify(
	const FString& AnimationPath,
	const FString& NotifyClassPath,
	float TriggerTime)
{
	UAnimSequenceBase* Animation = LoadObject<UAnimSequenceBase>(nullptr, *AnimationPath);
	UClass* NotifyClass = LoadClass<UAnimNotify>(nullptr, *NotifyClassPath);
	if (!Animation || !NotifyClass || !NotifyClass->IsChildOf(UAnimNotify::StaticClass()))
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 动画或 Notify 类无效: %s | %s"),
			*AnimationPath, *NotifyClassPath);
		return false;
	}

	TArray<FAnimNotifyEvent> ExistingEvents;
	UAnimationBlueprintLibrary::GetAnimationNotifyEvents(Animation, ExistingEvents);
	int32 MatchingClassCount = 0;
	bool bHasExactMatch = false;
	for (const FAnimNotifyEvent& Event : ExistingEvents)
	{
		if (Event.Notify && Event.Notify->GetClass() == NotifyClass)
		{
			++MatchingClassCount;
			bHasExactMatch |= FMath::IsNearlyEqual(
				UAnimationBlueprintLibrary::GetAnimNotifyEventTriggerTime(Event), TriggerTime, 0.0001f);
		}
	}

	if (MatchingClassCount == 1 && bHasExactMatch)
	{
		return true;
	}
	if (MatchingClassCount > 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s 已有 %d 个同类 Notify，但时间或数量不符；拒绝自动覆盖。"),
			*AnimationPath, MatchingClassCount);
		return false;
	}

	TArray<FName> TrackNames;
	UAnimationBlueprintLibrary::GetAnimationNotifyTrackNames(Animation, TrackNames);
	if (TrackNames.IsEmpty())
	{
		UAnimationBlueprintLibrary::AddAnimationNotifyTrack(Animation, TEXT("1"));
		TrackNames.Add(TEXT("1"));
	}

	Animation->Modify();
	UAnimNotify* AddedNotify = UAnimationBlueprintLibrary::AddAnimationNotifyEvent(
		Animation, TrackNames[0], TriggerTime, NotifyClass);
	if (!AddedNotify)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 添加 Notify 失败: %s"), *AnimationPath);
		return false;
	}

	return SaveAssetPackage(Animation);
}

bool UAnimationAssetFixerLibrary::EnsureNamedAnimNotify(
	const FString& AnimationPath,
	const FName NotifyName,
	float TriggerTime)
{
	UAnimSequenceBase* Animation = LoadObject<UAnimSequenceBase>(nullptr, *AnimationPath);
	if (!Animation || NotifyName.IsNone() || TriggerTime < 0.0f || TriggerTime > Animation->GetPlayLength())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] 动画、Skeleton Notify 名或时间无效: %s | %s | %.6f"),
			*AnimationPath, *NotifyName.ToString(), TriggerTime);
		return false;
	}

	int32 MatchingNameCount = 0;
	bool bHasExactMatch = false;
	for (const FAnimNotifyEvent& Event : Animation->Notifies)
	{
		// Named Skeleton Notify 不携带 Notify/NotifyState 对象；只按名称匹配，避免误认自定义 Notify 类。
		if (!Event.Notify && !Event.NotifyStateClass && Event.NotifyName == NotifyName)
		{
			++MatchingNameCount;
			bHasExactMatch |= FMath::IsNearlyEqual(
				UAnimationBlueprintLibrary::GetAnimNotifyEventTriggerTime(Event), TriggerTime, 0.0001f);
		}
	}

	if (MatchingNameCount == 1 && bHasExactMatch)
	{
		return true;
	}
	if (MatchingNameCount > 0)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s 已有 %d 个名为 %s 的 Skeleton Notify，但时间或数量不符；拒绝自动覆盖。"),
			*AnimationPath, MatchingNameCount, *NotifyName.ToString());
		return false;
	}

	TArray<FName> TrackNames;
	UAnimationBlueprintLibrary::GetAnimationNotifyTrackNames(Animation, TrackNames);
	if (TrackNames.IsEmpty())
	{
		UAnimationBlueprintLibrary::AddAnimationNotifyTrack(Animation, TEXT("1"));
		UAnimationBlueprintLibrary::GetAnimationNotifyTrackNames(Animation, TrackNames);
	}
	if (TrackNames.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法为 %s 创建 Notify Track。"), *AnimationPath);
		return false;
	}

	int32 TrackIndex = INDEX_NONE;
	for (int32 Index = 0; Index < Animation->AnimNotifyTracks.Num(); ++Index)
	{
		if (Animation->AnimNotifyTracks[Index].TrackName == TrackNames[0])
		{
			TrackIndex = Index;
			break;
		}
	}
	if (TrackIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] %s 的首个 Notify Track 无有效索引。"), *AnimationPath);
		return false;
	}

	USkeleton* Skeleton = Animation->GetSkeleton();
	if (!Skeleton)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] %s 没有目标 Skeleton。"), *AnimationPath);
		return false;
	}

	if (!Skeleton->AnimationNotifies.Contains(NotifyName))
	{
		Skeleton->Modify();
		Skeleton->AddNewAnimationNotify(NotifyName);
		if (!SaveAssetPackage(Skeleton))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[AnimationAssetFixer] 无法在 %s 登记 Skeleton Notify %s。"),
				*GetPathNameSafe(Skeleton), *NotifyName.ToString());
			return false;
		}
	}

	Animation->Modify();
	FAnimNotifyEvent& NewEvent = Animation->Notifies.AddDefaulted_GetRef();
	NewEvent.NotifyName = NotifyName;
	NewEvent.Link(Animation, TriggerTime);
	NewEvent.TriggerTimeOffset = GetTriggerTimeOffsetForType(Animation->CalculateOffsetForNotify(TriggerTime));
	NewEvent.TrackIndex = TrackIndex;
	NewEvent.Notify = nullptr;
	NewEvent.NotifyStateClass = nullptr;
	NewEvent.Guid = FGuid::NewGuid();
	NewEvent.bTriggerOnDedicatedServer = true;

	// 保持时间线顺序稳定，便于后续审计，也避免编辑器打开 Montage 时显示顺序随机。
	Animation->Notifies.StableSort([](const FAnimNotifyEvent& Left, const FAnimNotifyEvent& Right)
	{
		return UAnimationBlueprintLibrary::GetAnimNotifyEventTriggerTime(Left) <
			UAnimationBlueprintLibrary::GetAnimNotifyEventTriggerTime(Right);
	});

#if WITH_EDITOR
	if (UAnimMontage* Montage = Cast<UAnimMontage>(Animation))
	{
		Montage->UpdateLinkableElements();
	}
	Animation->PostEditChange();
#endif
	Animation->RefreshCacheData();

	const bool bSaved = SaveAssetPackage(Animation);
	UE_LOG(LogTemp, Log,
		TEXT("[AnimationAssetFixer] %s 添加 Skeleton Notify %s @ %.6f，保存=%d"),
		*AnimationPath, *NotifyName.ToString(), TriggerTime, bSaved);
	return bSaved;
}

TArray<FString> UAnimationAssetFixerLibrary::ListMontageSlotTracks(const FString& MontagePath)
{
	TArray<FString> Result;
	const UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *MontagePath);
	if (!Montage)
	{
		UE_LOG(LogTemp, Error, TEXT("[AnimationAssetFixer] 无法加载 Montage: %s"), *MontagePath);
		return Result;
	}

	for (int32 SlotIndex = 0; SlotIndex < Montage->SlotAnimTracks.Num(); ++SlotIndex)
	{
		const FSlotAnimationTrack& SlotTrack = Montage->SlotAnimTracks[SlotIndex];
		if (SlotTrack.AnimTrack.AnimSegments.IsEmpty())
		{
			Result.Add(FString::Printf(TEXT("%d|%s|-1|None|0|0|0"),
				SlotIndex, *SlotTrack.SlotName.ToString()));
			continue;
		}

		for (int32 SegmentIndex = 0; SegmentIndex < SlotTrack.AnimTrack.AnimSegments.Num(); ++SegmentIndex)
		{
			const FAnimSegment& Segment = SlotTrack.AnimTrack.AnimSegments[SegmentIndex];
			Result.Add(FString::Printf(TEXT("%d|%s|%d|%s|%.6f|%.6f|%.6f"),
				SlotIndex,
				*SlotTrack.SlotName.ToString(),
				SegmentIndex,
				*GetPathNameSafe(Segment.GetAnimReference()),
				Segment.StartPos,
				Segment.GetEndPos(),
				Segment.AnimPlayRate));
		}
	}
	return Result;
}

bool UAnimationAssetFixerLibrary::RemoveExactMontageSlotTrack(
	const FString& MontagePath,
	const FName SlotName,
	const FName ExpectedRemainingSlot)
{
	UAnimMontage* Montage = LoadObject<UAnimMontage>(nullptr, *MontagePath);
	if (!Montage || SlotName.IsNone() || ExpectedRemainingSlot.IsNone() || SlotName == ExpectedRemainingSlot)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] Montage 或 Slot 参数无效: %s | 删除=%s | 保留=%s"),
			*MontagePath, *SlotName.ToString(), *ExpectedRemainingSlot.ToString());
		return false;
	}

	int32 RemoveIndex = INDEX_NONE;
	int32 RemoveMatchCount = 0;
	int32 RemainingMatchCount = 0;
	for (int32 Index = 0; Index < Montage->SlotAnimTracks.Num(); ++Index)
	{
		const FName CurrentSlot = Montage->SlotAnimTracks[Index].SlotName;
		if (CurrentSlot == SlotName)
		{
			RemoveIndex = Index;
			++RemoveMatchCount;
		}
		if (CurrentSlot == ExpectedRemainingSlot)
		{
			++RemainingMatchCount;
		}
	}

	if (RemoveMatchCount != 1 || RemainingMatchCount != 1 || Montage->SlotAnimTracks.Num() <= 1)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[AnimationAssetFixer] %s 轨道前提不成立：删除命中=%d，保留命中=%d，总轨道=%d"),
			*MontagePath, RemoveMatchCount, RemainingMatchCount, Montage->SlotAnimTracks.Num());
		return false;
	}

	Montage->Modify();
	Montage->SlotAnimTracks.RemoveAt(RemoveIndex);
	// SyncSlotIndex 保存的是数组索引；删除前方轨道后必须同步平移，不能留下越界或错指轨道。
	if (Montage->SyncSlotIndex > RemoveIndex)
	{
		--Montage->SyncSlotIndex;
	}
	else if (Montage->SyncSlotIndex == RemoveIndex)
	{
		Montage->SyncSlotIndex = 0;
	}

#if WITH_EDITOR
	Montage->UpdateLinkableElements();
	Montage->PostEditChange();
#endif
	Montage->RefreshCacheData();

	const bool bSaved = SaveAssetPackage(Montage);
	UE_LOG(LogTemp, Log,
		TEXT("[AnimationAssetFixer] %s 删除 Slot=%s，保留 Slot=%s，保存=%d"),
		*MontagePath, *SlotName.ToString(), *ExpectedRemainingSlot.ToString(), bSaved);
	return bSaved;
}
