// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/DataObjects/ListDataObject_String.h"
#include "ListDataObject_StringResolution.generated.h"

/**
 * 从平台枚举分辨率候选，把 FIntPoint 的存储格式转换为易读的尺寸文案。
 * Registry 先用 InitResolutionValues 准备候选和默认值，加入集合时再调用继承来的 String 初始化流程。
 * 它仍复用 String 行控件；特殊之处是候选来源、尺寸格式，以及候选外当前尺寸的显示方式。
 */
UCLASS()
class FRONTENDUI_API UListDataObject_StringResolution : public UListDataObject_String
{
	GENERATED_BODY()
	
public:
	/**
	 * 枚举平台支持的全屏尺寸，按宽高平方和排序后追加候选，并把末项记录为最大候选及默认值。
	 * 须在 String 初始化前调用，且平台必须返回至少一个尺寸；当前实现直接读取 Last()，没有空结果回退。
	 * 函数不清空原候选，重复调用会追加；不写回当前屏幕分辨率。
	 */
	void InitResolutionValues();

protected:
	//~ Begin UListDataObject_String Interface
	/**
	 * 先按 String 规则读取当前缓存；候选不匹配时以 GameUserSettings 的当前屏幕尺寸生成文案。
	 * 候选外尺寸只作为显示回退，不加入候选，也不修改当前原始字符串。
	 */
	virtual void OnDataObjectInitialized() override;
	//~ End UListDataObject_String Interface

private:
	/**
	 * 把尺寸编码为属性路径读写使用的 FIntPoint 字符串。
	 * @param InResolution 宽、高像素数；不在此检查尺寸有效性。
	 * @return 形如 (X=1920,Y=1080) 的原始值。
	 */
	FString ResToValueString(const FIntPoint& InResolution) const;
	/**
	 * 把尺寸转换为行控件展示的宽高文案。
	 * @param InResolution 宽、高像素数。
	 * @return 形如 1920 x 1080 的显示文本。
	 */
	FText ResToDisplayText(const FIntPoint& InResolution) const;

	/**
	 * 平台候选按宽高平方和排序后的末项字符串；用作默认值和无边框条件强制值，不等同于另行查询的桌面尺寸。
	 */
	FString MaximumAllowedResolution;

public:
	FORCEINLINE FString GetMaximumAllowedResolution() const { return MaximumAllowedResolution;}
};
