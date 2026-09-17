// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AnimationAssetFixer : ModuleRules
{
	public AnimationAssetFixer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"AnimGraph",
				"AnimationBlueprintLibrary",
				"BlueprintGraph",
				"BlueprintEditorLibrary",
				"GraphEditor",
			}
		);
	}
}
