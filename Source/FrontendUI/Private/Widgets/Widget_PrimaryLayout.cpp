// Vince Petrelli All Rights Reserved


#include "Widgets/Widget_PrimaryLayout.h"

#include "FrontendDebugHelper.h"

// Push 调用链先从子系统来到这里，取到容器后才调用 AddWidget。
// checkf 把“忘记注册目标层”视为开发配置错误，并输出具体标签，便于检查蓝图初始化流程。
UCommonActivatableWidgetContainerBase* UWidget_PrimaryLayout::FindWidgetStackByTag(const FGameplayTag& InTag) const
{	
	checkf(RegisteredWidgetStackMap.Contains(InTag),TEXT("Can not find the widget stack by the tag %s"),*InTag.ToString());

	return RegisteredWidgetStackMap.FindRef(InTag);
}

void UWidget_PrimaryLayout::RegisterWidgetStack(UPARAM(meta = (Categories = "Frontend.WidgetStack")) FGameplayTag InStackTag, UCommonActivatableWidgetContainerBase* InStack)
{
	// 主布局的控件蓝图可在初始化时登记各层；设计器预览不注册运行时容器。
	// 一个标签只接收第一次注册的容器，后续同名登记不会替换它；这里也不负责创建或显示容器。
	if (!IsDesignTime())
	{
		if (!RegisteredWidgetStackMap.Contains(InStackTag))
		{
			RegisteredWidgetStackMap.Add(InStackTag,InStack);
		}
	}
}
