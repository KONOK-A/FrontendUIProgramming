// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonRotator.h"
#include "FrontendCommonRotator.generated.h"

/**
 * 为 CommonUI 选项轮换控件补充按显示文本定位选项的入口，供 String 类型的设置行使用。
 * String 行先填充候选标签，再把数据对象的当前显示文本传进来；数据变化时也走同一刷新入口。
 * 本类只同步选项显示，不直接修改 GameUserSettings；用户轮换后的写回由 String 行与数据对象处理。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UFrontendCommonRotator : public UCommonRotator
{
	GENERATED_BODY()
	
public:
	/**
	 * 按显示文本同步当前选项，使用 FText::EqualTo 查找首个匹配候选。
	 * @param InTextOption 数据对象的当前显示文本，不是保存设置时使用的内部键。
	 * @note 匹配时调用 SetSelectedItem；未匹配时只更新 MyText，保留原选择索引，要求文字控件有效。
	 */
	void SetSelectedOptionByText(const FText& InTextOption);
};
