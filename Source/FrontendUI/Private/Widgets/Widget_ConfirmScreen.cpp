// Vince Petrelli All Rights Reserved


#include "Widgets/Widget_ConfirmScreen.h"
#include "CommonTextBlock.h"
#include "Components/DynamicEntryBox.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "ICommonInputModule.h"

// 三个工厂统一构造标题/正文和按钮数组，让同一个确认页面承载不同用途。
// 单按钮通知只表示“已关闭”，不表示同意某项操作，因此这里的结果是 Closed。
UConfirmScreenInfoObject* UConfirmScreenInfoObject::CreateOKScreen(const FText& InScreenTitle, const FText& InScreenMsg)
{	
	UConfirmScreenInfoObject* InfoObject = NewObject<UConfirmScreenInfoObject>();
	InfoObject->ScreenTitle = InScreenTitle;
	InfoObject->ScreenMessage = InScreenMsg;

	FConfirmScreenButtonInfo OKButtonInfo;
	OKButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Closed;
	OKButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("Ok"));

	InfoObject->AvailableScreenButtons.Add(OKButtonInfo);

	return InfoObject;
}

// Yes/No 与 Ok/Cancel 使用相同的结果语义，调用方可统一只在 Confirmed 时执行重置等操作。
UConfirmScreenInfoObject* UConfirmScreenInfoObject::CreateYesNoScreen(const FText& InScreenTitle, const FText& InScreenMsg)
{
	UConfirmScreenInfoObject* InfoObject = NewObject<UConfirmScreenInfoObject>();
	InfoObject->ScreenTitle = InScreenTitle;
	InfoObject->ScreenMessage = InScreenMsg;

	FConfirmScreenButtonInfo YesButtonInfo;
	YesButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Confirmed;
	YesButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("Yes"));

	FConfirmScreenButtonInfo NoButtonInfo;
	NoButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Cancelled;
	NoButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("No"));

	InfoObject->AvailableScreenButtons.Add(YesButtonInfo);
	InfoObject->AvailableScreenButtons.Add(NoButtonInfo);

	return InfoObject;
}

UConfirmScreenInfoObject* UConfirmScreenInfoObject::CreateOkCancelScreen(const FText& InScreenTitle, const FText& InScreenMsg)
{
	UConfirmScreenInfoObject* InfoObject = NewObject<UConfirmScreenInfoObject>();
	InfoObject->ScreenTitle = InScreenTitle;
	InfoObject->ScreenMessage = InScreenMsg;

	FConfirmScreenButtonInfo OkButtonInfo;
	OkButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Confirmed;
	OkButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("Ok"));

	FConfirmScreenButtonInfo CancelButtonInfo;
	CancelButtonInfo.ConfirmScreenButtonType = EConfirmScreenButtonType::Cancelled;
	CancelButtonInfo.ButtonTextToDisplay = FText::FromString(TEXT("Cancel"));

	InfoObject->AvailableScreenButtons.Add(OkButtonInfo);
	InfoObject->AvailableScreenButtons.Add(CancelButtonInfo);

	return InfoObject;
}

void UWidget_ConfirmScreen::InitConfirmScreen(UConfirmScreenInfoObject* InScreenInfoObject, TFunction<void(EConfirmScreenButtonType)> ClickedButtonCallback)
{
	check(InScreenInfoObject && CommonTextBlock_Title && CommonTextBlock_Message && DynamicEntryBox_Buttons);

	// 阶段 1：把请求数据写入已由控件蓝图绑定的子控件。Init 只消费数据，不长期保存 InfoObject。
	CommonTextBlock_Title->SetText(InScreenInfoObject->ScreenTitle);
	CommonTextBlock_Message->SetText(InScreenInfoObject->ScreenMessage);

	// 阶段 2：页面可能再次初始化；先清理旧条目上的点击委托，再重置按钮容器。
	// 委托保存“点击时运行什么”，若复用按钮却未清理，就可能把上一次弹窗的业务逻辑再次执行。
	if (DynamicEntryBox_Buttons->GetNumEntries() != 0)
	{	
		// 条目控件类型由子蓝图配置，Reset 回调负责清空旧委托。
		DynamicEntryBox_Buttons->Reset<UFrontendCommonButtonBase>(
			[](UFrontendCommonButtonBase& ExistingButton)
			{
				ExistingButton.OnClicked().Clear();
			}
		);
	}

	check(!InScreenInfoObject->AvailableScreenButtons.IsEmpty());

	// 阶段 3：每条配置生成一个按钮；DynamicEntryBox 负责条目实例，C++ 填入文字和点击行为。
	for (const FConfirmScreenButtonInfo& AvailableButtonInfo : InScreenInfoObject->AvailableScreenButtons)
	{	
		UFrontendCommonButtonBase* AddedButton = DynamicEntryBox_Buttons->CreateEntry<UFrontendCommonButtonBase>();
		AddedButton->SetButtonText(AvailableButtonInfo.ButtonTextToDisplay);
		// 点击发生在此函数返回之后，所以按钮配置和上游回调按值捕获；不要捕获循环变量的引用。
		// this 指向当前确认页供点击后停用；普通 lambda 不会自动把 UObject 指针变成 GC 强引用。
		AddedButton->OnClicked().AddLambda(
			[ClickedButtonCallback,AvailableButtonInfo,this]()
			{
				// 先将结果交给上游（如确认重置，或异步节点广播蓝图输出），再结束当前页面的激活状态。
				// DeactivateWidget 是 CommonUI 页面退出入口，不是 delete；容器会接续处理页面状态。
				ClickedButtonCallback(AvailableButtonInfo.ConfirmScreenButtonType);

				DeactivateWidget();
			}
		);
	}
}

// CommonUI 或 GetDesiredFocusTarget 调用方询问页面的导航焦点目标时进入此重写。
// 键盘/手柄需要焦点落到可交互按钮上；本实现还在查询过程中直接 SetFocus，阅读时注意这个副作用。
UWidget* UWidget_ConfirmScreen::NativeGetDesiredFocusTarget() const
{
	if (DynamicEntryBox_Buttons->GetNumEntries() != 0)
	{	
		// 尝试聚焦末尾按钮（双按钮时为 No 或 Cancel）。
		// 此函数仍返回 Super 的结果；调用方再次设置焦点时可能覆盖这里的选择。
		DynamicEntryBox_Buttons->GetAllEntries().Last()->SetFocus();
	}

	return Super::NativeGetDesiredFocusTarget();
}
