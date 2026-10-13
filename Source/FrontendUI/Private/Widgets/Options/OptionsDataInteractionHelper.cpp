// Vince Petrelli All Rights Reserved


#include "Widgets/Options/OptionsDataInteractionHelper.h"
#include "FontendSettings/FrontendGameUserSettings.h"

// MAKE_OPTIONS_DATA_CONTROL 传入函数名，例如 GetCurrentGameDifficulty；FCachedPropertyPath 保存该路径。
// PropertyPathHelpers 随后从 UFrontendGameUserSettings 实例按路径读写，所以目标函数须可被 UE 反射找到。
// 这里缓存设置对象的弱引用，不拥有它；同一设置项的 getter 和 setter 使用各自的 Helper。
FOptionsDataInteractionHelper::FOptionsDataInteractionHelper(const FString& InSetterOrGetterFuncPath)
	: CachedDynamicFunctionPath(InSetterOrGetterFuncPath)
{
	CachedWeakGameUserSettings = UFrontendGameUserSettings::Get();
}

// String 初始化和 Scalar 刷新时调用：按 getter 路径读取，将结果交回数据对象。
// 难度直接得到 Normal 之类的字符串；音量等数值也统一变成字符串，数据类再按需转换。
FString FOptionsDataInteractionHelper::GetValueAsString() const
{	
	FString OutStringValue;
	PropertyPathHelpers::GetPropertyValueAsString(
		CachedWeakGameUserSettings.Get(),
		CachedDynamicFunctionPath,
		OutStringValue
	);

	return OutStringValue;
}

// String 切换、Scalar 拖动或 Reset 时调用：将字符串按目标参数类型转换，交给所配置的 setter。
// 本函数只负责写值，不广播、不 Apply；写入后的通知及应用时机由数据对象与 Base 控制。
void FOptionsDataInteractionHelper::SetValueFromString(const FString& InStringValue)
{
	PropertyPathHelpers::SetPropertyValueFromString(
		CachedWeakGameUserSettings.Get(),
		CachedDynamicFunctionPath,
		InStringValue
	);
}
