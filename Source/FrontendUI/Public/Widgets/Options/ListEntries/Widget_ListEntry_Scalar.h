// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/ListEntries/Widget_ListEntry_Base.h"
#include "Widget_ListEntry_Scalar.generated.h"

class UCommonNumericTextBlock;
class UAnalogSlider;
class UListDataObject_Scalar;
/**
 * 连续数值选项行，用于音量、亮度等设置；同一份 Scalar 数据同时驱动滑条和数值文本。
 * 滑条处理的是显示范围，数据对象负责换算为设置的输出范围，例如音量显示值与实际倍率。
 * 用户拖动会写入数据，数据通知再更新两种显示，外部恢复默认也走这条刷新路径。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_ListEntry_Scalar : public UWidget_ListEntry_Base
{
	GENERATED_BODY()
	
protected:
	//~ Begin UUserWidget Interface
	/** UE 实例初始化入口；绑定滑条值变化及鼠标捕获开始的动态委托，具体范围和数值随后由数据提供。 */
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

	//~ Begin UWidget_ListEntry_Base Interface
	/**
	 * 保留基类绑定后缓存 Scalar 数据，并设置数值格式、滑条显示范围、步长及当前显示值。
	 * @param InOwningListDataObject 必须为非空 UListDataObject_Scalar，使用 CastChecked 验证类型。
	 */
	virtual void OnOwningListDataObjectSet(UListDataObject_Base* InOwningListDataObject) override;

	/**
	 * 从当前缓存重新读取显示值，同时刷新数值文本与滑条；缓存为空时不处理。
	 * @param OwningModifiedData 通知中的修改对象；此实现不据此替换缓存或过滤旧订阅。
	 * @param ModifyReason 修改来源；直接修改、恢复默认与依赖变化都采用相同刷新逻辑。
	 */
	virtual void OnOwningListDataObjectModified(UListDataObject_Base* OwningModifiedData,EOptionsListDataModifyReason ModifyReason) override;
	//~ End UWidget_ListEntry_Base Interface

private:
	/**
	 * 响应滑条动态委托；有缓存时把显示值交给数据对象转换并写入设置，通知再驱动界面刷新。
	 * @param Value 滑条显示范围内的数值，不是设置的实际输出值。
	 */
	UFUNCTION()
	void OnSliderValueChanged(float Value);

	/** 鼠标开始被滑条捕获用于拖动时，选中所属列表的当前数据，联动高亮和详情。 */
	UFUNCTION()
	void OnSliderMouseCaptureBegin();

	//***** Bound Widgets ***** //
	/** 行蓝图必须提供的数值控件；数据决定数值类型和小数格式，样式由蓝图配置。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget,AllowPrivateAccess = "true"))
	UCommonNumericTextBlock* CommonNumeric_SettingValue;

	/** 行蓝图必须提供的滑条；使用数据的显示范围和步长，显示值与实际设置的换算交由数据对象。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget,AllowPrivateAccess = "true"))
	UAnalogSlider* AnalogSlider_SettingSlider;
	//***** Bound Widgets ***** //

	/** 最近一次分配的 Scalar 数据；供拖动提交与通知回读共用，行复用时替换。 */
	UPROPERTY(Transient)
	UListDataObject_Scalar* CachedOwningScalarDataObject;
};
