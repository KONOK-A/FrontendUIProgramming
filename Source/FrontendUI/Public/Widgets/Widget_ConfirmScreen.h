// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "Widget_ConfirmScreen.generated.h"

class UCommonTextBlock;
class UDynamicEntryBox;

/** 确认按钮的展示文字与语义结果；业务分支使用结果枚举，不依赖可本地化的按钮文字。 */
USTRUCT(BlueprintType)
struct FConfirmScreenButtonInfo
{
	GENERATED_BODY()

	/** 点击按钮时回传的语义结果；Unknown 为未配置初值。 */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	EConfirmScreenButtonType ConfirmScreenButtonType = EConfirmScreenButtonType::Unknown;

	/** 传给动态按钮的显示文字，与结果枚举独立。 */
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FText ButtonTextToDisplay;
};

/**
 * 确认框的一次展示数据，包含标题、正文及按钮列表，本身不创建或显示 UI。
 * 子系统通过工厂生成数据，在页面创建后交给 InitConfirmScreen 生成实际按钮。
 * 工厂返回普通 UObject 指针；调用方须保证消费前对象仍存活，原始指针或 lambda 捕获不构成 GC 保活。
 */
UCLASS()
class FRONTENDUI_API UConfirmScreenInfoObject : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 创建只用于通知的 Ok 按钮配置，按钮结果为 Closed。
	 * @param InScreenTitle 要复制到展示数据的标题。
	 * @param InScreenMsg 要复制到展示数据的正文。
	 * @return 新建的展示数据对象；尚未创建或显示确认页面。
	 */
	static UConfirmScreenInfoObject* CreateOKScreen(const FText& InScreenTitle,const FText& InScreenMsg);
	/**
	 * 创建 Yes/No 配置，按钮结果依次为 Confirmed 和 Cancelled。
	 * @param InScreenTitle 要复制到展示数据的标题。
	 * @param InScreenMsg 要复制到展示数据的正文。
	 * @return 新建的展示数据对象，按钮顺序为 Yes、No。
	 */
	static UConfirmScreenInfoObject* CreateYesNoScreen(const FText& InScreenTitle,const FText& InScreenMsg);
	/**
	 * 创建 Ok/Cancel 决策配置，按钮结果依次为 Confirmed 和 Cancelled。
	 * @param InScreenTitle 要复制到展示数据的标题。
	 * @param InScreenMsg 要复制到展示数据的正文。
	 * @return 新建的展示数据对象，按钮顺序为 Ok、Cancel。
	 */
	static UConfirmScreenInfoObject* CreateOkCancelScreen(const FText& InScreenTitle,const FText& InScreenMsg);

	/** 本次弹窗的标题，在 InitConfirmScreen 中复制到标题控件。 */
	UPROPERTY(Transient)
	FText ScreenTitle;
	
	/** 本次弹窗的正文，在 InitConfirmScreen 中复制到正文控件。 */
	UPROPERTY(Transient)
	FText ScreenMessage;

	/** 按生成顺序排列的按钮配置；初始化确认页时必须至少有一项。 */
	UPROPERTY(Transient)
	TArray<FConfirmScreenButtonInfo> AvailableScreenButtons;
};

/**
 * Modal 层中的确认页面：根据传入数据动态生成按钮，将点击结果交给调用方后停用页面。
 * C++ 决定数据填充、按钮行为与焦点逻辑；子蓝图提供标题/正文控件、按钮容器及按钮条目类。
 * 阅读回调链：子系统 InitConfirmScreen -> 按钮 OnClicked -> 上游结果回调 -> DeactivateWidget。
 * 上游可以是 C++ 设置页，也可以是 AsyncAction_PushConfirmScreen，由后者再广播给蓝图。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_ConfirmScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()
	
public:
	/**
	 * 在入栈前填充展示内容，清空旧按钮的点击委托并重建本次请求的按钮及行为。
	 * @param InScreenInfoObject 非空展示数据，按钮数组须非空；本函数消费内容但不长期保存此对象。
	 * @param ClickedButtonCallback 必须有效的结果回调，按值保存到按钮委托；点击时执行，返回后停用页面。
	 * @note 三个必需子控件须已绑定，按钮容器的条目类须派生自 UFrontendCommonButtonBase。
	 */
	void InitConfirmScreen(UConfirmScreenInfoObject* InScreenInfoObject,TFunction<void(EConfirmScreenButtonType)> ClickedButtonCallback);

protected:
	//~ Begin UCommonActivatableWidget Interface
	/**
	 * 查询焦点目标时，若存在按钮，先直接尝试聚焦末尾条目（决策框通常为 No/Cancel）。
	 * @return Super 的焦点目标，而非直接返回末尾按钮；调用方随后设置焦点可能覆盖此次副作用。
	 * @note 按钮容器须已绑定；这是带 SetFocus 副作用的查询重写。
	 */
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	//~ End UCommonActivatableWidget Interface

private:
	/** 子蓝图中必需的同名标题控件；BindWidget 绑定已有控件，不负责创建。 */
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonTextBlock_Title;

	/** 子蓝图中必需的同名正文控件。 */
	UPROPERTY(meta = (BindWidget))
	UCommonTextBlock* CommonTextBlock_Message;

	/** 必需的按钮容器；条目类须配置为 UFrontendCommonButtonBase 派生类，初始化时会重建其条目。 */
	UPROPERTY(meta = (BindWidget))
	UDynamicEntryBox* DynamicEntryBox_Buttons;
};
