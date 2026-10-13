// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonListView.h"
#include "FrontendCommonListView.generated.h"

class UDataAsset_DataListEntryMapping;

/**
 * 设置页的列表容器：接收 OptionsScreen 提供的数据对象，按其类型选择显示该数据的行控件。
 * 数据对象保存“这是什么设置、当前值是什么”；行控件负责“怎样展示、怎样接收用户操作”。
 * 数据类型到行类的规则来自 DataListEntryMapping，生成后的行由 NativeOnListItemObjectSet 接收数据。
 * 条目实例由 CommonUI 列表机制管理，滚动/刷新时可复用，不应假定每个数据对象永久对应一个控件。
 * 集合数据在扁平列表中充当分组标题，本类禁止把它作为可选中或可导航到的设置项。
 */
UCLASS()
class FRONTENDUI_API UFrontendCommonListView : public UCommonListView
{
	GENERATED_BODY()
	
protected:
	//~ Begin UCommonListView Interface
	/**
	 * 运行时按数据对象的类型映射生成行；设计器预览及无匹配映射时使用父类流程。
	 * @param Item 当前数据项；运行时须非空且派生自 UListDataObject_Base，类型由 CastChecked 校验。
	 * @param DesiredEntryClass 交给父类回退流程的默认条目类。
	 * @param OwnerTable 接收生成条目的底层 Slate 表格。
	 * @return 列表生成或复用的条目控件引用，其生命周期仍由列表管理。
	 * @note 运行时必须配置 DataListEntryMapping；无匹配行类可回退，未配置映射资产不能回退。
	 */
	virtual UUserWidget& OnGenerateEntryWidgetInternal(UObject* Item, TSubclassOf<UUserWidget> DesiredEntryClass, const TSharedRef<STableViewBase>& OwnerTable) override;
	/**
	 * 让集合数据作为分组标题显示，同时排除其选中及导航行为。
	 * @param FirstSelectedItem 列表查询的非空数据项；实现直接访问其类型。
	 * @return 非 UListDataObject_Collection 类型时为 true；不移除集合项，也不追加父类判定。
	 */
	virtual bool OnIsSelectableOrNavigableInternal(UObject* FirstSelectedItem) override;
	//~ End UCommonListView Interface

private:
	//~ Begin UWidget Interface
#if WITH_EDITOR	
	/**
	 * 编译控件蓝图时检查行映射资产是否配置，不替代运行时的有效性前提。
	 * @param CompileLog 接收父类诊断及缺失 DataListEntryMapping 的编译错误。
	 */
	virtual void ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const override;
#endif
	//~ End UWidget Interface

	/**
	 * 必须配置的行类映射资产，例如 String 对应轮换行、Scalar 对应滑块行。
	 * 映射按数据实际类型及父类查找，只决定如何展示；当前数据项仍由 OptionsScreen 提供。
	 */
	UPROPERTY(EditAnywhere, Category = "Frontend List View Settings")
	UDataAsset_DataListEntryMapping* DataListEntryMapping;
};
