// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/DataObjects/ListDataObject_Value.h"
#include "CommonNumericTextBlock.h"
#include "ListDataObject_Scalar.generated.h"

/**
 * 滑条数据：在界面显示区间与设置实际输出区间之间转换浮点值。
 * Widget_ListEntry_Scalar 用它配置滑条与数字文本；拖动时传入显示值，Helper 向设置写入输出值。
 * 它没有 String 那样的当前值缓存：GetCurrentValue 每次通过 getter 读取，因此能显示画质联动后的值。
 */
UCLASS()
class FRONTENDUI_API UListDataObject_Scalar : public UListDataObject_Value
{
	GENERATED_BODY()

public:
	LIST_DATA_ACCESSOR(TRange<float>,DisplayValueRange)
	LIST_DATA_ACCESSOR(TRange<float>,OutputValueRange)
	LIST_DATA_ACCESSOR(float,SliderStepSize)
	LIST_DATA_ACCESSOR(ECommonNumericType,DisplayNumericType)
	LIST_DATA_ACCESSOR(FCommonNumberFormattingOptions,NumberFormattingOptions)

	/**
	 * 创建不显示小数的数字文本格式，仅设置最大小数位数，不改变滑条或设置的数值精度。
	 * @return MaximumFractionalDigits 为 0、其余成员保留默认值的格式配置。
	 */
	static FCommonNumberFormattingOptions NoDecimal();
	/**
	 * 创建指定最大小数位数的数字文本格式，不改变实际数值精度。
	 * @param NumFracDigit 交给文本格式化器的最大小数位数；本函数不校验范围。
	 * @return 仅 MaximumFractionalDigits 使用传入值、其余成员保留默认值的格式配置。
	 */
	static FCommonNumberFormattingOptions WithDecimal(int32 NumFracDigit);

	/**
	 * 每次通过 getter 读取输出值，再钳制并映射到显示区间，不使用当前值缓存。
	 * 使用映射前须配置有上下界的有效区间；字符串解析不向调用方提供失败标志。
	 * @return 映射后的显示数值；没有 getter 时返回 0，不保证该回退值处于自定义显示区间。
	 */
	float GetCurrentValue() const;
	/**
	 * 把显示值钳制映射到输出区间，转换为字符串后调用 setter 并广播直接修改。
	 * 没有 setter 时不处理；本入口不检查编辑条件或新旧值是否相同。
	 * @param InNewValue 滑条提交的显示区间数值，单位与 DisplayValueRange 一致。
	 */
	void SetCurrentValueFromSlider(float InNewValue);

private:
	//~ Begin UListDataObject_Base Interface
	/**
	 * 使用输出值的单位比较默认字符串与 getter 当前值，容差为 0.01。
	 * @return 已配置默认值与 getter，且两值不近似相等时为 true；不检查 setter。
	 */
	virtual bool CanResetBackToDefaultValue() const override;
	/**
	 * 可重置且有 setter 时直接写入默认输出字符串，不再执行显示到输出的映射，并通知 ResetToDefault。
	 * @return 执行了 setter 和通知路径时为 true，不验证底层写入；无需重置或缺少所需 Helper 时为 false。
	 */
	virtual bool TryResetBackToDefaultValue() override;
	/**
	 * 广播本项的 DependencyModified，使行控件重新回读 getter，再由基类转发依赖事件。
	 * 不在此写入 setter；本项开启立即应用时，该广播路径仍会触发 ApplySettings。
	 * @param ModifiedDependencyData 原始依赖源，由基类原样转发。
	 * @param ModifyReason 原始通知原因，由基类原样转发。
	 */
	virtual void OnEditDependencyDataModified(UListDataObject_Base* ModifiedDependencyData,EOptionsListDataModifyReason ModifyReason) override;
	//~ End UListDataObject_Base Interface

	/**
	 * 通过 LexFromString 把 Helper 返回的字符串转换为浮点值，不提供解析成功标志。
	 * @param InString 预期为浮点数字符串，解释单位由当前操作使用的区间决定。
	 * @return 初始为 0 的输出变量经 LexFromString 处理后的值，不附加范围校验。
	 */
	float StringToFloat(const FString& InString) const;

	/**
	 * 滑条及数字文本使用的显示区间，默认 0..1；Percentage 文本格式下 1 表示 100%。
	 */
	TRange<float> DisplayValueRange = TRange<float>(0.f,1.f);
	/**
	 * getter、setter 与默认字符串使用的实际值区间，默认 0..1；具体单位由所连接设置决定。
	 */
	TRange<float> OutputValueRange = TRange<float>(0.f,1.f);
	/**
	 * 传给滑条 SetStepSize 的步长，默认 0.1；本类不按步长量化写入值，也不校验范围。
	 */
	float SliderStepSize = 0.1f;
	/**
	 * 只决定数字文本如何显示，默认 Number，不改变读写区间映射。
	 */
	ECommonNumericType DisplayNumericType = ECommonNumericType::Number;
	/**
	 * 交给 CommonNumericTextBlock 的格式配置；仅影响文案的小数位等显示细节。
	 */
	FCommonNumberFormattingOptions NumberFormattingOptions;
};
