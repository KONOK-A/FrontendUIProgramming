// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/ListEntries/Widget_ListEntry_Base.h"
#include "Widget_ListEntry_String.generated.h"

class UFrontendCommonButtonBase;
class UFrontendCommonRotator;
class UListDataObject_String;

/**
 * 展示有限候选值的选项行，例如难度、布尔开关和画质枚举，均可由 String 系列数据驱动。
 * Rotator 是在一组标签之间切换的控件；鼠标使用前后按钮，手柄变化经 Rotator 回调提交。
 * 行只负责把操作交给数据对象，再根据数据修改通知刷新标签，不直接访问 GameUserSettings。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_ListEntry_String : public UWidget_ListEntry_Base
{
	GENERATED_BODY()
	
protected:
	//~ Begin UUserWidget Interface
	/** UE 实例初始化入口；绑定前后按钮、Rotator 点击及旋转事件，具体数据在分配行时设置。 */
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin UWidget_ListEntry_Base Interface
	/**
	 * 保留基类绑定后缓存 String 数据，并同时替换 Rotator 的候选标签和当前显示。
	 * @param InOwningListDataObject 必须为非空 UListDataObject_String 派生对象，使用 CastChecked 验证。
	 */
	virtual void OnOwningListDataObjectSet(UListDataObject_Base* InOwningListDataObject) override;

	/**
	 * 数据通知到来时，从当前缓存重新读取显示文本并更新 Rotator；缓存为空时不处理。
	 * @param OwningModifiedData 通知中的修改对象；此实现不据此替换缓存或过滤旧订阅。
	 * @param ModifyReason 修改来源；所有来源采用同一显示刷新路径。
	 */
	virtual void OnOwningListDataObjectModified(UListDataObject_Base* OwningModifiedData,EOptionsListDataModifyReason ModifyReason) override;

	/**
	 * 保留基类标题处理后，同步前后按钮和 Rotator 的启用状态。
	 * @param bIsEditable 是否允许操作当前选项。
	 */
	virtual void OnToggleEditableState(bool bIsEditable) override;
	//~ End UWidget_ListEntry_Base Interface

private:
	/** 向当前缓存数据请求前一候选，再选中本行；没有缓存时仍同步列表选择。 */
	void OnPreviousOptionButtonClicked();

	/** 向当前缓存数据请求后一候选，再选中本行；值写入和通知由数据对象负责。 */
	void OnNextOptionButtonClicked();

	/**
	 * 仅在缓存和输入子系统有效、当前输入为手柄且属于用户操作时，向数据对象提交选中标签。
	 * 程序刷新引发的事件被忽略，避免显示回写；鼠标切换由前后按钮处理。
	 * @param Value Rotator 事件提供的索引；此实现不使用它，而是读取控件的当前标签。
	 * @param bUserInitiated 是否由用户操作发起，而非程序更新。
	 */
	void OnRotatorValueChanged(int32 Value, bool bUserInitiated);

	//***** Bound Widgets ***** //
	/** 行蓝图必须提供的前一项按钮；点击后请求数据切换并同步列表选择。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget,AllowPrivateAccess = "true"))
	UFrontendCommonButtonBase* CommonButton_PreviousOption;

	/** 行蓝图必须提供的候选控件；展示 FText 标签，用户手柄旋转经数据对象转换为实际设置值。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget,AllowPrivateAccess = "true"))
	UFrontendCommonRotator* CommonRotator_AvailableOptions;

	/** 行蓝图必须提供的后一项按钮；与前一项按钮共用数据通知驱动的显示刷新。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget,AllowPrivateAccess = "true"))
	UFrontendCommonButtonBase* CommonButton_NextOption;
	//***** Bound Widgets ***** //

	/** 最近一次分配的 String 数据；输入回调写入它，数据通知从它读取当前显示，行复用时替换。 */
	UPROPERTY(Transient)
	UListDataObject_String* CachedOwningStringDataObject;
};
