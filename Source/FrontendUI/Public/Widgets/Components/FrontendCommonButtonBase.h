// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "FrontendCommonButtonBase.generated.h"

class UCommonTextBlock;
class UCommonLazyImage;
/**
 * 通用前端按钮，统一文字、图片及文本样式，并通过子系统广播悬停描述。
 * CommonButtonBase 已提供点击、选中、悬停等按钮机制；本类把它们接到项目使用的文字/图片子控件。
 * 子蓝图负责按钮布局和样式，C++ 设置内容；确认框、设置页签和改键行均可复用这个基类。
 * 具体点击做什么由使用者通过 OnClicked 绑定，本按钮不会直接决定打开哪个页面或修改哪项设置。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UFrontendCommonButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()
	
public:
	/**
	 * 更新实际文字子控件，按配置决定是否转换为大写；不回写 ButtonDisplayText。
	 * @param InText 要显示的文字；为空时忽略，不能用空文本清除现有内容。
	 * @note 未绑定可选文字控件时不执行更新。
	 */
	UFUNCTION(BlueprintCallable)
	void SetButtonText(FText InText);

	/**
	 * 读取按钮当前显示的文字，供设置页构造重置确认提示等使用。
	 * @return 文字子控件的当前内容；未绑定文字控件时返回空 FText，不返回设计器默认值。
	 */
	UFUNCTION(BlueprintCallable)
	FText GetButtonDisplayText() const;

	/**
	 * 更新可选图片控件的画刷，供改键行显示当前按键图标；未绑定图片控件时忽略。
	 * @param InBrush 图片资源及其绘制参数，由 CommonLazyImage::SetBrush 应用。
	 */
	UFUNCTION(BlueprintCallable)
	void SetButtionDisplayImage(const FSlateBrush& InBrush);

private:
	//~ Begin UUserWidget Interface
	/** 在设计器预览及运行时构建阶段应用默认按钮文字，不作为一次性初始化入口。 */
	virtual void NativePreConstruct() override;
	//~ End UUserWidget Interface
	
	//~ Begin UCommonButtonBase Interface
	/** 将 CommonUI 当前文字样式应用到可选文字控件；控件或样式为空时跳过。 */
	virtual void NativeOnCurrentTextStyleChanged() override;
	/** 描述非空时经 UI 子系统广播来源按钮及描述；广播要求有效世界与 UI 子系统。 */
	virtual void NativeOnHovered() override;
	/** 经 UI 子系统广播空描述以清除说明，即使按钮原描述为空也会广播。 */
	virtual void NativeOnUnhovered() override;
	//~ End UCommonButtonBase Interface
	
	//**** Bound Widgets ****//
	/** 可选的同名文字子控件；BindWidgetOptional 只绑定蓝图中已有控件，允许纯图标按钮省略。 */
	UPROPERTY(meta = (BindWidgetOptional))
	UCommonTextBlock* CommonTextBlock_ButtonText;

	/** 可选的同名图片子控件，允许纯文字按钮省略；访问时须检查是否已绑定。 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional,AllowPrivateAccess = "true"))
	UCommonLazyImage* CommonLazyImage_ButtonImage;
	//**** Bound Widgets ****//

	/** PreConstruct 中应用的默认文字；运行时 SetButtonText 只更新子控件，不回写此属性。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Button", meta = (AllowPrivateAccess = "true"))
	FText ButtonDisplayText;

	/** SetButtonText 是否先用 FText::ToUpper 转换文字；改变此值本身不会立即刷新子控件。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Button", meta = (AllowPrivateAccess = "true"))
	bool bUserUpperCaseForButtonText = false;

	/** 悬停时经子系统广播的说明文字；订阅方负责布局与显示，结束悬停时广播空文本。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Button", meta = (AllowPrivateAccess = "true"))
	FText ButtonDescriptionText;
};
