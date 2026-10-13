// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

// 模块构建规则：声明本模块编译时可以使用哪些引擎模块，不参与游戏运行逻辑。
// Target.cs 选择要构建游戏还是编辑器，Build.cs 则为其中的 FrontendUI 模块列出依赖。
public class FrontendUI : ModuleRules
{
	public FrontendUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// 输入、标签和 UI 的依赖集中在这里；.uproject 还提供了 CommonUI 等额外依赖。
		// Public 依赖也提供给使用本模块的代码；Private 依赖仅供本模块内部使用。
		// 例如 PropertyPath 用于设置的反射读写，Slate/SlateCore 用于控件与键位输入捕获。
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
            "GameplayTags",
			"UMG",
            "CommonInput",
            "PropertyPath",
            "PreLoadScreen"});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
