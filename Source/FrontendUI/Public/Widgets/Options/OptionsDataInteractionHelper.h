// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "PropertyPathHelpers.h"

class UFrontendGameUserSettings;
/**
 * 通过反射函数路径，把设置的 getter/setter 包装成统一的字符串读写接口。
 * Registry 为每个普通设置项创建 getter Helper 和 setter Helper，交给 Value 的派生类使用。
 * 例如 getter 路径 GetCurrentGameDifficulty 可读出 Normal；setter 路径 SetCurrentGameDifficulty 可写入 Hard。
 * 两个 Helper 各缓存一条路径；GetValueAsString/SetValueFromString 必须与所配置路径的读写用途对应。
 */
class FRONTENDUI_API FOptionsDataInteractionHelper
{
public:
	/**
	 * 缓存一条反射函数路径，并弱引用当前 UFrontendGameUserSettings 实例。
	 * 调用读写入口前须保证设置对象仍有效，路径可反射访问且符合对应 getter/setter 用途。
	 * @param InSetterOrGetterFuncPath Registry 传入的函数名路径，如 GetCurrentGameDifficulty。
	 */
	FOptionsDataInteractionHelper(const FString& InSetterOrGetterFuncPath);

	/**
	 * 通过缓存的 getter 路径读取设置并转为字符串，不刷新设置对象的弱引用。
	 * 底层读取的成功标志未向上传递，空字符串不能区分合法空值与未取得值。
	 * @return PropertyPathHelpers 写入的输出字符串；初始输出为空字符串。
	 */
	FString GetValueAsString() const;
	/**
	 * 按缓存的 setter 路径解释字符串并请求写入；不在此广播修改或调用 ApplySettings。
	 * 本函数未检查或返回底层写入结果，返回不表示写入已经验证成功。
	 * @param InStringValue 与目标 setter 参数类型兼容的原始字符串。
	 */
	void SetValueFromString(const FString& InStringValue);

private:
	/**
	 * 构造时确定的单条读或写路径；一对 getter/setter 分别使用两个 Helper。
	 */
	FCachedPropertyPath CachedDynamicFunctionPath;
	/**
	 * 不拥有设置对象的弱引用；读取、写入入口不会在引用失效后重新获取实例。
	 */
	TWeakObjectPtr<UFrontendGameUserSettings> CachedWeakGameUserSettings;
};
