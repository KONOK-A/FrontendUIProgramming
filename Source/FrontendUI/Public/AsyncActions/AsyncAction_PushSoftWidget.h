// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GameplayTagContainer.h"
#include "AsyncAction_PushSoftWidget.generated.h"

class UWidget_ActivatableBase;

/**
 * 页面入栈阶段事件，共用于页面创建后的初始化和入栈完成通知。
 * @param PushedWidget 本次请求创建的页面实例；两个输出传递同一对象。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPushSoftWidgetDelegate,UWidget_ActivatableBase*,PushedWidget);

/**
 * 将软类页面的异步加载与入栈包装为蓝图节点，实际加载和容器操作由 FrontendUISubsystem 完成。
 * 节点生命周期：工厂缓存输入 -> 蓝图绑定输出 -> Activate 发起请求 -> 两阶段事件 -> 解除保活。
 * “异步”表示结果稍后通过事件送回，不表示在新线程上创建 UWidget。
 * 两个输出均带同一页面实例：先初始化页面，再收到入栈后的通知。
 */
UCLASS()
class FRONTENDUI_API UAsyncAction_PushSoftWidget : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	/**
	 * 创建并向 GameInstance 注册一次入栈请求；此工厂只缓存参数，加载在 Activate 中发起。
	 * WorldContext/HidePin 提供蓝图世界上下文，BlueprintInternalUseOnly 用于异步节点生成流程。
	 * @param WorldContextObject 用于解析所属 World 和 GameInstance 的上下文对象。
	 * @param OwningPlayerController 页面创建后、首个输出广播前设置的所属玩家，以弱引用缓存，可为空。
	 * @param InSoftWidgetClass 要加载的页面软类；空引用会触发 checkf，非空不保证加载成功。
	 * @param InWidgetStackTag 页面要进入的已注册容器标签，区别于页面资源标签。
	 * @param bFocusOnNewlyPushedWidget 是否在 AfterPush 广播后将焦点设到页面提供的非空目标。
	 * @return 已注册的请求节点；GEngine 不存在或无法解析 World 时返回 nullptr。
	 * @note C++ 调用方须自行绑定输出并调用 Activate；工厂返回不表示页面已创建。
	 */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", HidePin = "WorldContextObject", BlueprintInternalUseOnly = "true", DisplayName = "Push Soft Widget To Widget Stack"))
	static UAsyncAction_PushSoftWidget* PushSoftWidget(const UObject* WorldContextObject,
	APlayerController* OwningPlayerController,
	TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass,
	UPARAM(meta = (Categories = "Frontend.WidgetStack")) FGameplayTag InWidgetStackTag,
	bool bFocusOnNewlyPushedWidget = true);

	//~ Begin UBlueprintAsyncActionBase Interface
	/**
	 * 用工厂缓存的参数启动一次异步入栈，并在 AfterPush 的广播和焦点处理后解除节点保活。
	 * @note 所属世界和 UI 子系统须仍有效，布局及目标栈须就绪；没有失败输出或重复激活保护。
	 */
	virtual void Activate() override;
	//~ End UBlueprintAsyncActionBase Interface

	/** 已设置所属玩家、尚未入栈时广播；可在此为页面配置本次请求的数据。 */
	UPROPERTY(BlueprintAssignable)
	FOnPushSoftWidgetDelegate OnWidgetCreatedBeforePush;

	/** AddWidget 返回后广播；早于本节点主动设置焦点，不表示转场动画完成。 */
	UPROPERTY(BlueprintAssignable)
	FOnPushSoftWidgetDelegate AfterPush;

private:
	/** 工厂解析的世界；弱引用不延长世界寿命，激活前仍须保证其有效。 */
	TWeakObjectPtr<UWorld> CachedOwningWorld;
	/** 创建页面时使用的所属玩家；弱引用失效后读取可为空。 */
	TWeakObjectPtr<APlayerController> CachedOwningPC;
	/** 等待 Activate 发起加载的软类路径，不是已创建的页面实例。 */
	TSoftClassPtr<UWidget_ActivatableBase> CachedSoftWidgetClass;
	/** 本次请求的目标容器标签。 */
	FGameplayTag CachedWidgetStackTag;
	/** 控制 AfterPush 广播后的可选焦点转移。 */
	bool bCachedFocusOnNewlyPushedWidget = false;
};
