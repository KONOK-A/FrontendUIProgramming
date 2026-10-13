// Copyright Epic Games, Inc. All Rights Reserved.

#include "FrontendUI.h"
#include "Modules/ModuleManager.h"

// 向 UE 注册主游戏模块；项目里的 C++ 类都属于 FrontendUI 模块。
// 这是模块入口而不是界面启动流程；读菜单行为应从 Subsystem、AsyncAction 和 Widget 类继续。
IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, FrontendUI, "FrontendUI" );
