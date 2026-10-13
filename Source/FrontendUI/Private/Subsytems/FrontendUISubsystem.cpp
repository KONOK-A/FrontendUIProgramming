// Vince Petrelli All Rights Reserved


#include "Subsytems/FrontendUISubsystem.h"
#include "Engine/AssetManager.h"
#include "Widgets/Widget_PrimaryLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "Widgets/Widget_ActivatableBase.h"
#include "Widgets/Widget_ConfirmScreen.h"
#include "FrontendGameplayTags.h"
#include "FrontendFunctionLibrary.h"

#include "FrontendDebugHelper.h"

// 静态便捷入口。Widget 等 UObject 可作为 WorldContextObject，使调用方不必逐层寻找 GameInstance。
// Assert 模式要求传入对象能关联到有效世界；这是调用前提，并非能接收任意 UObject 的全局单例。
UFrontendUISubsystem* UFrontendUISubsystem::Get(const UObject* WorldContextObject)
{
	if (GEngine)
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject,EGetWorldErrorMode::Assert);

		return UGameInstance::GetSubsystem<UFrontendUISubsystem>(World->GetGameInstance());
	}

	return nullptr;
}

// UE 初始化 GameInstance 子系统时查询是否需要此实现；普通打开页面的操作不会调用它。
// Outer 在此生命周期入口中是所属 GameInstance，CastChecked 表达对类型的强约束。
bool UFrontendUISubsystem::ShouldCreateSubsystem(UObject* Outer) const
{	
	// 专用服务器不需要前端 UI。
	if (!CastChecked<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{	
		TArray<UClass*> FoundClasses;
		GetDerivedClasses(GetClass(),FoundClasses);

		// 当前已发现派生类时跳过当前类，让派生实现承担子系统职责，避免基类和子类重复服务。
		// 这里只检查 GetDerivedClasses 返回的类；不会主动扫描或加载所有未加载的蓝图资源。
		return FoundClasses.IsEmpty();
	}

	return false;
}

void UFrontendUISubsystem::RegisterCreatedPrimaryLayoutWidget(UWidget_PrimaryLayout* InCreatedWidget)
{	
	check(InCreatedWidget);

	// 布局负责保存“栈标签 -> 容器”，子系统只保存布局入口；这两级注册都要在 Push 前完成。
	// 当前 C++ 未调用此注册函数；蓝图可调用的接口不代表其实际执行顺序已由这里保证。
	CreatedPrimaryLayout = InCreatedWidget;
}

void UFrontendUISubsystem::PushSoftWidgetToStackAsync(const FGameplayTag& InWidgetStackTag, TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass,TFunction<void(EAsyncPushWidgetState,UWidget_ActivatableBase*)> AysncPushStateCallback)
{
	check(!InSoftWidgetClass.IsNull());

	// 阶段 1：软类引用保存资源路径，不等于页面实例。把路径交给 StreamableManager 请求加载，
	// 完成后才运行下面的委托；本函数没有直接返回新页面，结果只能在回调中取得。
	// 例如 KeyRemap 行先经 FunctionLibrary 用页面标签查出软类，再将其和 Modal 栈标签传入。
	UAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
		InSoftWidgetClass.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda(
			// 按值捕获软类、标签和回调，保证此函数返回后这些参数副本仍可使用。
			// this 捕获的是子系统指针；普通 C++ lambda 捕获本身不会替 UObject 建立 GC 强引用。
			[InSoftWidgetClass,this,InWidgetStackTag,AysncPushStateCallback]()
			{
				UClass* LoadedWidgetClass = InSoftWidgetClass.Get();
				
				check(LoadedWidgetClass && CreatedPrimaryLayout);

				// 阶段 2：类加载成功后，沿“子系统 -> 主布局 -> 目标容器”找到页面的归属层。
				// check 和 FindWidgetStackByTag 的检查表达配置前提：布局及标签对应的栈必须已注册。
				UCommonActivatableWidgetContainerBase* FoundWidgetStack = CreatedPrimaryLayout->FindWidgetStackByTag(InWidgetStackTag);

				// 阶段 3：交给 CommonUI 容器创建/管理可激活页面，不在这里单独 CreateWidget/AddToViewport。
				// AddWidget 的初始化回调在页面加入容器前提供实例，让页面激活前就具备必需的数据。
				// 此时通知的是同一个页面实例，后面的 AfterPush 不会再创建第二份。
				UWidget_ActivatableBase* CreatedWidget = FoundWidgetStack->AddWidget<UWidget_ActivatableBase>(
					LoadedWidgetClass,
					[AysncPushStateCallback](UWidget_ActivatableBase& CreatedWidgetInstance)
					{
						AysncPushStateCallback(EAsyncPushWidgetState::OnCreatedBeforePush,&CreatedWidgetInstance);
					}
				);

				// 阶段 4：通知调用方入栈流程已经返回，AsyncAction 在此阶段广播输出、按需设置焦点。
				// AfterPush 不表示转场动画或首帧显示已完成，不能把它当成渲染完成事件。
				AysncPushStateCallback(EAsyncPushWidgetState::AfterPush,CreatedWidget);
			}
		)
	);
}

void UFrontendUISubsystem::PushConfirmScreenToModalStackAynsc(EConfirmScreenType InScreenType, const FText& InScreenTitle, const FText& InScreenMsg, TFunction<void(EConfirmScreenButtonType)> ButtonClickedCallback)
{	
	// 先把弹窗类型转换为纯展示数据：标题、正文、按钮文字以及每个按钮对应的结果枚举。
	// 调用方（设置页、改键行或蓝图异步节点）只关心最后是 Confirmed、Cancelled 还是 Closed。
	UConfirmScreenInfoObject* CreatedInfoObject = nullptr;

	switch (InScreenType)
	{
	case EConfirmScreenType::Ok:
		CreatedInfoObject = UConfirmScreenInfoObject::CreateOKScreen(InScreenTitle,InScreenMsg);
		break;

	case EConfirmScreenType::YesNo:
		CreatedInfoObject = UConfirmScreenInfoObject::CreateYesNoScreen(InScreenTitle,InScreenMsg);
		break;

	case EConfirmScreenType::OKCancel:
		CreatedInfoObject = UConfirmScreenInfoObject::CreateOkCancelScreen(InScreenTitle,InScreenMsg);
		break;

	case EConfirmScreenType::Unkown:
		break;
	default:
		break;
	}

	check(CreatedInfoObject);

	// 然后复用通用入栈流程：页面标签经项目设置换成软类，目标容器固定选择 Modal。
	// 页面标签对应的类必须继承 UWidget_ConfirmScreen；下方 CastChecked 会检查这一约定。
	// 此处 CreatedInfoObject 被 lambda 按原始指针捕获，这不等于 GC 保活；勿把捕获当成 UPROPERTY。
	PushSoftWidgetToStackAsync(
		FrontendGameplayTags::Frontend_WidgetStack_Modal,
		UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(FrontendGameplayTags::Frontend_Widget_ConfirmScreen),
		[CreatedInfoObject,ButtonClickedCallback](EAsyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			// 只在入栈前配置一次；AfterPush 不需要再生成按钮，也不会在此报告用户已经点击。
			// InitConfirmScreen 会把 ButtonClickedCallback 继续交给各个按钮的点击委托。
			if (InPushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				UWidget_ConfirmScreen* CreatedConfirmScreen = CastChecked<UWidget_ConfirmScreen>(PushedWidget);
				CreatedConfirmScreen->InitConfirmScreen(CreatedInfoObject,ButtonClickedCallback);
			}
		}
	);
}
