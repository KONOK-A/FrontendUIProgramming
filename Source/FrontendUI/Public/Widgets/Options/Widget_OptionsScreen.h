// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "Widget_OptionsScreen.generated.h"

class UOptionsDataRegistry;
class UFrontendTabListWidgetBase;
class UFrontendCommonListView;
class UWidget_OptionsDetailsView;
class UListDataObject_Base;
/**
 * 设置页的调度者：向 Registry 要数据，将选中页的数据交给 ListView，并驱动右侧详情。
 * Registry 创建的数据对象保存选项规则；列表行只负责显示与接收操作，二者不是同一个对象。
 * 读流程时可从 NativeOnActivated 进入，再跟到 OnOptionsTabSelected 和各类 ListEntry。
 * 页面还统一接收 CommonUI 的 Reset/Back 动作，具体读写设置仍交给数据对象。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_OptionsScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()
	
protected:
	//~ Begin UUserWidget Interface
	/** UE 实例初始化入口；注册 Reset/Back 动作及页签、悬停和选择委托，每次激活无需重绑。 */
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin UCommonActivatableWidget Interface
	/** CommonUI 每次激活页面时调用；取得 Registry，并仅注册尚不存在的页签按钮。 */
	virtual void NativeOnActivated() override;

	/** CommonUI 停用入口；调用 ApplySettings(true) 应用设置，不恢复进入页面前的值。 */
	virtual void NativeOnDeactivated() override; 

	/**
	 * 为 CommonUI 提供默认焦点；列表选中数据与实际接收输入的行控件是不同状态。
	 * @return 已选中数据对应的现存行控件；尚未生成行或无选择时返回父类的焦点目标。
	 */
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	//~ End UCommonActivatableWidget Interface

private:
	/**
	 * 懒创建并初始化本页面的数据 Registry；激活和切页复用同一实例。
	 * 首次创建时传入拥有页面的 LocalPlayer，供 Control 页建立玩家键位数据。
	 * @return 页面持有的 Registry；创建结果必须有效，否则触发检查。
	 */
	UOptionsDataRegistry* GetOrCreateDataRegistry();

	/**
	 * 响应 CommonUI Reset 动作；候选为空时不处理，否则以当前页签名请求确认弹窗。
	 * 要求活动页签按钮为 UFrontendCommonButtonBase；确认后遍历回调执行时的候选数组。
	 * 逐项恢复会同步通知各行；期间只禁止本页面改动候选数组。全部成功才清空候选并移除动作。
	 */
	void OnResetBoundActionTriggered();

	/** 响应 CommonUI Back 动作并停用页面，随后由 NativeOnDeactivated 应用设置。 */
	void OnBackBoundActionTriggered();

	/**
	 * 响应 TabList 的动态选择委托；清空详情、替换列表数据并重建 Reset 候选与动作状态。
	 * 非空列表会请求导航并选中索引 0，是否允许选中仍由 ListView 的规则决定。
	 * 为新数据添加修改订阅，不解除旧页数据上的既有订阅。
	 * @param TabId Registry 中已注册页签集合的 DataID；找不到集合会触发 Registry 检查。
	 */
	UFUNCTION()
	void OnOptionsTabSelected(FName TabId);

	/**
	 * 响应 ListView 悬停变化；通知行蓝图，并临时预览悬停数据的详情，不改变列表选择。
	 * 离开时若存在选中项则恢复其详情；没有选中项时保留当前详情。
	 * @param InHoveredItem 悬停变化的数据；空值不处理，非空时要求为设置数据且已有对应行。
	 * @param bWasHovered true 表示鼠标进入，false 表示离开。
	 */
	void OnListViewItemHovered(UObject* InHoveredItem,bool bWasHovered);

	/**
	 * 根据 ListView 新选中的数据刷新详情，不负责转移键盘或手柄焦点。
	 * @param InSelectedItem 新选中的设置数据；空值不处理且不清空已有详情。
	 */
	void OnListViewItemSelected(UObject* InSelectedItem);

	/**
	 * 查询数据项当前对应的行类名，仅用于详情中的教学信息。
	 * @param InOwningListItem 交给 ListView 查找现存行的数据对象。
	 * @return 已生成行的实际类名；找不到行时返回 "Entry Widget Not Valid"。
	 */
	FString TryGetEntryWidgetClassName(UObject* InOwningListItem) const;

	/**
	 * 已订阅数据同步广播修改时，按其当前可重置状态增删 Reset 候选并同步动作绑定。
	 * 批量恢复期间立即返回以保护遍历；此处不检查数据是否仍属于当前页，行显示由行自身刷新。
	 * @param ModifiedData 发生变化的数据；空值不处理。
	 * @param ModifyReason 修改来源；本处理器对所有来源采用相同逻辑。
	 */
	void OnListViewListDataModified(UListDataObject_Base* ModifiedData,EOptionsListDataModifyReason ModifyReason);

	//***** Bound Widgets ***** //
	/** 子蓝图必须提供的同名页签列表；激活时注册按钮，选择委托驱动设置列表切页。 */
	UPROPERTY(meta = (BindWidget))
	UFrontendTabListWidgetBase* TabListWidget_OptionsTabs;

	/** 子蓝图必须提供的设置列表；持有数据对象并生成或复用对应类型的行控件。 */
	UPROPERTY(meta = (BindWidget))
	UFrontendCommonListView* CommonListView_OptionsList;

	/** 子蓝图必须提供的详情区域；悬停时临时预览，选择变化时展示选中数据。 */
	UPROPERTY(meta = (BindWidget))
	UWidget_OptionsDetailsView* DetailsView_ListEntryInfo;
	//***** Bound Widgets ***** //
	
	/** 经 GetOrCreateDataRegistry 首次创建后复用的数据树入口；UPROPERTY 保持引用，Transient 不序列化。 */
	UPROPERTY(Transient)
	UOptionsDataRegistry* CreatedOwningDataRegistry;

	/** 蓝图类默认值中配置的 CommonUI Reset 动作数据表行；为空时初始化阶段不注册该动作。 */
	UPROPERTY(EditDefaultsOnly, Category = "Frontend Options Screen", meta = (RowType = "/Script/CommonUI.CommonInputActionDataBase"))
	FDataTableRowHandle ResetAction;

	/** 初始化时注册 Reset 得到的句柄；候选增删时据此移除或恢复绑定，无需重建回调。 */
	FUIActionBindingHandle ResetActionHandle;

	/** 切页时按 CanResetBackToDefaultValue 重建，后续订阅通知会增删的候选；确认回调直接遍历此数组。 */
	UPROPERTY(Transient)
	TArray<UListDataObject_Base*> ResettableDataArray;

	/** 批量恢复期间保护候选数组；数据对象仍同步广播、刷新行并按自身规则应用设置。 */
	bool bIsResettingData = false;
};
