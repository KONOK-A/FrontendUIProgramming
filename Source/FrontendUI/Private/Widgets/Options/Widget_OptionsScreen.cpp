// Vince Petrelli All Rights Reserved


#include "Widgets/Options/Widget_OptionsScreen.h"
#include "Input/CommonUIInputTypes.h"
#include "ICommonInputModule.h"
#include "Widgets/Options/OptionsDataRegistry.h"
#include "Widgets/Components/FrontendTabListWidgetBase.h"
#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"
#include "Widgets/Components/FrontendCommonListView.h"
#include "FontendSettings/FrontendGameUserSettings.h"
#include "Widgets/Options/ListEntries/Widget_ListEntry_Base.h"
#include "Widgets/Options/Widget_OptionsDetailsView.h"
#include "Subsytems/FrontendUISubsystem.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"

#include "FrontendDebugHelper.h"

void UWidget_OptionsScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 初始化阶段先连接页面输入。CommonUI 动作用数据表描述键鼠/手柄的触发方式，
	// 这里绑定“触发后执行哪个函数”；FBindUIActionArgs 的 true 允许它出现在动作栏。
	// CreateUObject 把当前页面和成员函数组成委托，事件真正触发时才调用该函数。
	if (!ResetAction.IsNull())
	{
		ResetActionHandle = RegisterUIActionBinding(
			FBindUIActionArgs(
				ResetAction,
				true,
				FSimpleDelegate::CreateUObject(this, &ThisClass::OnResetBoundActionTriggered)
			)
		);
	}
	
	RegisterUIActionBinding(
		FBindUIActionArgs(
			ICommonInputModule::GetSettings().GetDefaultBackAction(),
			true,
			FSimpleDelegate::CreateUObject(this,&ThisClass::OnBackBoundActionTriggered)
		)
	);

	// 再连接子控件事件：切页换整批数据，悬停/选中只换详情。
	// AddUniqueDynamic 是动态委托绑定，AddUObject 是原生 C++ 委托绑定；两者都在注册回调。
	TabListWidget_OptionsTabs->OnTabSelected.AddUniqueDynamic(this,&ThisClass::OnOptionsTabSelected);
	
	CommonListView_OptionsList->OnItemIsHoveredChanged().AddUObject(this,&ThisClass::OnListViewItemHovered);
	CommonListView_OptionsList->OnItemSelectionChanged().AddUObject(this,&ThisClass::OnListViewItemSelected);
}

void UWidget_OptionsScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 激活阶段准备可交互内容；它可发生多次，区别于前面只负责连接事件的初始化。
	// 第一次访问时创建设置数据，之后复用 Registry，并只补注册尚不存在的页签。
	for (UListDataObject_Collection* TabCollection : GetOrCreateDataRegistry()->GetRegisteredOptionsTabCollections())
	{
		if (!TabCollection)
		{
			continue;
		}

		const FName TabID = TabCollection->GetDataID();

		if (TabListWidget_OptionsTabs->GetTabButtonBaseByID(TabID) != nullptr)
		{
			continue;
		}

		TabListWidget_OptionsTabs->RequestRegisterTab(TabID,TabCollection->GetDataDisplayName());
	}
}

void UWidget_OptionsScreen::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	// 停用是本工程统一应用设置的出口；Back 调用 DeactivateWidget 后也会到这里。
	// 很多数据项修改时已立即 Apply，这里再次应用；页面没有“返回即撤销”的快照流程。
	UFrontendGameUserSettings::Get()->ApplySettings(true);
}

UWidget* UWidget_OptionsScreen::NativeGetDesiredFocusTarget() const
{	
	// CommonUI 请求默认焦点时，优先找到已选中数据对应的行控件。
	// “选中项”属于 ListView 数据状态，“焦点”决定谁接收键盘/手柄输入，二者需要衔接。
	if (UObject* SelectedObject = CommonListView_OptionsList->GetSelectedItem())
	{
		if (UUserWidget* SelectedEntryWidget = CommonListView_OptionsList->GetEntryWidgetFromItem(SelectedObject))
		{
			return SelectedEntryWidget;
		}
	}

	return Super::NativeGetDesiredFocusTarget();
}

UOptionsDataRegistry* UWidget_OptionsScreen::GetOrCreateDataRegistry()
{	
	// 懒创建 Registry：只有实际需要设置数据时才初始化各页集合。
	// NewObject 创建 UE 管理的 UObject；成员上的 UPROPERTY 让页面持有这份运行时数据。
	// 传入 LocalPlayer，使 Control 页能查询这个本地玩家的 Enhanced Input 按键配置。
	if (!CreatedOwningDataRegistry)
	{
		CreatedOwningDataRegistry = NewObject<UOptionsDataRegistry>();
		CreatedOwningDataRegistry->InitOptionsDataRegistry(GetOwningLocalPlayer());
	}

	checkf(CreatedOwningDataRegistry,TEXT("Data registry for options screen is not valid"));

	return CreatedOwningDataRegistry;
}

void UWidget_OptionsScreen::OnResetBoundActionTriggered()
{
	if (ResettableDataArray.IsEmpty())
	{
		return;
	}

	// 第一步：取当前页签名并请求 Yes/No 弹窗；Reset 面向候选数组，逐项恢复数据默认值。
	// 候选在切页时重建、随后随数据通知更新，并不是调用 GameUserSettings 的全局重置。
	UCommonButtonBase* SelectedTabButton = TabListWidget_OptionsTabs->GetTabButtonBaseByID(TabListWidget_OptionsTabs->GetActiveTab());

	const FString SelectedTabButtonName = CastChecked<UFrontendCommonButtonBase>(SelectedTabButton)->GetButtonDisplayText().ToString();

	// 弹窗异步显示，下面的 lambda 是用户作出选择后的回调，不会在这行调用时立即执行。
	UFrontendUISubsystem::Get(this)->PushConfirmScreenToModalStackAynsc(
		EConfirmScreenType::YesNo,
		FText::FromString(TEXT("Reset")),
		FText::FromString(TEXT("Are you sure you want to reset all the settings under the ") + SelectedTabButtonName + TEXT(" tab?")),
		[this](EConfirmScreenButtonType ClickedButtonType)
		{
			if (ClickedButtonType != EConfirmScreenButtonType::Confirmed)
			{
				return;
			}

			// 恢复默认也会广播修改；此标记避免回调在遍历期间增删重置数组。
			bIsResettingData = true;
			bool bHasDataFailedToReset = false;

			// 第二步：让每个数据对象自行恢复；String、Scalar、KeyRemap 各有自己的写回方式。
			// 恢复产生的数据通知会驱动列表行刷新，因此这里不用逐个操作行内控件。
			for (UListDataObject_Base* DataToReset : ResettableDataArray)
			{
				if (!DataToReset)
				{
					continue;
				}

				if (DataToReset->TryResetBackToDefaultValue())
				{
					Debug::Print(DataToReset->GetDataDisplayName().ToString() + TEXT(" was reset"));
				}
				else
				{
					bHasDataFailedToReset = true;
					Debug::Print(DataToReset->GetDataDisplayName().ToString() + TEXT(" failed to reset"));
				}
			}

			// 全部恢复成功后清空候选并移除 Reset 动作；有失败则保留，供后续处理。
			if (!bHasDataFailedToReset)
			{
				ResettableDataArray.Empty();
				RemoveActionBinding(ResetActionHandle);
			}

			bIsResettingData = false;
		}
	);
}

void UWidget_OptionsScreen::OnBackBoundActionTriggered()
{
	DeactivateWidget();
}

void UWidget_OptionsScreen::OnOptionsTabSelected(FName TabId)
{	
	// 第一步：TabList 广播 TabId 后清除旧详情，防止切页时仍显示上一页说明。
	DetailsView_ListEntryInfo->ClearDetailsViewInfo();

	// 第二步：Registry 将该页分类树展开成含分类标题的数据列表。
	// SetListItems 传入的是 UObject 数据，不是现成控件；ListView 根据类型映射生成/复用行，
	// 并通过行的 NativeOnListItemObjectSet 将对应数据交给它。
	TArray<UListDataObject_Base*> FoundListSourceItems = GetOrCreateDataRegistry()->GetListSourceItemsBySelectedTabID(TabId);

	CommonListView_OptionsList->SetListItems(FoundListSourceItems);
	CommonListView_OptionsList->RequestRefresh();

	// 为新列表请求起始导航位置和选中项；分类标题能否选中仍由 ListView 的规则判断。
	if (CommonListView_OptionsList->GetNumItems() != 0)
	{
		CommonListView_OptionsList->NavigateToIndex(0);
		CommonListView_OptionsList->SetSelectedIndex(0);
	}

	// 第三步：重建当前页的 Reset 候选，并订阅数据修改以持续更新动作状态。
	// CanResetBackToDefaultValue 同时表达“支持恢复”和“当前确实偏离默认值”；
	// 没配置默认值的项目不会仅因为显示在列表里就成为重置候选。
	ResettableDataArray.Empty();

	for (UListDataObject_Base* FoundListSourceItem : FoundListSourceItems)
	{
		if (!FoundListSourceItem)
		{
			continue;
		}

		if (!FoundListSourceItem->OnListDataModified.IsBoundToObject(this))
		{
			FoundListSourceItem->OnListDataModified.AddUObject(this,&ThisClass::OnListViewListDataModified);
		}

		if (FoundListSourceItem->CanResetBackToDefaultValue())
		{
			ResettableDataArray.AddUnique(FoundListSourceItem);
		}
	}

	// 第四步：根据候选是否为空同步 Reset 的输入绑定和动作栏提示。
	// 移除/加入的是已注册的句柄，回调本身不需要重复创建。
	if (ResettableDataArray.IsEmpty())
	{
		RemoveActionBinding(ResetActionHandle);
	}
	else
	{
		if (!GetActionBindings().Contains(ResetActionHandle))
		{
			AddActionBinding(ResetActionHandle);
		}
	}
}

void UWidget_OptionsScreen::OnListViewItemHovered(UObject* InHoveredItem, bool bWasHovered)
{
	if (!InHoveredItem)
	{
		return;
	}

	// ListView 通知鼠标进入/离开某个数据项，页面再找到其当前行，让蓝图更新行内表现。
	UWidget_ListEntry_Base* HoveredEntryWidget = CommonListView_OptionsList->GetEntryWidgetFromItem<UWidget_ListEntry_Base>(InHoveredItem);

	check(HoveredEntryWidget);

	HoveredEntryWidget->NativeOnListEntryWidgetHovered(bWasHovered);

	// 悬停只临时预览详情，不会在这里修改 ListView 的选中项。
	// 移开鼠标后若存在选中项，就恢复那一项的详情；因此选中行与悬停行可以不同。
	if (bWasHovered)
	{
		DetailsView_ListEntryInfo->UpdateDetailsViewInfo(
			CastChecked<UListDataObject_Base>(InHoveredItem),
			TryGetEntryWidgetClassName(InHoveredItem)
		);
	}
	else
	{
		if (UListDataObject_Base* SelectedItem = CommonListView_OptionsList->GetSelectedItem<UListDataObject_Base>())
		{
			DetailsView_ListEntryInfo->UpdateDetailsViewInfo(
				SelectedItem,
				TryGetEntryWidgetClassName(SelectedItem)
			);
		}
	}
		
}

// ListView 的选中数据改变时调用；鼠标操作行内控件也会通过 SelectThisEntryWidget 触发它。
void UWidget_OptionsScreen::OnListViewItemSelected(UObject* InSelectedItem)
{
	if (!InSelectedItem)
	{
		return;
	}

	DetailsView_ListEntryInfo->UpdateDetailsViewInfo(
			CastChecked<UListDataObject_Base>(InSelectedItem),
			TryGetEntryWidgetClassName(InSelectedItem)
		);
}

// 数据存在不代表行控件已生成：ListView 按需维护行；这里的类名仅用于详情中的教学信息。
FString UWidget_OptionsScreen::TryGetEntryWidgetClassName(UObject* InOwningListItem) const
{
	if (UUserWidget* FoundEntryWidget = CommonListView_OptionsList->GetEntryWidgetFromItem(InOwningListItem))
	{
		return FoundEntryWidget->GetClass()->GetName();
	}

	return TEXT("Entry Widget Not Valid");
}

// 已订阅的数据对象同步广播修改通知时调用，来源可能是直接修改、恢复默认或依赖变化。
// 这里只维护 Reset 状态；bIsResettingData 避免批量恢复时改动正在遍历的同一个数组。
void UWidget_OptionsScreen::OnListViewListDataModified(UListDataObject_Base* ModifiedData, EOptionsListDataModifyReason ModifyReason)
{
	if (!ModifiedData || bIsResettingData)
	{
		return;
	}

	// 重新询问当前状态而不是简单标记“改过”：用户改回默认值时也应移出候选集合。
	if (ModifiedData->CanResetBackToDefaultValue())
	{
		ResettableDataArray.AddUnique(ModifiedData);

		if (!GetActionBindings().Contains(ResetActionHandle))
		{
			AddActionBinding(ResetActionHandle);
		}
	}
	else
	{
		if (ResettableDataArray.Contains(ModifiedData))
		{
			ResettableDataArray.Remove(ModifiedData);
		}
	}

	if (ResettableDataArray.IsEmpty())
	{
		RemoveActionBinding(ResetActionHandle);
	}
}
