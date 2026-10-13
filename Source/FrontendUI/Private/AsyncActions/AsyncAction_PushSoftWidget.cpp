// Vince Petrelli All Rights Reserved


#include "AsyncActions/AsyncAction_PushSoftWidget.h"
#include "Subsytems/FrontendUISubsystem.h"
#include "Widgets/Widget_ActivatableBase.h"

// 蓝图调用此工厂获得“一次请求”的 UObject 节点。这里只保存输入，避免输出事件尚未绑定就发出结果。
// 无法取得世界时返回 nullptr；非空软类检查只证明提供了资源路径，并不保证资源一定加载成功。
UAsyncAction_PushSoftWidget* UAsyncAction_PushSoftWidget::PushSoftWidget(const UObject* WorldContextObject, APlayerController* OwningPlayerController, TSoftClassPtr<UWidget_ActivatableBase> InSoftWidgetClass, UPARAM(meta = (Categories = "Frontend.WidgetStack")) FGameplayTag InWidgetStackTag, bool bFocusOnNewlyPushedWidget)
{	
	checkf(!InSoftWidgetClass.IsNull(),TEXT("PushSoftWidgetToStack was passed a null soft widget class "));

	if (GEngine)
	{
		if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject,EGetWorldErrorMode::LogAndReturnNull))
		{
			UAsyncAction_PushSoftWidget* Node = NewObject<UAsyncAction_PushSoftWidget>();
			Node->CachedOwningWorld = World;
			Node->CachedOwningPC = OwningPlayerController;
			Node->CachedSoftWidgetClass = InSoftWidgetClass;
			Node->CachedWidgetStackTag = InWidgetStackTag;
			Node->bCachedFocusOnNewlyPushedWidget = bFocusOnNewlyPushedWidget;

			// 让 GameInstance 持有节点，避免等待异步结果时节点被 GC 回收。
			Node->RegisterWithGameInstance(World);

			return Node;
		}
	}

	return nullptr;
}

// 蓝图异步节点基础设施在创建节点并连接输出后启动 Activate；此处把请求转交给 UI 子系统。
// lambda 的 [this] 让后续阶段能够读取缓存参数并广播成员委托，节点的存活依靠上面的注册。
void UAsyncAction_PushSoftWidget::Activate()
{
	UFrontendUISubsystem* FrontendUISubsystem = UFrontendUISubsystem::Get(CachedOwningWorld.Get());

	FrontendUISubsystem->PushSoftWidgetToStackAsync(CachedWidgetStackTag,CachedSoftWidgetClass,
		[this](EAsyncPushWidgetState InPushState, UWidget_ActivatableBase* PushedWidget)
		{
			switch (InPushState)
			{
			case EAsyncPushWidgetState::OnCreatedBeforePush:
				
				// 第一阶段：此时已有页面对象，但尚未加入容器。
				// 先设置所属玩家，使蓝图初始化时可访问对应的 PlayerController/LocalPlayer。
				// Broadcast 执行订阅此输出的逻辑，执行完后 AddWidget 才继续其入栈流程。
				PushedWidget->SetOwningPlayer(CachedOwningPC.Get());

				OnWidgetCreatedBeforePush.Broadcast(PushedWidget);

				break;

			case EAsyncPushWidgetState::AfterPush:

				// 第二阶段：先通知蓝图；后续焦点设置仍在本节点内执行，因此输出回调可以先配置目标。
				AfterPush.Broadcast(PushedWidget);

				// 激活页面和键盘/手柄焦点是两个概念：页面可见不代表内部按钮已经收到导航输入。
				// GetDesiredFocusTarget 交由页面决定具体目标（例如设置页返回当前选中行）；为空则跳过。
				if (bCachedFocusOnNewlyPushedWidget)
				{
					if (UWidget* WidgetToFocus = PushedWidget->GetDesiredFocusTarget())
					{
						WidgetToFocus->SetFocus();
					}
				}

				// 这次异步任务至此完成，解除节点在 GameInstance 上的保活注册，允许后续 GC 回收。
				// 被释放的是请求节点的保活关系；页面仍由 CommonUI 容器管理，不会因此被关闭。
				SetReadyToDestroy();

				break;
			default:
				break;
			}
		}
	);
}
