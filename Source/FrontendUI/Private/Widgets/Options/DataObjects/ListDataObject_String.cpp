// Vince Petrelli All Rights Reserved


#include "Widgets/Options/DataObjects/ListDataObject_String.h"
#include "Widgets/Options/OptionsDataInteractionHelper.h"

#include "FrontendDebugHelper.h"

// Registry 最后调用 AddChildListData，经 Base::InitDataObject 进入此重写函数。
// 初始缓存依次取“首个候选 → 已配置默认值 → 非空 getter 结果”，后者覆盖前者。
// 例如 Difficulty 默认 Normal，但 getter 已有 Hard 时，应显示 Hard；getter 为空才保留候补值。
// 初始化只准备显示，不把默认值自动写回设置。
void UListDataObject_String::OnDataObjectInitialized()
{
	if (!AvailableOptionsStringArray.IsEmpty())
	{
		CurrentStringValue = AvailableOptionsStringArray[0];
	}

	if (HasDefaultValue())
	{
		CurrentStringValue = GetDefaultValueAsString();
	}

	if (DataDynamicGetter)
	{
		if (!DataDynamicGetter->GetValueAsString().IsEmpty())
		{
			CurrentStringValue = DataDynamicGetter->GetValueAsString();
		}
	}

	// 值确定后才查对应文案；没有匹配时父类显示 Invalid Option，Integer/Resolution 会再定制显示。
	if (!TrySetDisplayTextFromStringValue(CurrentStringValue))
	{
		CurrentDisplayText = FText::FromString(TEXT("Invalid Option"));
	}
}

void UListDataObject_String::AddDynamicOption(const FString& InStringValue, const FText& InDisplayText)
{
	// 两个数组的相同索引对应一组选项：一个用于 setter，一个用于用户可见文案。
	AvailableOptionsStringArray.Add(InStringValue);
	AvailableOptionsTextArray.Add(InDisplayText);
}

// String 行的 Next 按钮调用这里；Previous 走下方同样的流程，只是方向相反。
// 分成三步：选下一个原始值、同步显示文本、写回设置并通知；到候选末端后循环到首项。
void UListDataObject_String::AdvanceToNextOption()
{
	if (AvailableOptionsStringArray.IsEmpty() || AvailableOptionsTextArray.IsEmpty())
	{
		return;
	}

	const int32 CurrentDisplayIndex = AvailableOptionsStringArray.IndexOfByKey(CurrentStringValue);
	// 用实际值查索引而非显示文本；Difficulty 当前 Normal 时，下一个候选就是 Hard。
	const int32 NextIndexToDisplay = CurrentDisplayIndex + 1;

	const bool bIsNextIndexValid = AvailableOptionsStringArray.IsValidIndex(NextIndexToDisplay);

	if (bIsNextIndexValid)
	{
		CurrentStringValue = AvailableOptionsStringArray[NextIndexToDisplay];
	}
	else
	{
		CurrentStringValue = AvailableOptionsStringArray[0];
	}

	// 文案先在数据层同步；真正更新 Rotator 显示的是后面的修改通知。
	TrySetDisplayTextFromStringValue(CurrentStringValue);

	if (DataDynamicSetter)
	{
		// 以难度为例，这一步经 Helper 调用 SetCurrentGameDifficulty，把 Hard 写入设置对象。
		DataDynamicSetter->SetValueFromString(CurrentStringValue);

		Debug::Print(TEXT("DataDynamicSetter is used. The latest value from Getter: ") + DataDynamicGetter->GetValueAsString());

		// Base 广播后，String 行读取新文案、OptionsScreen 更新 Reset；最后按开关 Apply。
		NotifyListDataModified(this);
	}
}

void UListDataObject_String::BackToPreviousOption()
{
	if (AvailableOptionsStringArray.IsEmpty() || AvailableOptionsTextArray.IsEmpty())
	{
		return;
	}

	const int32 CurrentDisplayIndex = AvailableOptionsStringArray.IndexOfByKey(CurrentStringValue);
	const int32 PreviousIndexToDisplay = CurrentDisplayIndex - 1;

	const bool bIsPreviousIndexValid = AvailableOptionsStringArray.IsValidIndex(PreviousIndexToDisplay);

	if (bIsPreviousIndexValid)
	{
		CurrentStringValue = AvailableOptionsStringArray[PreviousIndexToDisplay];
	}
	else
	{
		CurrentStringValue = AvailableOptionsStringArray.Last();
	}

	TrySetDisplayTextFromStringValue(CurrentStringValue);

	if (DataDynamicSetter)
	{
		DataDynamicSetter->SetValueFromString(CurrentStringValue);

		Debug::Print(TEXT("DataDynamicSetter is used. The latest value from Getter: ") + DataDynamicGetter->GetValueAsString());

		NotifyListDataModified(this);
	}
}

// String 行接到手柄用户旋转后传入所选 FText；程序刷新 Rotator 时不从该行再次提交。
// 通过显示文本找到索引，再取对应实际值，例如选择 Enabled 时写入的是 true。
void UListDataObject_String::OnRotatorInitiatedValueChange(const FText& InNewSelectedText)
{
	const int32 FoundIndex = AvailableOptionsTextArray.IndexOfByPredicate(
		[InNewSelectedText](const FText& AvailableText)->bool
		{
			return AvailableText.EqualTo(InNewSelectedText);
		}
	);

	if (FoundIndex != INDEX_NONE && AvailableOptionsStringArray.IsValidIndex(FoundIndex))
	{
		CurrentDisplayText = InNewSelectedText;
		CurrentStringValue = AvailableOptionsStringArray[FoundIndex];

		if (DataDynamicSetter)
		{
			DataDynamicSetter->SetValueFromString(CurrentStringValue);

			NotifyListDataModified(this);
		}
	}
}

// 页面用它决定 Reset 是否需要包含本项：必须配置默认值，且当前缓存与默认字符串不同。
bool UListDataObject_String::CanResetBackToDefaultValue() const
{
	return HasDefaultValue() && CurrentStringValue != GetDefaultValueAsString();
}

// OptionsScreen 的 Reset 确认后调用；例如 Difficulty 从 Hard 恢复为注册时指定的 Normal。
// 同时更新缓存、文案和设置，再以 ResetToDefault 通知；执行到写入和通知分支后才返回 true。
bool UListDataObject_String::TryResetBackToDefaultValue()
{
	if (CanResetBackToDefaultValue())
	{
		CurrentStringValue = GetDefaultValueAsString();

		TrySetDisplayTextFromStringValue(CurrentStringValue);

		if (DataDynamicSetter)
		{
			DataDynamicSetter->SetValueFromString(CurrentStringValue);

			NotifyListDataModified(this,EOptionsListDataModifyReason::ResetToDefault);

			return true;
		}
	}

	return false;
}

// Base 求编辑条件时调用：强制值用于条件不满足时覆盖当前值，不同于用户主动 Reset。
// 这里只检查当前值是否不同，避免重复通知；并不负责验证强制字符串是否属于候选。
// 条件重新满足只解除禁用，并不会自动恢复覆盖前的旧值。
bool UListDataObject_String::CanSetToForcedStringValue(const FString& InForcedValue) const
{	
	return CurrentStringValue != InForcedValue;
}

// 例如 WindowMode 变成 Borderless 后，分辨率按条件给出的最大候选写回，并报告 DependencyModified。
void UListDataObject_String::OnSetToForcedStringValue(const FString& InForcedValue)
{
	CurrentStringValue = InForcedValue;
	TrySetDisplayTextFromStringValue(CurrentStringValue);

	if (DataDynamicSetter)
	{
		DataDynamicSetter->SetValueFromString(CurrentStringValue);

		NotifyListDataModified(this,EOptionsListDataModifyReason::DependencyModified);
	}
}

// 只做原始值 → 文案查找，不改设置。返回 false 时由调用处决定如何显示候选外的值。
bool UListDataObject_String::TrySetDisplayTextFromStringValue(const FString& InStringValue)
{	
	const int32 CurrentFoundIndex = AvailableOptionsStringArray.IndexOfByKey(InStringValue);

	if (AvailableOptionsTextArray.IsValidIndex(CurrentFoundIndex))
	{
		CurrentDisplayText = AvailableOptionsTextArray[CurrentFoundIndex];

		return true;
	}

	return false;
}

//************ UListDataObject_StringBool ************//

void UListDataObject_StringBool::OverrideTrueDisplayText(const FText& InNewTrueDisplayText)
{
	if (!AvailableOptionsStringArray.Contains(TrueString))
	{
		AddDynamicOption(TrueString,InNewTrueDisplayText);
	}
}

void UListDataObject_StringBool::OverrideFalseDisplayText(const FText& InNewFalseDisplayText)
{
	if (!AvailableOptionsStringArray.Contains(FalseString))
	{
		AddDynamicOption(FalseString,InNewFalseDisplayText);
	}
}

void UListDataObject_StringBool::SetTrueAsDefaultValue()
{
	SetDefaultValueFromString(TrueString);
}

void UListDataObject_StringBool::SetFalseAsDefaultValue()
{
	SetDefaultValueFromString(FalseString);
}

void UListDataObject_StringBool::OnDataObjectInitialized()
{
	// 先补齐未提供的 ON/OFF 候选；Registry 已提前添加 Enabled/Disabled 时，不会被默认文案覆盖。
	TryInitBoolValues();

	// Super 在这里指 String 父类：复用其默认值/getter/文案流程，而不是从头重写初始化。
	Super::OnDataObjectInitialized();
}

void UListDataObject_StringBool::TryInitBoolValues()
{
	if (!AvailableOptionsStringArray.Contains(TrueString))
	{
		AddDynamicOption(TrueString,FText::FromString(TEXT("ON")));
	}

	if (!AvailableOptionsStringArray.Contains(FalseString))
	{
		AddDynamicOption(FalseString,FText::FromString(TEXT("OFF")));
	}
}

//************ UListDataObject_StringBool ************//

//************ UListDataObject_StringInteger ************//

void UListDataObject_StringInteger::AddIntegerOption(int32 InIntegerValue, const FText& InDisplayText)
{
	AddDynamicOption(LexToString(InIntegerValue),InDisplayText);
}

// 与 Bool 先准备候选不同，这里先让 String 读出值，再把候选外值的提示改成 Custom。
void UListDataObject_StringInteger::OnDataObjectInitialized()
{
	Super::OnDataObjectInitialized();

	if (!TrySetDisplayTextFromStringValue(CurrentStringValue))
	{
		CurrentDisplayText = FText::FromString(TEXT("Custom"));
	}
}

// 被订阅项广播后进入，例如总体画质改变后各单项需要回读；这里不再调用本项 setter。
// 反向也相同：单项变动后总体画质重新读取自己的 getter，不能匹配预设档位时显示 Custom。
void UListDataObject_StringInteger::OnEditDependencyDataModified(UListDataObject_Base* ModifiedDependencyData, EOptionsListDataModifyReason ModifyReason)
{
	if (DataDynamicGetter)
	{	
		// 总体画质与各子项互相订阅；值未变化就停止传播，避免双向通知反复递归。
		if (CurrentStringValue == DataDynamicGetter->GetValueAsString())
		{
			return;
		}

		CurrentStringValue = DataDynamicGetter->GetValueAsString();

		if (!TrySetDisplayTextFromStringValue(CurrentStringValue))
		{
			CurrentDisplayText = FText::FromString(TEXT("Custom"));
		}

		NotifyListDataModified(this,EOptionsListDataModifyReason::DependencyModified);
	}

	// 保留 Base 向行控件转发“依赖变化”的行为；上方值不变的提前 return 会终止本次传播。
	Super::OnEditDependencyDataModified(ModifiedDependencyData, ModifyReason);
}

//************ UListDataObject_StringInteger ************//