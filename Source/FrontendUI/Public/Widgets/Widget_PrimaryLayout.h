// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "Widget_PrimaryLayout.generated.h"

class UCommonActivatableWidgetContainerBase;

/**
 * 主布局是页面容器的宿主：维护 GameplayTag 到 CommonUI 容器的映射，供子系统查找页面栈。
 * C++ 规定“如何登记和查找”，控件蓝图负责实际容器层级、布局及调用 RegisterWidgetStack。
 * 例如 Modal 是容器的身份标签；确认框/改键框是后来推入该容器的页面，二者不是同一对象。
 * 使用前还需把此布局实例交给 FrontendUISubsystem::RegisterCreatedPrimaryLayoutWidget。
 * 这里没有 C++ 创建布局或添加到视口的代码，不能仅凭接口声明确定蓝图中的初始化顺序。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_PrimaryLayout : public UCommonUserWidget
{
	GENERATED_BODY()
	
public:
	/**
	 * 查找已登记的容器，供 UI 子系统在类加载完成后创建并推入页面。
	 * @param InTag 已登记的页面栈标签；缺少该键会触发 checkf。
	 * @return 注册表保存的容器指针；本函数只检查键存在，不额外校验指针是否为空。
	 */
	UCommonActivatableWidgetContainerBase* FindWidgetStackByTag(const FGameplayTag& InTag) const;

protected:
	/**
	 * 在运行时记录栈标签与已有容器的映射；设计器预览中不登记，重复标签保留第一次的值。
	 * @param InStackTag 用于子系统路由的栈标签；Categories 仅筛选蓝图引脚，不代替注册或校验。
	 * @param InStack 子蓝图中的容器实例；调用方须传入可用容器，实现不会校验空指针。
	 * @note 不创建或显示容器，也不把当前布局注册到 UI 子系统。
	 */
	UFUNCTION(BlueprintCallable)
	void RegisterWidgetStack(UPARAM(meta = (Categories = "Frontend.WidgetStack")) FGameplayTag InStackTag, UCommonActivatableWidgetContainerBase* InStack);

private:
	/** 运行时的栈标签到容器映射；UPROPERTY 追踪对象引用，Transient 不将映射保存到资产。 */
	UPROPERTY(Transient)
	TMap<FGameplayTag,UCommonActivatableWidgetContainerBase*> RegisteredWidgetStackMap;
};
