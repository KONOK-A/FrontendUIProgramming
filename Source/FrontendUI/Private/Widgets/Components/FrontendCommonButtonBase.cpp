// Vince Petrelli All Rights Reserved


#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Subsytems/FrontendUISubsystem.h"
#include "CommonLazyImage.h"

// 内容由页签、确认框等使用方提供，大小写规则由本按钮统一处理。
// 此函数忽略空文本，因此调用 SetButtonText(FText()) 不会清除已有文字。
void UFrontendCommonButtonBase::SetButtonText(FText InText)
{
	if (CommonTextBlock_ButtonText && !InText.IsEmpty())
	{
		CommonTextBlock_ButtonText->SetText(bUserUpperCaseForButtonText? InText.ToUpper() : InText);
	}
}

FText UFrontendCommonButtonBase::GetButtonDisplayText() const
{	
	if (CommonTextBlock_ButtonText)
	{
		return CommonTextBlock_ButtonText->GetText();
	}

	return FText();
}

void UFrontendCommonButtonBase::SetButtionDisplayImage(const FSlateBrush& InBrush)
{
	if (CommonLazyImage_ButtonImage)
	{
		CommonLazyImage_ButtonImage->SetBrush(InBrush);
	}
}

void UFrontendCommonButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	// UUserWidget 的 PreConstruct 可在设计器预览和运行时构建阶段执行，不适合作为只运行一次的入口。
	// 这里只应用默认文字，让蓝图作者在设计器中看到效果；运行时使用方还可再次 SetButtonText。
	SetButtonText(ButtonDisplayText);
}

void UFrontendCommonButtonBase::NativeOnCurrentTextStyleChanged()
{
	Super::NativeOnCurrentTextStyleChanged();

	// CommonUI 在按钮的当前文本样式变化时调用此钩子，例如状态切换后需要新的文字表现。
	// 父按钮管理“应使用哪个样式”，此处负责把选出的样式类应用到实际绘制文字的子控件。
	if (CommonTextBlock_ButtonText && GetCurrentTextStyleClass())
	{
		CommonTextBlock_ButtonText->SetStyle(GetCurrentTextStyleClass());
	}
}

void UFrontendCommonButtonBase::NativeOnHovered()
{
	Super::NativeOnHovered();

	// CommonUI 通知悬停状态后，通过子系统事件发送“来源按钮 + 描述文字”。
	// 按钮无需持有说明控件；订阅者可按来源过滤事件。当前 C++ 中只有广播端，订阅可由蓝图接入。
	if (!ButtonDescriptionText.IsEmpty())
	{
		UFrontendUISubsystem::Get(this)->OnButtonDescriptionTextUpdated.Broadcast(this,ButtonDescriptionText);
	}
}

void UFrontendCommonButtonBase::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();

	// 结束悬停时发送空文字，交由同一订阅者清除该按钮对应的说明。
	UFrontendUISubsystem::Get(this)->OnButtonDescriptionTextUpdated.Broadcast(this,FText::GetEmpty());
}
