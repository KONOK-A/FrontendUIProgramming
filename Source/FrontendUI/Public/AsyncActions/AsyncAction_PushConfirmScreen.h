// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "AsyncAction_PushConfirmScreen.generated.h"

/**
 * 确认页面按钮的语义结果事件。
 * @param ClickedButtonType 被点击按钮的结果；Ok 页面为 Closed，决策页面为 Confirmed 或 Cancelled。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConfirmScreenButtonClickedDelegate,EConfirmScreenButtonType,ClickedButtonType);

/**
 * 蓝图确认弹窗节点，将按钮点击结果通过输出事件返回。
 * 本节点只保存请求及转发结果；子系统选择 Modal/页面类，ConfirmScreen 动态生成按钮。
 * 节点在收到按钮回调后完成，弹窗入栈本身不代表操作完成，因此保活时间包含等待用户选择。
 */
UCLASS()
class FRONTENDUI_API UAsyncAction_PushConfirmScreen : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	/**
	 * 创建确认请求并注册到 GameInstance 保活；只缓存输入，由 Activate 请求弹窗。
	 * @param WorldContextObject 用于解析所属 World 和 GameInstance 的上下文对象。
	 * @param ScreenType 按钮组合；后续激活只支持 Ok、YesNo、OKCancel。
	 * @param InScreenTitle 本次请求的标题，按值缓存。
	 * @param InScreenMessage 本次请求的正文，按值缓存。
	 * @return 已注册的请求节点；GEngine 不存在或世界解析失败时返回 nullptr。
	 * @note C++ 调用方须绑定输出后调用 Activate；工厂本身不显示确认页面。
	 */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", HidePin = "WorldContextObject", BlueprintInternalUseOnly = "true", DisplayName = "Show Confirmation Screen"))
	static UAsyncAction_PushConfirmScreen* PushConfirmScreen(
		const UObject* WorldContextObject,
		EConfirmScreenType ScreenType,
		FText InScreenTitle,
		FText InScreenMessage
	);

	//~ Begin UBlueprintAsyncActionBase Interface
	/**
	 * 通过 UI 子系统显示确认页，等待按钮结果后广播 OnButtonClicked 并解除节点保活。
	 * @note 世界、子系统及 Modal 配置须有效；Unkown 类型会在子系统中触发 check。
	 * @note 入栈不代表请求完成；未收到按钮回调的退出路径不会在此主动解除保活。
	 */
	virtual void Activate() override;
	//~ End UBlueprintAsyncActionBase Interface

	/** 按钮点击后广播语义结果；广播早于请求节点解除保活及确认页面停用。 */
	UPROPERTY(BlueprintAssignable)
	FOnConfirmScreenButtonClickedDelegate OnButtonClicked;

private:
	/** 工厂解析的世界；弱引用不会因等待用户选择而延长世界寿命。 */
	TWeakObjectPtr<UWorld> CachedOwningWorld;
	/** 工厂按值保存的按钮组合，在 Activate 中转交给子系统。 */
	EConfirmScreenType CachedScreenType;
	/** 本次请求的标题副本。 */
	FText CachedScreenTitle;
	/** 本次请求的正文副本。 */
	FText CachedScreenMessage;
};
