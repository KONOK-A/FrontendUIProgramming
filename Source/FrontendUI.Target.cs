// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

// Game 目标用于构建游戏程序；Editor 目标则包含编辑器所需支持。
public class FrontendUITarget : TargetRules
{
	public FrontendUITarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("FrontendUI");
	}
}
