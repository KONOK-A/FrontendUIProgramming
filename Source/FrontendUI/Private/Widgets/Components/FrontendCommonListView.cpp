// Vince Petrelli All Rights Reserved


#include "Widgets/Components/FrontendCommonListView.h"
#include "Editor/WidgetCompilerLog.h"
#include "Widgets/Options/DataAsset_DataListEntryMapping.h"
#include "Widgets/Options/ListEntries/Widget_ListEntry_Base.h"
#include "Widgets/Options/DataObjects/ListDataObject_Base.h"
#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"

// 列表系统需要一个可见行时调用此入口。Item 是数据对象，DesiredEntryClass 是默认行类，
// OwnerTable 是底层 Slate 表格；此处只改选类规则，实例管理仍交回列表的 GenerateTypedEntry。
UUserWidget& UFrontendCommonListView::OnGenerateEntryWidgetInternal(UObject* Item, TSubclassOf<UUserWidget> DesiredEntryClass, const TSharedRef<STableViewBase>& OwnerTable)
{
	// 设计器预览不要求拥有真实运行时设置数据，沿用父类生成方式。
	if (IsDesignTime())
	{
		return Super::OnGenerateEntryWidgetInternal(Item,DesiredEntryClass,OwnerTable);
	}

	// 运行时约定 Item 必须是 UListDataObject_Base 派生对象；CastChecked 在类型不符时暴露错误。
	// 映射资产先找实际数据类型，再向父类查找，因此 Bool/Enum 等数据可复用 String 对应的行。
	// 生成行后，列表会将数据交给该行的 IUserObjectListEntry 接口，不在这里手动填每个子控件。
	if (TSubclassOf<UWidget_ListEntry_Base> FoundWidgetClass = DataListEntryMapping->FindEntryWidgetClassByDataObject(CastChecked<UListDataObject_Base>(Item)))
	{
		return GenerateTypedEntry<UWidget_ListEntry_Base>(FoundWidgetClass,OwnerTable);
	}
	else
	{
		// 没有对应映射时使用默认行类；与“映射资产本身没配置”不同，后者由编辑器检查报告。
		return Super::OnGenerateEntryWidgetInternal(Item,DesiredEntryClass,OwnerTable);
	}

	
}

bool UFrontendCommonListView::OnIsSelectableOrNavigableInternal(UObject* FirstSelectedItem)
{
	// 由列表的选中/导航流程查询。集合项仍可生成并显示标题行，但不接受选中或导航停留。
	// 因而手柄导航处理的是实际设置项；这里没有把集合数据从列表源中删除。
	return !FirstSelectedItem->IsA<UListDataObject_Collection>();
}

#if WITH_EDITOR	
void UFrontendCommonListView::ValidateCompiledDefaults(IWidgetCompilerLog& CompileLog) const
{
	Super::ValidateCompiledDefaults(CompileLog);

	// WITH_EDITOR 将此检查限定在编辑器构建；编译控件蓝图时尽早报告漏配的映射资产。
	// 这不是运行时的空值回退：上面的运行时生成流程仍以资产已配置为前提。
	if (!DataListEntryMapping)
	{
		CompileLog.Error(FText::FromString(
		TEXT("The variable DataListEntryMapping has no valid data asset assigned ") +
		GetClass()->GetName() +
		TEXT(" needs a valid data asset to function properly")
		));
	}
}
#endif