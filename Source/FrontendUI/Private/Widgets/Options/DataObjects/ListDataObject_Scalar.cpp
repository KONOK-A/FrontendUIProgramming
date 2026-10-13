// Vince Petrelli All Rights Reserved


#include "Widgets/Options/DataObjects/ListDataObject_Scalar.h"
#include "Widgets/Options/OptionsDataInteractionHelper.h"

FCommonNumberFormattingOptions UListDataObject_Scalar::NoDecimal()
{
	FCommonNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = 0;

	return Options;
}

FCommonNumberFormattingOptions UListDataObject_Scalar::WithDecimal(int32 NumFracDigit)
{
	FCommonNumberFormattingOptions Options;
	Options.MaximumFractionalDigits = NumFracDigit;

	return Options;
}

// Scalar 行首次绑定以及收到修改通知时都会读取此值，供滑条位置和数字文本同步使用。
// getter 返回实际输出值，读入时映射到显示区间；滑条写回时做反向映射并限制范围。
// 例如 OverallVolume 输出 1，在 0..2 中位于一半，对应显示值 0.5，再格式化为 50%。
float UListDataObject_Scalar::GetCurrentValue() const
{	
	if (DataDynamicGetter)
	{
		return FMath::GetMappedRangeValueClamped(
			OutputValueRange,
			DisplayValueRange,
			StringToFloat(DataDynamicGetter->GetValueAsString())
		);
	}

	return 0.0f;
}

// 行控件 OnSliderValueChanged 传入显示区间的数值；例如显示 0.75，音量输出范围 0..2 时写入 1.5。
// 百分号和小数位只由文本控件格式化，这里传递和计算的始终是 float。
void UListDataObject_Scalar::SetCurrentValueFromSlider(float InNewValue)
{
	if (DataDynamicSetter)
	{	
		const float ClampedValue = FMath::GetMappedRangeValueClamped(
			DisplayValueRange,
			OutputValueRange,
			InNewValue
		);

		// 先写入设置，再通知界面；是否立即 Apply 由 Base 中的开关决定。
		DataDynamicSetter->SetValueFromString(LexToString(ClampedValue));

		NotifyListDataModified(this);
	}
}

bool UListDataObject_Scalar::CanResetBackToDefaultValue() const
{	
	if (HasDefaultValue() && DataDynamicGetter)
	{	
		// 默认值采用输出区间；用容差比较，避免浮点误差让 Reset 一直可用。
		const float DefaultValue = StringToFloat(GetDefaultValueAsString());
		const float CurrentValue = StringToFloat(DataDynamicGetter->GetValueAsString());

		return !FMath::IsNearlyEqual(DefaultValue,CurrentValue,0.01f);
	}

	return false;
}

// 默认字符串已经处于输出区间，因此 Reset 直接交给 setter，不再做一次滑条区间换算。
bool UListDataObject_Scalar::TryResetBackToDefaultValue()
{
	if (CanResetBackToDefaultValue())
	{
		if (DataDynamicSetter)
		{
			DataDynamicSetter->SetValueFromString(GetDefaultValueAsString());

			NotifyListDataModified(this,EOptionsListDataModifyReason::ResetToDefault);

			return true;
		}
	}

	return false;
}

void UListDataObject_Scalar::OnEditDependencyDataModified(UListDataObject_Base* ModifiedDependencyData, EOptionsListDataModifyReason ModifyReason)
{
	// 例如总体画质改变分辨率比例：通知行重新通过 getter 读取，不在此重复写 setter。
	NotifyListDataModified(this,EOptionsListDataModifyReason::DependencyModified);

	// 继续执行 Base 的依赖通知，让行控件在刷新数值之外，也能重新判断可编辑状态。
	Super::OnEditDependencyDataModified(ModifiedDependencyData,ModifyReason);
}

// Helper 统一返回 FString；Scalar 在数值比较与范围映射前用 LexFromString 转成 float。
float UListDataObject_Scalar::StringToFloat(const FString& InString) const
{	
	float OutConvertedValue = 0.f;

	LexFromString(OutConvertedValue,*InString);

	return OutConvertedValue;
}


