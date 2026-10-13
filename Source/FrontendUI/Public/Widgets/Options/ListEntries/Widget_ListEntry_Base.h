// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "Widget_ListEntry_Base.generated.h"

class UCommonTextBlock;
class UListDataObject_Base;
/**
 * 设置列表行的公共基类：ListView 持有 DataObject 数据，再按需生成/复用这些显示控件。
 * IUserObjectListEntry 是接收列表数据的接口；每次分配数据时会调用 NativeOnListItemObjectSet。
 * 数据对象定义值和规则，行控件订阅它的通知并刷新 UI；同一个行实例可以先后显示不同数据。
 * 三种状态要分开：选中是列表记录的当前数据，悬停是鼠标所在行，焦点是键盘/手柄输入目标。
 * 本类连接这些状态与蓝图事件；蓝图负责具体高亮表现，以及返回行内的手柄焦点控件。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_ListEntry_Base : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
	
public:
	/**
	 * 供子蓝图实现的悬停表现事件；由自定义悬停入口调用，C++ 不在此定义具体视觉效果。
	 * @param bWasHovered true 表示进入，false 表示离开或行释放时清理悬停。
	 * @param bIsEntryWidgetStillSelected 行是否仍是列表选中项；没有分配数据时为 false。
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On List Entry Widget Hovered"))
	void BP_OnListEntryWidgetHovered(bool bWasHovered,bool bIsEntryWidgetStillSelected);

	/**
	 * 项目自定义的悬停入口，由 OptionsScreen 或行释放回调调用，并非 UE 自动调用的生命周期重写。
	 * 先通知蓝图悬停状态，再切换高亮；离开时仍被选中的行继续高亮。
	 * @param bWasHovered true 表示进入悬停，false 表示结束悬停。
	 */
	void NativeOnListEntryWidgetHovered(bool bWasHovered);

protected:
	/**
	 * 供子蓝图选择真正接收手柄操作的行内控件，例如 Rotator 或 Slider。
	 * @return 期望获得手柄焦点的控件；未提供目标或其 Slate 控件尚未缓存时沿用父类焦点处理。
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "Get Widget To Focus For Gamepad"))
	UWidget* BP_GetWidgetToFocusForGamepad() const;
	
	/**
	 * 供子蓝图绘制高亮；由选择变化或自定义悬停入口调用，不负责改变 ListView 选择和输入焦点。
	 * @param bShouldHighlight 此次回调要求的高亮状态。
	 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Toggle Entry Widget Highlight State"))
	void BP_OnToggleEntryWidgetHighlightState(bool bShouldHighlight) const;

	//~ Begin IUserObjectListEntry Interface
	/**
	 * ListView 每次为行分配数据时调用；保留接口处理后转入项目自定义的数据绑定入口。
	 * @param ListItemObject 必须为非空 UListDataObject_Base 派生对象，使用 CastChecked 验证类型。
	 */
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;

	/**
	 * ListView 通知选择变化时更新本行高亮；页面详情由 OptionsScreen 的独立订阅刷新。
	 * @param bIsSelected 此行当前是否被列表选中。
	 */
	virtual void NativeOnItemSelectionChanged(bool bIsSelected) override;

	/** ListView 释放行时刷新悬停表现；行仍可复用，此实现不清空缓存或解绑旧数据通知。 */
	virtual void NativeOnEntryReleased() override;
	//~ End IUserObjectListEntry Interface

	//~ Begin UUserWidget Interface
	/**
	 * 行接收焦点时，若当前为手柄输入则尝试转交给蓝图指定的行内 Slate 控件。
	 * @param InGeometry 当前行几何信息；未转交焦点时传给父类。
	 * @param InFocusEvent UE 派发的焦点事件；未转交焦点时传给父类。
	 * @return 取得目标的 Slate 控件时返回带 SetUserFocus 请求的 Handled 回复；其余情况返回父类结果。
	 */
	virtual FReply NativeOnFocusReceived( const FGeometry& InGeometry, const FFocusEvent& InFocusEvent ) override;
	//~ End UUserWidget Interface
	
	/**
	 * 数据分配的自定义扩展点；更新可选标题、订阅值和依赖通知、求编辑条件并缓存当前数据。
	 * 编辑条件求值可能强制写入设置并同步广播；只防止本数据重复订阅，不解除旧数据订阅。
	 * 派生类应先调用 Super 保留公共处理，再初始化自己的类型缓存和操作控件。
	 * @param InOwningListDataObject 本次分配的非空设置数据；派生行还要求匹配各自的数据类型。
	 */
	virtual void OnOwningListDataObjectSet(UListDataObject_Base* InOwningListDataObject);

	/**
	 * 已订阅数据广播值变化时调用的显示刷新扩展点；基类为空，派生实现无须调用 Super。
	 * @param OwningModifiedData 通知中的修改对象；行复用后仍可能收到旧订阅通知。
	 * @param ModifyReason 直接修改、恢复默认或依赖修改的来源标记。
	 */
	virtual void OnOwningListDataObjectModified(UListDataObject_Base* OwningModifiedData,EOptionsListDataModifyReason ModifyReason);

	/**
	 * 收到依赖通知后重算当前缓存数据的编辑条件，并通过 OnToggleEditableState 更新界面。
	 * 缓存为空时不处理；求条件可能应用强制值，例如窗口模式变化限制分辨率。
	 * @param OwningModifiedDependencyData 发生变化的依赖数据；此实现不以它替换本行缓存。
	 * @param ModifyReason 依赖修改的来源标记；此实现不按来源分支。
	 */
	virtual void OnOwningDependencyDataObjectModified(UListDataObject_Base* OwningModifiedDependencyData,EOptionsListDataModifyReason ModifyReason);

	/**
	 * 更新可选标题的启用状态；需要限制操作控件的派生类应先调用 Super，再更新对应控件。
	 * @param bIsEditable 当前数据是否通过编辑条件；基类只控制标题，不自动禁用整行。
	 */
	virtual void OnToggleEditableState(bool bIsEditable);

	/** 将本行数据设为所属 UListView 的选中项；要求有效的所属列表，本入口不主动设置输入焦点。 */
	void SelectThisEntryWidget();

private:
	//***** Bound Widgets ***** //
	/** 子蓝图可省略的设置标题；绑定数据时显示名称，求编辑条件后更新启用状态，使用前须检查。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional,AllowPrivateAccess = "true"))
	UCommonTextBlock* CommonText_SettingDisplayName;
	//***** Bound Widgets ***** //

	/**
	 * 当前分配的数据，依赖通知到来时用于重算本行条件；复用时替换，释放行时不清空。
	 * UPROPERTY 保留可追踪的 UObject 引用，Transient 不写入资产。
	 */
	UPROPERTY(Transient)
	UListDataObject_Base* CachedOwningDataObject;
};
