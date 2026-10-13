// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/DataObjects/ListDataObject_Value.h"
#include "ListDataObject_String.generated.h"

/**
 * 用字符串保存实际选项值，另用 FText 保存显示文案，供按钮和 Rotator 切换。
 * Registry 配置候选，初始化时通过 getter 读取设置；Widget_ListEntry_String 读取候选文案并提交用户选择。
 * 实际值与文案可以不同，例如布尔值 false 对应 Disabled；setter 接收前者，Rotator 显示后者。
 * CurrentStringValue 是数据对象的当前缓存，真正的设置保存在 getter/setter 所访问的对象中。
 */
UCLASS()
class FRONTENDUI_API UListDataObject_String : public UListDataObject_Value
{
	GENERATED_BODY()
	
public:
	/**
	 * 按相同索引追加原始值与显示文案；不去重、不选中，也不写回设置。
	 * @param InStringValue 供 getter/setter 使用的原始值字符串。
	 * @param InDisplayText 对应的用户可见文案；Rotator 按文案匹配首个候选，宜避免歧义。
	 */
	void AddDynamicOption(const FString& InStringValue,const FText& InDisplayText);
	/**
	 * 循环切换到下一个候选并更新缓存；当前值不在候选中时选第一项，空候选时不处理。
	 * 有 setter 时写入并通知，调试回读同时要求 getter 有效；没有 setter 时只改本地缓存。
	 * 本入口不检查编辑条件，调用方通过控件可编辑状态限制用户操作。
	 */
	void AdvanceToNextOption();
	/**
	 * 循环切换到上一个候选并更新缓存；当前值不在候选中时选末项，空候选时不处理。
	 * 有 setter 时写入并通知，调试回读同时要求 getter 有效；没有 setter 时只改本地缓存。
	 */
	void BackToPreviousOption();
	/**
	 * 按 FText::EqualTo 找到首个同文案候选并更新缓存，有 setter 时再写入并通知。
	 * 找不到文案或对应值索引无效时不处理；本入口不检查用户来源或编辑条件。
	 * @param InNewSelectedText 行控件提交的 Rotator 当前标签，不是写入设置的原始字符串。
	 */
	void OnRotatorInitiatedValueChange(const FText& InNewSelectedText);

protected:
	//~ Begin UListDataObject_Base Interface
	/**
	 * 依次用首个候选、已配置默认值、非空 getter 结果建立缓存，后者覆盖前者。
	 * 再匹配显示文案，失败时显示 Invalid Option；不在初始化时调用 setter 或通知。
	 */
	virtual void OnDataObjectInitialized() override;
	/**
	 * 判断缓存是否偏离已配置的默认字符串，不读取 getter，也不要求 setter 有效。
	 * @return 存在默认值且与 CurrentStringValue 不同为 true。
	 */
	virtual bool CanResetBackToDefaultValue() const override;
	/**
	 * 可重置时先更新当前缓存及匹配到的文案，再由 setter 写入并发出 ResetToDefault 通知。
	 * @return 执行了 setter 与通知路径时为 true，不验证底层写入；不可重置或无 setter 时为 false，后者仍可能已改变缓存。
	 */
	virtual bool TryResetBackToDefaultValue() override;
	/**
	 * 判断强制字符串是否与当前缓存不同，不检查它是否属于候选。
	 * @param InForcedValue 失败编辑条件提供的原始值。
	 * @return 与 CurrentStringValue 不同时为 true，防止同值重复强制通知。
	 */
	virtual bool CanSetToForcedStringValue(const FString& InForcedValue) const override;
	/**
	 * 更新强制值缓存并尝试匹配文案；有 setter 时写入并通知 DependencyModified。
	 * 候选不匹配时保留旧文案，条件恢复后不会自动还原原值。
	 * @param InForcedValue 条件要求的原始字符串；本函数不验证候选合法性。
	 */
	virtual void OnSetToForcedStringValue(const FString& InForcedValue) override;
	//~ End UListDataObject_Base Interface
	
	/**
	 * 按原始值查找首个候选，并尝试同步当前显示文案，不改变原始缓存或底层设置。
	 * @param InStringValue 用于匹配候选的原始字符串。
	 * @return 对应文案索引有效并已更新时为 true；失败时为 false 且保留原文案。
	 */
	bool TrySetDisplayTextFromStringValue(const FString& InStringValue);

	/**
	 * 本地原始值缓存；String/Enum 查询不会每次回读设置，外部改值须经相应初始化或依赖刷新路径同步。
	 */
	FString CurrentStringValue;
	/**
	 * 当前标签；来自候选匹配或派生类的候选外值提示，未必与原始值同文。
	 */
	FText CurrentDisplayText;
	/**
	 * 按注册顺序保存实际候选值，与文案数组保持等长且索引一一对应。
	 */
	TArray<FString> AvailableOptionsStringArray;
	/**
	 * 供 Rotator 显示的标签数组；相同索引对应原始值数组中的候选。
	 */
	TArray<FText> AvailableOptionsTextArray;

public:
	FORCEINLINE const TArray<FText>& GetAvailableOptionsTextArray() const { return AvailableOptionsTextArray;}
	FORCEINLINE FText GetCurrentDisplayText() const { return CurrentDisplayText;}
};

/**
 * 布尔选项复用 String 的缓存、切换、写入与重置流程，原始值固定为 true/false。
 * 初始化补齐缺少的 ON/OFF 候选；Registry 可提前指定自定义文案。
 */
UCLASS()
class FRONTENDUI_API UListDataObject_StringBool : public UListDataObject_String
{
	GENERATED_BODY()

public:
	/**
	 * 在 true 候选尚不存在时追加自定义文案，已存在时不替换；应在加入集合前调用。
	 * @param InNewTrueDisplayText true 对应的显示标签，如 Enabled。
	 */
	void OverrideTrueDisplayText(const FText& InNewTrueDisplayText);
	/**
	 * 在 false 候选尚不存在时追加自定义文案，已存在时不替换；应在加入集合前调用。
	 * @param InNewFalseDisplayText false 对应的显示标签，如 Disabled。
	 */
	void OverrideFalseDisplayText(const FText& InNewFalseDisplayText);
	/**
	 * 仅把 true 记录为默认字符串，不立即写回设置。
	 */
	void SetTrueAsDefaultValue();
	/**
	 * 仅把 false 记录为默认字符串，不立即写回设置。
	 */
	void SetFalseAsDefaultValue();

protected:
	//~ Begin UListDataObject_String Interface
	/**
	 * 先补齐缺少的布尔候选，再执行 String 的默认值/getter/显示文案初始化流程。
	 */
	virtual void OnDataObjectInitialized() override;
	//~ End UListDataObject_String Interface

private:
	/**
	 * 只为尚不存在的 true/false 添加 ON/OFF 标签，不覆盖 Registry 已配置的候选文案。
	 */
	void TryInitBoolValues();

	const FString TrueString = TEXT("true");
	const FString FalseString = TEXT("false");
};

/**
 * 用 UE 枚举反射名称作为 String 原始值，继承 String 的候选、缓存与写入流程。
 * StaticEnum 获取枚举描述；添加、读取与默认值转换须使用同一受反射支持的枚举类型。
 * 名称如 WindowedFullscreen 可与用户看到的文案不同；枚举查询只转换本地缓存，不回读 getter。
 */
UCLASS()
class FRONTENDUI_API UListDataObject_StringEnum : public UListDataObject_String
{
	GENERATED_BODY()

public:
	/**
	 * 将枚举值转换为反射名称后追加候选，不验证枚举值或名称是否有效。
	 * @tparam EnumType 须支持 StaticEnum 的枚举类型，并与本数据对象其他枚举调用保持一致。
	 * @param InEnumOption 待注册的枚举值。
	 * @param InDisplayText 对应的用户可见标签。
	 */
	template<typename EnumType>
	void AddEnumOption(EnumType InEnumOption, const FText& InDisplayText)
	{
		const UEnum* StaticEnumOption = StaticEnum<EnumType>();
		const FString ConvertedEnumString = StaticEnumOption->GetNameStringByValue(InEnumOption);

		AddDynamicOption(ConvertedEnumString,InDisplayText);
	}

	/**
	 * 把当前字符串缓存按指定枚举的反射名称反查，不读取底层设置。
	 * 调用方须确保缓存是该类型的有效名称；实现直接转换查找结果，没有失败分支或默认枚举回退。
	 * @tparam EnumType 与注册候选一致且支持 StaticEnum 的枚举类型。
	 * @return 反射名称查找结果转换成的枚举值，名称无效时不保证是合法枚举项。
	 */
	template<typename EnumType>
	EnumType GetCurrentValueAsEnum() const
	{
		const UEnum* StaticEnumOption = StaticEnum<EnumType>();

		return (EnumType)StaticEnumOption->GetValueByNameString(CurrentStringValue);
	}

	/**
	 * 把枚举值的反射名称记录为默认字符串，不添加候选或写入设置。
	 * @tparam EnumType 与注册候选一致且支持 StaticEnum 的枚举类型。
	 * @param InEnumOption 用作默认值的有效枚举项；本函数不校验枚举值有效性。
	 */
	template<typename EnumType>
	void SetDefaultValueFromEnumOption(EnumType InEnumOption)
	{
		const UEnum* StaticEnumOption = StaticEnum<EnumType>();
		const FString ConvertedEnumString = StaticEnumOption->GetNameStringByValue(InEnumOption);

		SetDefaultValueFromString(ConvertedEnumString);
	}
};

/**
 * 以整数字符串复用 String 的候选切换与写入流程；初始化或依赖回读的候选外值显示 Custom。
 * 用于总体画质及各单项画质，依赖值未改变时停止传播以避免双向通知反复递归。
 */
UCLASS()
class FRONTENDUI_API UListDataObject_StringInteger : public UListDataObject_String
{
	GENERATED_BODY()

public:
	/**
	 * 将整数转换成字符串并追加候选，不限制整数范围或去重。
	 * @param InIntegerValue 设置实际使用的整数档位。
	 * @param InDisplayText 该档位的用户可见标签。
	 */
	void AddIntegerOption(int32 InIntegerValue,const FText& InDisplayText);

protected:
	//~ Begin UListDataObject_String Interface
	/**
	 * 先执行 String 初始化，再把无法匹配候选的缓存值显示为 Custom。
	 */
	virtual void OnDataObjectInitialized() override;
	/**
	 * 依赖变化时从 getter 回读；值变化则更新缓存和文案并通知自身变化，不调用本项 setter。
	 * 有 getter 且回读值未改变时直接返回，不再转发依赖事件；其他路径最后调用 Super。
	 * @param ModifiedDependencyData 原始依赖通知中的数据对象，交给基类转发。
	 * @param ModifyReason 原始修改原因；本项刷新通知使用 DependencyModified，基类转发仍保留此原因。
	 */
	virtual void OnEditDependencyDataModified(UListDataObject_Base* ModifiedDependencyData,EOptionsListDataModifyReason ModifyReason) override;
	//~ End UListDataObject_String Interface
};