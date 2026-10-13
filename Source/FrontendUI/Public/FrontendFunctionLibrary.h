// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "FrontendFunctionLibrary.generated.h"

class UWidget_ActivatableBase;

/**
 * 供蓝图和 C++ 共用的静态查询函数；调用方无需创建本类的对象。
 * 这里只回答“标签对应哪个资源”，加载页面和加入容器由 FrontendUISubsystem 完成。
 */
UCLASS()
class FRONTENDUI_API UFrontendFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	/**
	 * 从项目设置查询页面蓝图类的软引用，供 UI 子系统随后异步加载。
	 * @param InWidgetTag - 已登记到 FrontendWidgetMap 的页面标签，不是容器栈标签。
	 * @return 配置中的软类引用；此处不加载资源或创建 Widget，也不检查引用是否为空。
	 * @warning 映射必须包含该键；缺项由 checkf 暴露，不提供默认页面。
	 * @note BlueprintPure 提供无执行引脚的查询，Categories 仅限定编辑时的标签选择。
	 */
	UFUNCTION(BlueprintPure, Category = "Frontend Function Library")
	static TSoftClassPtr<UWidget_ActivatableBase> GetFrontendSoftWidgetClassByTag( UPARAM(meta = (Categories = "Frontend.Widget")) FGameplayTag InWidgetTag);

	/**
	 * 为设置项查询说明图片，供详情控件在使用时按需加载。
	 * @param InImageTag - 已登记到 OptionsScreenSoftImageMap 的图片标签。
	 * @return 配置中的 Texture2D 软引用；不保证图片已加载或引用非空。
	 * @warning 映射必须包含该键，否则触发 checkf 检查。
	 */
	UFUNCTION(BlueprintPure, Category = "Frontend Function Library")
	static TSoftObjectPtr<UTexture2D> GetOptionsSoftImageByTag(UPARAM(meta = (Categories = "Frontend.Image")) FGameplayTag InImageTag);
};
