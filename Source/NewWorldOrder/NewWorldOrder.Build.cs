// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NewWorldOrder : ModuleRules
{
	public NewWorldOrder(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		//IPlatformInputDeviceMapper属于"ApplicationCore"模块
		PublicDependencyModuleNames.AddRange(new string[] {"Core", "CoreUObject", "Engine", "InputCore"});

		//物理模块，引入才可以使用UPhysicalMaterial
		PublicDependencyModuleNames.AddRange(new string[] {"PhysicsCore"});

		PublicDependencyModuleNames.AddRange(new string[] {"AIModule"});

		//Mutable
		PublicDependencyModuleNames.AddRange(new string[] {"MutableRuntime", "CustomizableObject"});

		//CommonUI
		PublicDependencyModuleNames.AddRange(new string[] {"CommonUI", "CommonInput", "EnhancedInput", "ModelViewViewModel", "GameSettings", "GameSubtitles"});

		PublicDependencyModuleNames.AddRange(new string[] {"ApplicationCore"});

		PublicDependencyModuleNames.AddRange(new string[]
			{"OnlineSubsystem", "OnlineSubsystemSteam"});

		// 语言首启策略需要读取 Steam 应用语言；该依赖来自当前 Unreal 安装的 Steamworks SDK，
		// 只参与编译和链接，打包后的玩家不需要安装 Unreal Engine。
		PrivateDefinitions.Add("NEWWORLDORDER_WITH_STEAM=1");
		AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");

		PublicDependencyModuleNames.AddRange(new string[] {"GameplayAbilities", "GameplayTasks", "GameplayTags"});

		PublicDependencyModuleNames.AddRange(new string[] {"Niagara", "NavigationSystem"});
		PublicDependencyModuleNames.AddRange(new string[] {"LevelSequence", "MovieScene"});

		PublicDependencyModuleNames.AddRange(new string[] {"UMG", "Slate", "SlateCore", "RHI", "AudioModulation", "NetworkReplayStreaming", "PropertyPath"});
		PublicDependencyModuleNames.AddRange(new string[] {"UIExtension"});

		PublicDependencyModuleNames.AddRange(new string[] {"Json", "JsonUtilities"});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AudioMixer",
				"RenderCore",
				"DeveloperSettings",
				"NetCore",
				"IrisCore",
				"CommonGame",
				"CommonLoadingScreen",
				"CommonUser",
				"OnlineSubsystemUtils",
				"EngineSettings",
				"ModularGameplay",
				"ModularGameplayActors",
				"GameplayMessageRuntime"
			}
		);
	}
}
