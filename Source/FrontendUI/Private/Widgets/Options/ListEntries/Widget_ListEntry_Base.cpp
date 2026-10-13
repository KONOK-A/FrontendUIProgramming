// Vince Petrelli All Rights Reserved


#include "Widgets/Options/ListEntries/Widget_ListEntry_Base.h"
#include "Widgets/Options/DataObjects/ListDataObject_Base.h"
#include "CommonTextBlock.h"
#include "Components/ListView.h"
#include "CommonInputSubsystem.h"

void UWidget_ListEntry_Base::NativeOnListEntryWidgetHovered(bool bWasHovered)
{
	// 先把“是否悬停”和“是否仍选中”一起交给蓝图，再决定通用高亮。
	// 鼠标离开时仍选中的行保留高亮，因此悬停消失不等于选中状态也消失。
	BP_OnListEntryWidgetHovered(bWasHovered, GetListItem()? IsListItemSelected() : false);

	if (bWasHovered)
	{
		BP_OnToggleEntryWidgetHighlightState(true);
	}
	else
	{	
		BP_OnToggleEntryWidgetHighlightState(GetListItem() && IsListItemSelected()? true : false);
	}
}

void UWidget_ListEntry_Base::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	// ListView 分配的是 UObject，通过 CastChecked 明确要求它属于本项目的数据基类。
	// 之后调用可被派生类覆写的入口：Base 连接通知，String/Scalar/KeyRemap 初始化专属控件。
	// 这是每次分配数据都会走的路径，不能只在 NativeOnInitialized 中读取一次设置值。
	OnOwningListDataObjectSet(CastChecked<UListDataObject_Base>(ListItemObject));
}

void UWidget_ListEntry_Base::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserObjectListEntry::NativeOnItemSelectionChanged(bIsSelected);

	// 列表选择变化只在这里更新本行高亮；OptionsScreen 的选择回调另行更新右侧详情。
	BP_OnToggleEntryWidgetHighlightState(bIsSelected);
}

void UWidget_ListEntry_Base::NativeOnEntryReleased()
{
	IUserObjectListEntry::NativeOnEntryReleased();

	// 被 ListView 释放不等于 UObject 已销毁，行可能进入复用池等待显示其他数据。
	// 此实现只刷新悬停/高亮表现，没有解除之前的数据委托订阅。
	NativeOnListEntryWidgetHovered(false);
}

FReply UWidget_ListEntry_Base::NativeOnFocusReceived(const FGeometry& InGeometry, const FFocusEvent& InFocusEvent)
{
	UCommonInputSubsystem* CommonInputSubsystem = GetInputSubsystem();

	// UE 将焦点给这一行时，手柄操作还需要到达行内真正可交互的控件。
	// 例如 String 行可由蓝图返回 Rotator；GetCachedWidget 取得其底层 Slate 控件，
	// FReply 中的 SetUserFocus 请求把焦点转交过去。具体返回哪一个控件取决于子蓝图。
	if (CommonInputSubsystem && CommonInputSubsystem->GetCurrentInputType() == ECommonInputType::Gamepad)
	{
		if (UWidget* WidgetToFocus = BP_GetWidgetToFocusForGamepad())
		{	
			if (TSharedPtr<SWidget> SlateWidgetToFocus = WidgetToFocus->GetCachedWidget())
			{
				return FReply::Handled().SetUserFocus(SlateWidgetToFocus.ToSharedRef());
			}		
		}
	}

	return Super::NativeOnFocusReceived(InGeometry,InFocusEvent);
}

void UWidget_ListEntry_Base::OnOwningListDataObjectSet(UListDataObject_Base* InOwningListDataObject)
{
	if (CommonText_SettingDisplayName)
	{
		CommonText_SettingDisplayName->SetText(InOwningListDataObject->GetDataDisplayName());
	}
	
	// 将数据对象的两类多播委托分别接到本行：值变化刷新显示，依赖变化重算可编辑状态。
	// AddUObject 注册“对象 + 成员函数”；数据层 Broadcast 时才会回调，不是此处立即调用。
	// IsBoundToObject 只防止在当前对象上重复订阅，不会替此行解除旧数据对象上的订阅。
	if (!InOwningListDataObject->OnListDataModified.IsBoundToObject(this))
	{
		InOwningListDataObject->OnListDataModified.AddUObject(this,&ThisClass::OnOwningListDataObjectModified);
	}

	if (!InOwningListDataObject->OnDependencyDataModified.IsBoundToObject(this))
	{
		InOwningListDataObject->OnDependencyDataModified.AddUObject(this,&ThisClass::OnOwningDependencyDataObjectModified);
	}

	// 编辑条件可能设置强制值；这里同时驱动控件的启用状态。
	OnToggleEditableState(InOwningListDataObject->IsDataCurrentlyEditable());

	CachedOwningDataObject = InOwningListDataObject;
}

void UWidget_ListEntry_Base::OnOwningListDataObjectModified(UListDataObject_Base* OwningModifiedData, EOptionsListDataModifyReason ModifyReason)
{
}

// 回调参数是发生变化的依赖数据；这里要更新的是当前行，所以使用 CachedOwningDataObject。
// 例如窗口模式改变后，分辨率行重新求自己的条件，再通过虚函数更新操作控件。
void UWidget_ListEntry_Base::OnOwningDependencyDataObjectModified(UListDataObject_Base* OwningModifiedDependencyData, EOptionsListDataModifyReason ModifyReason)
{	
	if (CachedOwningDataObject)
	{
		OnToggleEditableState(CachedOwningDataObject->IsDataCurrentlyEditable());
	}
	
}

void UWidget_ListEntry_Base::OnToggleEditableState(bool bIsEditable)
{
	if (CommonText_SettingDisplayName)
	{
		CommonText_SettingDisplayName->SetIsEnabled(bIsEditable);
	}
}

void UWidget_ListEntry_Base::SelectThisEntryWidget()
{
	// 子控件的点击/拖动未必等同于点击整行，所以各派生行主动同步 ListView 的选中数据。
	// 选择变化再分别通知本行更新高亮、OptionsScreen 更新详情，避免它们各自保存一份选择。
	CastChecked<UListView>(GetOwningListView())->SetSelectedItem(GetListItem());
}
