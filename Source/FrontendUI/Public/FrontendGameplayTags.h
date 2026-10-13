// Vince Petrelli All Rights Reserved

#pragma once

#include "NativeGameplayTags.h"

/**
 * 前端路由使用的原生标签：栈标签定位容器，页面标签查找 Widget 类，图片标签查找纹理。
 * 例如 Frontend.Widget.ConfirmScreen 表示页面身份，Frontend.WidgetStack.Modal 表示容器身份。
 * 变量在同名 .cpp 中注册实际路径；标签本身不会创建资源。
 */
namespace FrontendGameplayTags
{	
	//Frontend widget stack
	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_WidgetStack_Modal);
	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_WidgetStack_GameMenu);
	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_WidgetStack_GameHud);
	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_WidgetStack_Frontend);

	//Frontend widgets
	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_Widget_PressAnyKeyScreen);
	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_Widget_MainMenuScreen);
	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_Widget_OptionsScreen);

	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_Widget_ConfirmScreen);

	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_Widget_KeyRemapScreen);

	//Frontend Options Image
	FRONTENDUI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Frontend_Image_TestImage);
}