// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/DataObjects/ListDataObject_Base.h"
#include "ListDataObject_Value.generated.h"

class FOptionsDataInteractionHelper;
/**
 * 普通设置值的基类，组合动态 getter/setter 与可选默认值，不依赖具体行控件。
 * Registry 给它装配两个 Helper；String 用它读写候选值，Scalar 用它读写浮点输出值。
 * Helper 是普通 C++ 对象，因此使用 TSharedPtr；设置数据本身仍由 UE 的 UObject/UPROPERTY 管理。
 */
UCLASS(Abstract)
class FRONTENDUI_API UListDataObject_Value : public UListDataObject_Base
{
	GENERATED_BODY()
	
public:
	/**
	 * 配置读取设置的共享 Helper，不立即读取。
	 * @param InDynamicGetter 使用 getter 反射路径的 Helper；缺省时具体类型采用自身回退行为。
	 */
	void SetDataDynamicGetter(const TSharedPtr<FOptionsDataInteractionHelper>& InDynamicGetter);
	/**
	 * 配置写入设置的共享 Helper，不立即写入。
	 * @param InDynamicSetter 使用 setter 反射路径的 Helper；String 前后切换同时要求已配置 getter。
	 */
	void SetDataDynamicSetter(const TSharedPtr<FOptionsDataInteractionHelper>& InDynamicSetter);

	/**
	 * 记录 Reset 使用的原始默认字符串，不在此写回设置。
	 * String 也将其用作初始化候补；Scalar 默认值使用输出区间的单位，不使用显示区间。
	 * @param InDefaultValue 与目标 setter 格式兼容的字符串；空字符串仍算已配置默认值。
	 */
	void SetDefaultValueFromString(const FString& InDefaultValue) { DefaultStringValue = InDefaultValue;}

	//~ Begin UListDataObject_Base Interface
	/**
	 * 判断是否调用过默认值配置入口，不检查字符串内容或 setter 是否可用。
	 * @return 默认字符串的 TOptional 已设置时为 true。
	 */
	virtual bool HasDefaultValue() const override { return DefaultStringValue.IsSet();}
	//~ End UListDataObject_Base Interface

protected:
	/**
	 * 读取已配置的默认字符串。
	 * 调用前须保证 HasDefaultValue 为 true。
	 * @return 原样返回默认字符串，不做解析、范围校验或映射。
	 */
	FString GetDefaultValueAsString() const { return DefaultStringValue.GetValue();}

	/**
	 * 共享持有普通 C++ 读取桥接对象；具体值类决定读取时机。
	 */
	TSharedPtr<FOptionsDataInteractionHelper> DataDynamicGetter;
	/**
	 * 共享持有普通 C++ 写入桥接对象；通知与 Apply 仍由数据对象控制。
	 */
	TSharedPtr<FOptionsDataInteractionHelper> DataDynamicSetter;

private:
	/**
	 * 未设置与默认空字符串不同；未设置的普通值项不参与恢复默认。
	 */
	TOptional<FString> DefaultStringValue;
};
