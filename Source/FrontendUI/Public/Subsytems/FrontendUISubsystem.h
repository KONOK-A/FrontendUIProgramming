// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "FrontendUISubsystem.generated.h"

class UWidget_PrimaryLayout;
struct FGameplayTag;
class UWidget_ActivatableBase;
class UFrontendCommonButtonBase;

/** 一次成功入栈请求的两个回调阶段；两个阶段传递同一个页面实例。 */
enum class EAsyncPushWidgetState : uint8
{
	/** 页面已创建但尚未加入容器，可在此设置页面数据及所属玩家。 */
	OnCreatedBeforePush,
	/** AddWidget 已返回；不表示转场动画或首帧渲染完成。 */
	AfterPush
};

/**
 * 按钮描述变化的动态多播事件；订阅方自行决定描述区域的显示方式。
 * @param BroadcastingButton 发出描述更新的按钮，供订阅方过滤来源。
 * @param DescriptionText 悬停描述；结束悬停时为空文本，用于清除对应说明。
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnButtonDescriptionTextUpdatedDelegate,UFrontendCommonButtonBase*,BroadcastingButton,FText,DescriptionText);


/**
 * 随 GameInstance 存在的前端 UI 服务，统一管理主布局、页面入栈和确认弹窗。
 * GameInstance 的子系统由 UE 管理，无需每个页面自己 NewObject；通过 Get 取得同一实例。
 * 阅读主链：调用方取得页面软类 -> 本类异步加载 -> PrimaryLayout 按栈标签找容器
 * -> AddWidget 创建并入栈 -> 回调通知调用方。页面类标签与目标栈标签是两套不同的键。
 * 这里只保存已创建的布局；外部须先创建/显示布局、注册布局及其中的栈，才能推入页面。
 */
UCLASS()
class FRONTENDUI_API UFrontendUISubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/**
	 * 从上下文所属的 GameInstance 取得前端 UI 子系统，不创建或注册布局。
	 * @param WorldContextObject 能解析到有效 World 及其 GameInstance 的上下文对象。
	 * @return 已存在的子系统；GEngine 不存在或该 GameInstance 未提供此子系统时可为空。
	 * @note 世界解析使用 Assert 模式；无效上下文并不是受支持的空值查询方式。
	 */
	static UFrontendUISubsystem* Get(const UObject* WorldContextObject);

	//~ Begin USubsyem Interface
	/**
	 * 决定是否创建当前子系统类：专用服务器跳过，已发现派生类时也跳过当前类。
	 * @param Outer 所属 GameInstance；实现使用 CastChecked 校验其类型。
	 * @return 非专用服务器且 GetDerivedClasses 未找到派生类时为 true。
	 * @note 只检查当前可发现的派生类，不主动加载尚未加载的蓝图资源。
	 */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End USubsyem Interface

	/**
	 * 保存已创建的主布局引用，供后续入栈请求定位容器；再次注册会替换原引用。
	 * @param InCreatedWidget 非空的主布局实例；为空会触发 check。
	 * @note 只登记引用，不负责创建控件、加入视口或注册布局中的页面栈。
	 */
	UFUNCTION(BlueprintCallable)
	void RegisterCreatedPrimaryLayoutWidget(UWidget_PrimaryLayout* InCreatedWidget);

	/**
	 * 异步加载页面类，再交给主布局中的指定容器创建并入栈。
	 * 成功路径依次通知 OnCreatedBeforePush 和 AfterPush；蓝图入口由 AsyncAction_PushSoftWidget 提供。
	 * @param InWidgetStackTag 已在主布局中注册、且对应有效容器的栈标签。
	 * @param InSoftWidgetClass 非空且可加载为 UWidget_ActivatableBase 派生类的软引用。
	 * @param AysncPushStateCallback 必须有效的阶段回调，接收阶段及同一页面实例；会被按值保存并直接调用。
	 * @note 加载完成前须注册有效布局及目标栈，并保证回调使用的对象仍有效；lambda 捕获不提供 UObject 的 GC 保活。
	 * @note 不提供失败或取消事件；软类、加载结果、布局和栈的配置前提由 check/checkf 校验。
	 */
	void PushSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag,TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass,TFunction<void(EAsyncPushWidgetState,UWidget_ActivatableBase*)> AysncPushStateCallback);
	/**
	 * 使用已配置的确认框软类请求 Modal 页面，并在入栈前填充标题、正文及按钮。
	 * @param InScreenType 按钮组合，仅支持 Ok、YesNo、OKCancel；Unkown 或其他值会触发 check。
	 * @param InScreenTitle 本次弹窗标题，复制到展示数据中。
	 * @param InScreenMsg 本次弹窗正文，复制到展示数据中。
	 * @param ButtonClickedCallback 必须有效的选择回调；按钮点击时直接调用，执行结束后才停用页面。
	 * @note 须满足通用入栈前提，且页面标签配置的类必须派生自 UWidget_ConfirmScreen。
	 * @note 页面入栈不会触发选择回调；此接口也不为未点击按钮的退出路径合成结果。
	 */
	void PushConfirmScreenToModalStackAynsc(EConfirmScreenType InScreenType,const FText& InScreenTitle,const FText& InScreenMsg,TFunction<void(EConfirmScreenButtonType)> ButtonClickedCallback);

	/** 按钮悬停描述的广播入口；订阅方按来源更新或清除说明，本子系统不直接绘制描述。 */
	UPROPERTY(BlueprintAssignable)
	FOnButtonDescriptionTextUpdatedDelegate OnButtonDescriptionTextUpdated;

private:
	/** 当前注册的主布局；UPROPERTY 让 GC 追踪引用，Transient 避免将运行时实例序列化。 */
	UPROPERTY(Transient)
	UWidget_PrimaryLayout* CreatedPrimaryLayout;
};
