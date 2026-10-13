// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

// 用编辑器打开 C++ 项目时，需要构建这个目标对应的模块。
public class FrontendUIEditorTarget : TargetRules
{
	public FrontendUIEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		ExtraModuleNames.Add("FrontendUI");
	}
}
