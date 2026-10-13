// Vince Petrelli All Rights Reserved


#include "Widgets/Options/ListEntries/Widget_ListEntry_Scalar.h"
#include "Widgets/Options/DataObjects/ListDataObject_Scalar.h"
#include "AnalogSlider.h"

void UWidget_ListEntry_Scalar::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 实例初始化时连接两类事件：值变化提交数据，开始拖动同步列表选择。
	// Slider 使用动态委托，因此绑定目标在头文件中声明为 UFUNCTION；具体值在分配数据后填入。
	AnalogSlider_SettingSlider->OnValueChanged.AddUniqueDynamic(this,&ThisClass::OnSliderValueChanged);
	AnalogSlider_SettingSlider->OnMouseCaptureBegin.AddUniqueDynamic(this,&ThisClass::OnSliderMouseCaptureBegin);
}

void UWidget_ListEntry_Scalar::OnOwningListDataObjectSet(UListDataObject_Base* InOwningListDataObject)
{
	Super::OnOwningListDataObjectSet(InOwningListDataObject);

	CachedOwningScalarDataObject = CastChecked<UListDataObject_Scalar>(InOwningListDataObject);

	// 先用当前数据配置文本格式（如百分比、小数位）和当前值，再配置滑条的范围与步长。
	// 这些规则必须随数据重新设置：同一个复用行可能先显示音量，之后显示亮度。
	CommonNumeric_SettingValue->SetNumericType(CachedOwningScalarDataObject->GetDisplayNumericType());
	CommonNumeric_SettingValue->FormattingSpecification = CachedOwningScalarDataObject->GetNumberFormattingOptions();
	CommonNumeric_SettingValue->SetCurrentValue(CachedOwningScalarDataObject->GetCurrentValue());

	AnalogSlider_SettingSlider->SetMinValue(CachedOwningScalarDataObject->GetDisplayValueRange().GetLowerBoundValue());
	AnalogSlider_SettingSlider->SetMaxValue(CachedOwningScalarDataObject->GetDisplayValueRange().GetUpperBoundValue());
	AnalogSlider_SettingSlider->SetStepSize(CachedOwningScalarDataObject->GetSliderStepSize());
	AnalogSlider_SettingSlider->SetValue(CachedOwningScalarDataObject->GetCurrentValue());
}

// 数据对象广播修改后，文本与滑条一起重新读取当前显示值，保持两个控件一致。
void UWidget_ListEntry_Scalar::OnOwningListDataObjectModified(UListDataObject_Base* OwningModifiedData, EOptionsListDataModifyReason ModifyReason)
{
	if (CachedOwningScalarDataObject)
	{
		CommonNumeric_SettingValue->SetCurrentValue(CachedOwningScalarDataObject->GetCurrentValue());
		AnalogSlider_SettingSlider->SetValue(CachedOwningScalarDataObject->GetCurrentValue());
	}
}

void UWidget_ListEntry_Scalar::OnSliderValueChanged(float Value)
{
	if (CachedOwningScalarDataObject)
	{
		// 把显示范围内的值交给数据对象换算、写回设置并广播；这里不直接操作设置 getter/setter。
		// 通知会回到 OnOwningListDataObjectModified，完成“用户操作 → 数据 → 显示”的一轮更新。
		CachedOwningScalarDataObject->SetCurrentValueFromSlider(Value);
	}
}

void UWidget_ListEntry_Scalar::OnSliderMouseCaptureBegin()
{
	SelectThisEntryWidget();
}
