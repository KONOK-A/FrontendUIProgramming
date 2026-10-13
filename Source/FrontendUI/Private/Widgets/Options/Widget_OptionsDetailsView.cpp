// Vince Petrelli All Rights Reserved


#include "Widgets/Options/Widget_OptionsDetailsView.h"
#include "CommonTextBlock.h"
#include "CommonLazyImage.h"
#include "CommonRichTextBlock.h"
#include "Widgets/Options/DataObjects/ListDataObject_Base.h"

void UWidget_OptionsDetailsView::UpdateDetailsViewInfo(UListDataObject_Base* InDataObject, const FString& InEntryWidgetClassName)
{
	if (!InDataObject)
	{
		return;
	}

	CommonTextBlock_Title->SetText(InDataObject->GetDataDisplayName());

	// 软引用只描述图片资产位置，CommonLazyImage 据此按需加载，不要求页面预先持有贴图。
	// 有图时使用 SelfHitTestInvisible：图片可见但自身不拦截鼠标；无图时 Collapsed 不占布局空间。
	if (!InDataObject->GetSoftDescriptionImage().IsNull())
	{
		CommonLazyImage_DescriptionImage->SetBrushFromLazyTexture(InDataObject->GetSoftDescriptionImage());
		CommonLazyImage_DescriptionImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		CommonLazyImage_DescriptionImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	CommonRichText_Description->SetText(InDataObject->GetDescriptionRichText());

	// 同时展示 DataObject 和 Entry Widget 的实际类名，帮助从界面追到两侧的 C++/蓝图实现。
	const FString DynamicDetails = FString::Printf(
		TEXT("Data Object Class: <Bold>%s</>\n\nEntry Widget Class:<Bold>%s</>"),
		*InDataObject->GetClass()->GetName(),
		*InEntryWidgetClassName
	);

	CommonRichText_DynamicDetails->SetText(FText::FromString(DynamicDetails));

	// 重新求编辑条件后显示禁用原因，例如窗口模式限制分辨率选择。
	// IsDataCurrentlyEditable 在本项目中还可能应用条件的强制值，不只是读取一个布尔字段。
	CommonRichText_DisabledReason->SetText(InDataObject->IsDataCurrentlyEditable()? FText::GetEmpty() : InDataObject->GetDisabledRichText());
}

void UWidget_OptionsDetailsView::ClearDetailsViewInfo()
{
	CommonTextBlock_Title->SetText(FText::GetEmpty());
	CommonLazyImage_DescriptionImage->SetVisibility(ESlateVisibility::Collapsed);
	CommonRichText_Description->SetText(FText::GetEmpty());
	CommonRichText_DynamicDetails->SetText(FText::GetEmpty());
	CommonRichText_DisabledReason->SetText(FText::GetEmpty());
}

// 该区域不参与页面激活栈；实例初始化时先清空，之后由 OptionsScreen 的事件回调提供内容。
void UWidget_OptionsDetailsView::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	ClearDetailsViewInfo();
}
