// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widget_OptionsDetailsView.generated.h"

class UCommonTextBlock;
class UCommonLazyImage;
class UCommonRichTextBlock;
class UListDataObject_Base;
/**
 * 设置页右侧的说明区域，由 OptionsScreen 在列表选中/悬停改变时主动刷新。
 * 从 DataObject 读取标题、描述、图片和编辑限制；它不负责创建列表或绑定设置输入。
 * 数据对象类名与行控件类名也会显示出来，便于对照“设置规则”和“显示方式”的分工。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_OptionsDetailsView : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/**
	 * 展示数据的标题、描述、可选图片、类型信息及禁用原因。
	 * 重新求编辑条件可能应用强制值并同步触发数据通知，因此此更新不保证只读。
	 * @param InDataObject 要展示的设置数据；空值时保留当前详情并返回。
	 * @param InEntryWidgetClassName 对应行的类名，仅作辅助展示；省略时类名位置为空。
	 */
	void UpdateDetailsViewInfo(UListDataObject_Base* InDataObject,const FString& InEntryWidgetClassName = FString());

	/** 清空所有说明文本并折叠图片；不重置图片 Brush，下一次更新再替换或继续隐藏。 */
	void ClearDetailsViewInfo();

protected:
	//~ Begin UUserWidget Interface
	/** UE 实例初始化时清空详情；本控件不参与页面激活栈，后续内容由 OptionsScreen 主动提供。 */
	virtual void NativeOnInitialized() override;
	//~ End UUserWidget Interface

private:
	//***** Bound Widgets *****//
	/** 子蓝图必须提供的同名标题控件；显示数据对象的 DataDisplayName。 */
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonTextBlock_Title;

	/** 按软引用加载说明图片的必需控件；无图片时折叠，有图片时自身不拦截鼠标。 */
	UPROPERTY(meta = (BindWidget))
	UCommonLazyImage* CommonLazyImage_DescriptionImage;

	/** 展示数据静态说明的必需富文本控件；说明中的标签由蓝图样式配置解释。 */
	UPROPERTY(meta = (BindWidget))
	UCommonRichTextBlock* CommonRichText_Description;

	/** 展示数据类和当前行类名称的必需富文本控件，用于对照数据与显示的分工。 */
	UPROPERTY(meta = (BindWidget))
	UCommonRichTextBlock* CommonRichText_DynamicDetails;

	/** 展示编辑限制原因的必需富文本控件；数据可编辑时清空文本。 */
	UPROPERTY(meta = (BindWidget))
	UCommonRichTextBlock* CommonRichText_DisabledReason;
	//***** Bound Widgets *****//
};
