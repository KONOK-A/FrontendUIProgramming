// Vince Petrelli All Rights Reserved


#include "Widgets/Options/ListEntries/Widget_ListEntry_String.h"
#include "Widgets/Options/DataObjects/ListDataObject_String.h"
#include "Widgets/Components/FrontendCommonRotator.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "CommonInputSubsystem.h"

#include "FrontendDebugHelper.h"

void UWidget_ListEntry_String::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 初始化时一次性连接按钮与 Rotator 的事件；回调执行时再使用当前缓存的数据对象。
	// 即使 ListView 以后复用这一行，也不必重复给同一按钮添加点击回调。
	CommonButton_PreviousOption->OnClicked().AddUObject(this,&ThisClass::OnPreviousOptionButtonClicked);
	CommonButton_NextOption->OnClicked().AddUObject(this,&ThisClass::OnNextOptionButtonClicked);

	CommonRotator_AvailableOptions->OnClicked().AddLambda([this](){SelectThisEntryWidget();});
	CommonRotator_AvailableOptions->OnRotatedEvent.AddUObject(this,&ThisClass::OnRotatorValueChanged);
}

void UWidget_ListEntry_String::OnOwningListDataObjectSet(UListDataObject_Base* InOwningListDataObject)
{
	Super::OnOwningListDataObjectSet(InOwningListDataObject);

	CachedOwningStringDataObject = CastChecked<UListDataObject_String>(InOwningListDataObject);

	// 复用行时同时替换候选标签和当前值，避免继续显示上一个数据对象的选项。
	CommonRotator_AvailableOptions->PopulateTextLabels(CachedOwningStringDataObject->GetAvailableOptionsTextArray());
	CommonRotator_AvailableOptions->SetSelectedOptionByText(CachedOwningStringDataObject->GetCurrentDisplayText());

}

// Base 已把数据的修改通知连到此虚函数；直接改值、恢复默认等路径都据此统一刷新。
// 从当前缓存对象重新读取显示文本，使其他入口引起的变化也能体现在这一行。
void UWidget_ListEntry_String::OnOwningListDataObjectModified(UListDataObject_Base* OwningModifiedData, EOptionsListDataModifyReason ModifyReason)
{
	if (CachedOwningStringDataObject)
	{
		CommonRotator_AvailableOptions->SetSelectedOptionByText(CachedOwningStringDataObject->GetCurrentDisplayText());
	}
}

// Base 在分配数据或依赖变化时调用；除标题外，前后按钮和 Rotator 也要同步禁止/允许交互。
void UWidget_ListEntry_String::OnToggleEditableState(bool bIsEditable)
{
	Super::OnToggleEditableState(bIsEditable);

	CommonButton_PreviousOption->SetIsEnabled(bIsEditable);
	CommonRotator_AvailableOptions->SetIsEnabled(bIsEditable);
	CommonButton_NextOption->SetIsEnabled(bIsEditable);
}

// 前后按钮只向数据对象提出切换请求；候选查找、写入设置和修改广播由数据对象完成。
void UWidget_ListEntry_String::OnPreviousOptionButtonClicked()
{	
	if (CachedOwningStringDataObject)
	{
		CachedOwningStringDataObject->BackToPreviousOption();
	}

	SelectThisEntryWidget();
}

void UWidget_ListEntry_String::OnNextOptionButtonClicked()
{
	if (CachedOwningStringDataObject)
	{
		CachedOwningStringDataObject->AdvanceToNextOption();
	}

	SelectThisEntryWidget();
}

void UWidget_ListEntry_String::OnRotatorValueChanged(int32 Value, bool bUserInitiated)
{
	if (!CachedOwningStringDataObject)
	{
		return;
	}

	UCommonInputSubsystem* CommonInputSubsystem = GetInputSubsystem();

	// 数据通知会通过 SetSelectedOptionByText 刷新 Rotator，不能又把这次程序刷新当作用户改值。
	// 因此只接受 bUserInitiated 的事件，避免“数据刷新 UI → UI 再写回数据”的往返。
	if (!CommonInputSubsystem || !bUserInitiated)
	{
		return;
	}

	// 手柄通过 Rotator 选择；鼠标切换由前后按钮各自的回调处理。
	if (CommonInputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad)
	{
		// 提交当前选中的标签，由 String 数据对象找到对应候选值并写回设置。
		CachedOwningStringDataObject->OnRotatorInitiatedValueChange(CommonRotator_AvailableOptions->GetSelectedText());
	}
}
