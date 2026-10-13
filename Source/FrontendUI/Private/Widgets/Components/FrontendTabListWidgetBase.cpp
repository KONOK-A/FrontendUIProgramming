// Vince Petrelli All Rights Reserved


#include "Widgets/Components/FrontendTabListWidgetBase.h"
#include "Editor/WidgetCompilerLog.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"

void UFrontendTabListWidgetBase::RequestRegisterTab(const FName& InTabID, const FText& InTabDisplayName)
{
	// 把“集合 ID + 按钮类”交给 CommonUI 注册，父类承担按钮生成与页签选择事件。
	// 第三个参数为 nullptr，未关联 ContentWidget；OptionsScreen 的 OnOptionsTabSelected 负责换列表内容。
	RegisterTab(InTabID,TabButtonEntryWidgetClass,nullptr);

	// 页签 ID 是程序查找键，按钮文字可以是独立的 FText；修改显示名称无需改变 Registry 的查找关系。
	if (UFrontendCommonButtonBase* FoundButton = Cast<UFrontendCommonButtonBase>(GetTabButtonBaseByID(InTabID)))
	{
		FoundButton->SetButtonText(InTabDisplayName);
	}
}

#if WITH_EDITOR	
void UFrontendTabListWidgetBase::ValidateCompiledDefaults(IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledDefaults(CompileLog);

	// 编辑器编译子蓝图时检查按钮类配置，避免运行时请求注册却没有可创建的按钮类。
	if (!TabButtonEntryWidgetClass)
	{
		CompileLog.Error(FText::FromString(
			TEXT("The variable TabButtonEntryWidgetClass has no valid entry specified. ") +
			GetClass()->GetName() + 
			TEXT(" needs a valid entry widget class to function properly")
		));
	}
}
#endif