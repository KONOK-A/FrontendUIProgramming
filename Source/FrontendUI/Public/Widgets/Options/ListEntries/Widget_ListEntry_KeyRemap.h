// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/ListEntries/Widget_ListEntry_Base.h"
#include "Widget_ListEntry_KeyRemap.generated.h"

class UFrontendCommonButtonBase;
class UListDataObject_KeyRemap;
/**
 * Control 页的键位行，由 KeyRemap 数据对象提供当前按键图标和要捕获的设备类型。
 * 点击重映射后异步打开 KeyRemapScreen；弹窗返回按键，当前行再调用数据对象提交映射。
 * 恢复默认按钮只处理当前映射；页面级 Reset 则会遍历它收集的多个数据对象。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_ListEntry_KeyRemap : public UWidget_ListEntry_Base
{
	GENERATED_BODY()
	
protected:
	//~ Begin UUserWidget Interface
	/** UE 实例初始化入口；连接重映射和单行恢复默认按钮，点击时再读取当前缓存数据。 */
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface
	
	//~ Begin UWidget_ListEntry_Base Interface
	/**
	 * 保留基类绑定后缓存键位数据，并从当前映射读取图标供重映射按钮展示。
	 * @param InOwningListDataObject 必须为已初始化的非空 UListDataObject_KeyRemap，使用 CastChecked 验证类型。
	 */
	virtual void OnOwningListDataObjectSet(UListDataObject_Base* InOwningListDataObject) override;

	/**
	 * 从当前缓存键位重新查询图标；修改映射和恢复默认共用此路径，缓存为空时不处理。
	 * @param OwningModifiedData 通知中的修改对象；此实现不据此替换缓存或过滤旧订阅。
	 * @param ModifyReason 修改来源；此实现不按来源分支。
	 */
	virtual void OnOwningListDataObjectModified(UListDataObject_Base* OwningModifiedData,EOptionsListDataModifyReason ModifyReason) override;
	//~ End UWidget_ListEntry_Base Interface

private:
	/**
	 * 先选中本行，再异步请求 Modal 层的捕获页面；页面创建后、入栈前绑定结果并设置目标设备。
	 * 要求 UI 子系统、软类映射及页面类型有效；行及其对应数据应在异步回调期间保持有效。
	 */
	void OnRemapKeyButtonClicked();

	/**
	 * 选中本行并检查能否恢复当前映射；无缓存时返回，已为默认值时提示，否则请求 Yes/No 确认。
	 * 确认后使用届时缓存的数据尝试恢复，不检查恢复结果；弹窗等待期间须保持行和数据有效。
	 */
	void OnResetKeyBindingButtonClicked();

	/**
	 * 捕获弹窗延迟回传有效键时，交给当前缓存数据提交映射、保存并广播；无缓存时不处理。
	 * @param PressedKey 捕获页面筛选或转换后的键，不是此函数直接接收的底层输入事件。
	 */
	void OnKeyToRemapPressed(const FKey& PressedKey);

	/**
	 * 捕获弹窗回传取消结果时异步请求 OK 提示；此路径不修改键位数据。
	 * @param CanceledReason 捕获页面提供的取消原因，直接用作提示正文。
	 */
	void OnKeyRemapCanceled(const FString& CanceledReason);

	//***** Bound Widgets ***** //
	/** 行蓝图必须提供的重映射按钮；显示当前键图标，点击后请求捕获弹窗。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget,AllowPrivateAccess = "true"))
	UFrontendCommonButtonBase* CommonButton_RemapKey;

	/** 行蓝图必须提供的恢复默认按钮；仅请求重置当前映射，与页面批量 Reset 分开。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget,AllowPrivateAccess = "true"))
	UFrontendCommonButtonBase* CommonButton_ResetKeyBinding;
	//***** Bound Widgets ***** //

	/** 最近一次分配的键位数据；异步弹窗回调也使用此缓存，因此等待结果期间应避免行切换到其他数据。 */
	UPROPERTY(Transient)
	UListDataObject_KeyRemap* CachedOwningKeyRemapDataObject;
};
