// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "CommonInputTypeEnum.h"
#include "Widget_KeyRemapScreen.generated.h"

class UCommonRichTextBlock;
class FKeyRemapScreenInputPreprocessor;
/**
 * ListEntry_KeyRemap 请求打开的捕获弹窗，位于 Modal（模态弹窗）层。
 * 激活时注册 Slate 输入预处理器，让按键先到捕获逻辑；停用时注销，结束输入接管。
 * 预处理器筛选设备并报告结果，页面延后一 tick 通知列表行，列表行再让 DataObject 修改映射。
 * 页面只负责提示和捕获生命周期，不保存 Enhanced Input 的键位配置。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_KeyRemapScreen : public UWidget_ActivatableBase
{
	GENERATED_BODY()
	
public:
	/**
	 * 配置下一次激活时使用的捕获设备；调用方应在弹窗入栈前设置，不会更新已注册的预处理器。
	 * @param InDesiredInputType 目标输入类型；键鼠和手柄有专用过滤，其余类型不额外过滤且提示设备名为空。
	 */
	void SetDesiredInputTypeToFilter(ECommonInputType InDesiredInputType);

	/** 捕获结果的单播委托类型；按键由接收方提交到数据对象，不在捕获页面中保存。 */
	DECLARE_DELEGATE_OneParam(FOnKeyRemapScreenKeyPressedDelegate,const FKey& /*PressedKey*/)
	/** 有效键检测后的 CoreTicker 回调中、页面停用前执行；列表行通常在入栈前绑定。 */
	FOnKeyRemapScreenKeyPressedDelegate OnKeyRemapScreenKeyPressed;

	/** 取消结果的单播委托类型；携带 Escape 或设备不匹配导致的原因文本。 */
	DECLARE_DELEGATE_OneParam(FOnKeyRemapScreenKeySelectCanceledDelegate,const FString& /*CanceledReason*/)
	/** 在延迟停用前执行的取消通知；由列表行决定如何向玩家提示。 */
	FOnKeyRemapScreenKeySelectCanceledDelegate OnKeyRemapScreenKeySelectCanceled;

protected:
	//~ Begin UCommonActivatableWidget Interface
	/** CommonUI 激活入口；按已配置设备创建并注册 Slate 预处理器，连接结果委托并更新提示。 */
	virtual void NativeOnActivated() override;

	/** CommonUI 停用入口；向 Slate 注销预处理器后释放共享引用，结束本轮输入接管。 */
	virtual void NativeOnDeactivated() override; 
	//~ End UCommonActivatableWidget Interface

private:
	/**
	 * 接收预处理器的成功通知，复制按键并安排在 CoreTicker 回调中向列表行报告。
	 * @param PressedKey 通过过滤或由手柄模拟点击转换得到的键。
	 */
	void OnValidKeyPressedDetected(const FKey& PressedKey);

	/**
	 * 接收预处理器的取消通知，复制原因并安排延迟报告与停用。
	 * @param CanceledReason Escape 或设备不匹配产生的提示文本。
	 */
	void OnKeySelectCanceled(const FString& CanceledReason);

	/**
	 * 安排一次 CoreTicker 回调，离开当前 Slate 输入派发后再完成捕获收尾。
	 * 顺序为结果回调、DeactivateWidget、NativeOnDeactivated 注销预处理器。
	 * 调用方须保证本页面及回调依赖在执行时仍有效；本函数不合并重复请求，也不取消已排队回调。
	 * @param PreDeactivateCallback 停用前必须可调用的结果处理函数，即使结果委托未绑定也会执行它。
	 */
	void RequestDeactivateWidget(TFunction<void()> PreDeactivateCallback);

	//***** Bound Widgets ***** //
	/** 子蓝图必须提供的同名富文本控件；激活时显示目标设备提示，并使用蓝图配置的标签样式。 */
	UPROPERTY(meta = (BindWidget))
	UCommonRichTextBlock* CommonRichText_RemapMessage;
	//***** Bound Widgets ***** //

	/** 本轮激活注册的普通 C++ 输入预处理器；以共享指针管理，停用时先注销再释放。 */
	TSharedPtr<FKeyRemapScreenInputPreprocessor> CachedInputPreprocessor;

	/** 入栈前由列表行配置；下一次激活时复制给预处理器，并用于生成设备提示。 */
	ECommonInputType CachedDesiredInputType;
};
