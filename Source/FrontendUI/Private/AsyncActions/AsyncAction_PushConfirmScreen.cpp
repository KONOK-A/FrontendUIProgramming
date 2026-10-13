// Vince Petrelli All Rights Reserved


#include "AsyncActions/AsyncAction_PushConfirmScreen.h"
#include "Subsytems/FrontendUISubsystem.h"

// 与 PushSoftWidget 相同的工厂模式：先返回可绑定输出的节点，实际工作延后到 Activate。
// 世界取自调用上下文；这里 NewObject 创建的是异步任务对象，不是确认框控件。
UAsyncAction_PushConfirmScreen* UAsyncAction_PushConfirmScreen::PushConfirmScreen(const UObject* WorldContextObject, EConfirmScreenType ScreenType, FText InScreenTitle, FText InScreenMessage)
{	
	if (GEngine)
	{
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject,EGetWorldErrorMode::LogAndReturnNull))
		{
			UAsyncAction_PushConfirmScreen* Node = NewObject<UAsyncAction_PushConfirmScreen>();
			Node->CachedOwningWorld = World;
			Node->CachedScreenType = ScreenType;
			Node->CachedScreenTitle = InScreenTitle;
			Node->CachedScreenMessage = InScreenMessage;

			// 等待用户点击期间由 GameInstance 保活；按钮回调中才解除注册。
			Node->RegisterWithGameInstance(World);

			return Node;
		}
	}

	return nullptr;
}

// 请求交给子系统后，会经历资源加载、确认框入栈、等待点击；下面的 lambda 是最后一步的回调。
// 它不在页面刚加载完成时执行，所以能把异步“显示”进一步包装为异步“等待用户选择”。
void UAsyncAction_PushConfirmScreen::Activate()
{
	UFrontendUISubsystem::Get(CachedOwningWorld.Get())->PushConfirmScreenToModalStackAynsc(
		CachedScreenType,
		CachedScreenTitle,
		CachedScreenMessage,
		[this](EConfirmScreenButtonType ClickedButtonType)
		{
			// 先让蓝图按按钮结果继续，再释放请求节点的保活；确认框停用由按钮自己的回调负责。
			OnButtonClicked.Broadcast(ClickedButtonType);

			SetReadyToDestroy();
		}
	);
}
