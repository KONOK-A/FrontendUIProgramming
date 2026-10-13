#pragma once

#include "FrontendStructTypes.generated.h"

/**
 * 一条设置项编辑规则，组合判断函数、禁用原因和可选强制值。
 * Registry 负责组装，ListDataObject_Base::IsDataCurrentlyEditable 负责求值并处理失败结果。
 * 例如 VSync 要求全屏；未满足时说明原因并尝试强制为 false，描述符本身不写入设置。
 */
USTRUCT()
struct FOptionsDataEditConditionDescriptor
{
	GENERATED_BODY()

public:
	/**
	 * 保存以后查询时执行的条件逻辑，此时不计算结果。
	 * @param InEditConditionFunc - 返回 true 表示允许编辑；空函数表示没有有效规则。
	 * @note 调用方负责保证捕获对象在求值时可用；TFunction 不为捕获的 UObject 裸指针保活。
	 */
	void SetEditConditionFunc(TFunction<bool()> InEditConditionFunc)
	{
		EditConditionFunc = InEditConditionFunc;
	}

	/** 判断是否配置了可调用函数，不检查函数捕获对象的有效性。 */
	bool IsValid() const
	{
		return EditConditionFunc != nullptr;
	}

	/**
	 * 执行已保存的条件，使判断使用查询当时的状态。
	 * @return 条件函数的结果；未配置函数时返回 true，默认不限制编辑。
	 */
	bool IsEditConditionMet() const
	{		
		if (IsValid())
		{
			return EditConditionFunc();
		}

		return true;
	}

	/** 返回条件失败时用于详情视图的富文本说明，不会执行条件函数。 */
	FString GetDisabledRichReason() const
	{
		return DisabledRichReason;
	}

	void SetDisabledRichReason(const FString& InRichReason)
	{
		DisabledRichReason = InRichReason;
	}

	bool HasForcedStringValue() const
	{
		return DisabledForcedStringValue.IsSet();
	}

	/**
	 * 取得由数据对象在条件失败时尝试写入的值，不直接修改设置。
	 * @return 已配置的强制字符串；字符串为空也可以是有效配置。
	 * @warning 必须先确认 HasForcedStringValue 为 true，此处直接读取 TOptional::GetValue。
	 */
	FString GetDisabledForcedStringValue() const
	{
		return DisabledForcedStringValue.GetValue();
	}

	void SetDisabledForcedStringValue(const FString& InForcedValue)
	{
		DisabledForcedStringValue = InForcedValue;
	}

private:
	/** Registry 提供的条件函数，保存判断逻辑而非注册当时的 bool 结果。 */
	TFunction<bool()> EditConditionFunc;
	/** 条件失败时显示的富文本，可与同一选项的其他失败原因拼接。 */
	FString DisabledRichReason;
	/**
	 * 条件失败时可选的强制值；未设置时仅限制编辑，设置后由数据对象决定是否写入。
	 * TOptional 区分未配置与配置空字符串，强制值不等同于用户选择的恢复默认值。
	 */
	TOptional<FString> DisabledForcedStringValue;
};