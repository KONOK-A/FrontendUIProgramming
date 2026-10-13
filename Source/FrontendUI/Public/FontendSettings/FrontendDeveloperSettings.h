// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "FrontendDeveloperSettings.generated.h"

class UWidget_ActivatableBase;
/**
 * 开发者在项目设置中填写的资源映射，保存到 DefaultGame.ini；不是玩家个人设置。
 * 调用方只说“我要 OptionsScreen”，FrontendFunctionLibrary 再用标签从表中找到对应蓝图类。
 * 因此替换一个页面资源主要改这张映射表，UI 子系统仍沿用同一套加载/入栈流程。
 */
UCLASS(Config = Game, defaultconfig, meta = (DisplayName = "Frontend UI Settings"))
class FRONTENDUI_API UFrontendDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * 页面标签到可激活 Widget 蓝图类的映射，供 FunctionLibrary 查找及 UI 子系统按需加载。
	 * 键使用 Frontend.Widget 页面标签，不是 WidgetStack 标签；软类引用避免预先加载所有页面。
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Widget Reference", meta = (ForceInlineRow, Categories = "Frontend.Widget"))
	TMap< FGameplayTag,TSoftClassPtr<UWidget_ActivatableBase> > FrontendWidgetMap;

	/**
	 * 说明图片标签到 Texture2D 资源的软引用映射，供 Registry 配置详情图片。
	 * 图片引用对象，页面映射引用类；查表本身不会同步加载图片。
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Options Image Reference", meta = (ForceInlineRow, Categories = "Frontend.Image"))
	TMap< FGameplayTag, TSoftObjectPtr<UTexture2D> > OptionsScreenSoftImageMap;
};
