// Vince Petrelli All Rights Reserved


#include "FrontendFunctionLibrary.h"
#include "FontendSettings/FrontendDeveloperSettings.h"

// 例如传入 Frontend.Widget.ConfirmScreen，会查到 DefaultGame.ini 配置的确认框蓝图类。
// 注意页面标签与 WidgetStack 标签不同：前者选择“显示什么”，后者选择“放在哪里”。
TSoftClassPtr<UWidget_ActivatableBase> UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(UPARAM(meta = (Categories = "Frontend.Widget")) FGameplayTag InWidgetTag)
{	
	// GetDefault 读取带有项目配置的类默认对象，无需创建一个设置实例。
	const UFrontendDeveloperSettings* FrontendDeveloperSettings = GetDefault<UFrontendDeveloperSettings>();

	// 标签必须先在项目设置中登记；checkf 用于指出开发阶段的配置错误。
	checkf(FrontendDeveloperSettings->FrontendWidgetMap.Contains(InWidgetTag),TEXT("Could not find the corresponding widget under the tag %s"),*InWidgetTag.ToString());

	return FrontendDeveloperSettings->FrontendWidgetMap.FindRef(InWidgetTag);
}

// Registry 用图片标签取得说明图软引用，再交给设置详情界面；此函数只查表。
TSoftObjectPtr<UTexture2D> UFrontendFunctionLibrary::GetOptionsSoftImageByTag(FGameplayTag InImageTag)
{
	const UFrontendDeveloperSettings* FrontendDeveloperSettings = GetDefault<UFrontendDeveloperSettings>();

	checkf(FrontendDeveloperSettings->OptionsScreenSoftImageMap.Contains(InImageTag),TEXT("Could not find an image accociated with tag %s"),*InImageTag.ToString());

	return FrontendDeveloperSettings->OptionsScreenSoftImageMap.FindRef(InImageTag);

}