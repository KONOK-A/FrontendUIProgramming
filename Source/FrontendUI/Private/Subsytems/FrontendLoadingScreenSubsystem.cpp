// Vince Petrelli All Rights Reserved


#include "Subsytems/FrontendLoadingScreenSubsystem.h"
#include "PreLoadScreenManager.h"
#include "FontendSettings/FrontendLoadingScreenSettings.h"
#include "Blueprint/UserWidget.h"
#include "Interfaces/FrontendLoadingScreenInterface.h"

#include "FrontendDebugHelper.h"

bool UFrontendLoadingScreenSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// 专用服务器不需要界面；发现派生子系统时，让派生实现代替当前类。
	if (!CastChecked<UGameInstance>(Outer)->IsDedicatedServerInstance())
	{	
		TArray<UClass*> FoundClasses;
		GetDerivedClasses(GetClass(),FoundClasses);

		return FoundClasses.IsEmpty();
	}

	return false;
}

// 子系统由引擎创建后在这里接线：AddUObject 把对象及成员函数绑定到地图事件。
// 此处仅注册回调；真正的显示判断在地图事件触发或 Tick 时发生。
void UFrontendLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	// 监听地图切换的开始和结束；反初始化时成对移除这些绑定。
	FCoreUObjectDelegates::PreLoadMapWithContext.AddUObject(this,&ThisClass::OnMapPreLoaded);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this,&ThisClass::OnMapPostLoaded);
}

// GameInstance 结束时解除本对象的事件绑定；这不是普通切图时的遮罩移除函数。
void UFrontendLoadingScreenSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMapWithContext.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
}

// 从拥有它的 GameInstance 取得当前地图的 World，避免在切图后仍使用旧世界。
UWorld* UFrontendLoadingScreenSubsystem::GetTickableGameObjectWorld() const
{	
	if (UGameInstance* OwningGameInstance = GetGameInstance())
	{
		return OwningGameInstance->GetWorld();
	}

	return nullptr;
}

void UFrontendLoadingScreenSubsystem::Tick(float DeltaTime)
{
	TryUpdateLoadingScreen();
}

ETickableTickType UFrontendLoadingScreenSubsystem::GetTickableTickType() const
{	
	// 类默认对象只是模板，不参加游戏中的逐帧更新。
	if (IsTemplate())
	{
		return ETickableTickType::Never;
	}

	return ETickableTickType::Conditional;
}

// Conditional 模式还要通过这个条件：已有 GameInstance 和游戏视口才适合更新画面。
bool UFrontendLoadingScreenSubsystem::IsTickable() const
{
	return GetGameInstance() && GetGameInstance()->GetGameViewportClient();
}

// 给引擎的性能统计提供名称，与加载状态的判断无关。
TStatId UFrontendLoadingScreenSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFrontendLoadingScreenSubsystem,STATGROUP_Tickables);
}

// 收到“即将加载地图”的通知后，立刻尝试显示遮罩，而不是等下一帧再开始。
void UFrontendLoadingScreenSubsystem::OnMapPreLoaded(const FWorldContext& WorldContext, const FString& MapName)
{
	// 全局地图事件可能来自另一个 PIE 实例，只处理属于自己的 GameInstance。
	if (WorldContext.OwningGameInstance != GetGameInstance())
	{
		return;
	}

	// 上次加载完成后 Tick 已关闭；每次切图都需要重新启用检查。
	SetTickableTickType(ETickableTickType::Conditional);

	bIsCurrentlyLoadingMap = true;

	TryUpdateLoadingScreen();
}

// 地图加载结束只清除标志；World/控制器就绪和额外保持时间仍由后续 Tick 检查。
void UFrontendLoadingScreenSubsystem::OnMapPostLoaded(UWorld* LoadedWorld)
{
	if (LoadedWorld && LoadedWorld->GetGameInstance() == GetGameInstance())
	{
		bIsCurrentlyLoadingMap = false;
	}
}

// 整体流程：先让引擎已有的加载画面完成，再选择“继续显示”或“结束并停止 Tick”。
// OnLoadingReasonUpdated.Broadcast 会调用已绑定的监听者；文本如何画出来由 Widget 实现。
void UFrontendLoadingScreenSubsystem::TryUpdateLoadingScreen()
{
	// 引擎的 PreLoadScreen 仍在显示时暂不接管；这里没有直接查询启动视频的播放状态。
	if (IsPreLoadScreenActive())
	{
		return;
	}

	// 根据地图和世界状态决定显示或结束，不以 Widget 是否存在判断加载是否完成。
	if (ShouldShowLoadingScreen())
	{
		// 仅在没有缓存 Widget 时创建，后续 Tick 只更新原因。
		TryDisplayLoadingScreenIfNone();

		OnLoadingReasonUpdated.Broadcast(CurrentLoadingReason);
	}
	else
	{
		// 正常结束：移除遮罩、重置计时、通知接收者，然后停止无必要的 Tick。
		TryRemoveLoadingScreen();

		HoldLoadingScreenStartUpTime = -1.f;

		//Notify the loading is complete
		NotifyLoadingScreenVisibilityChanged(false);

		//Disable the ticking
		SetTickableTickType(ETickableTickType::Never);
	}
}

// PreLoadScreen 是引擎较早阶段使用的加载界面，与本类后面创建的 UMG 遮罩是两个对象。
bool UFrontendLoadingScreenSubsystem::IsPreLoadScreenActive() const
{	
	if (FPreLoadScreenManager* PreLoadScreenManager = FPreLoadScreenManager::Get())
	{
		return PreLoadScreenManager->HasValidActivePreLoadScreen();
	}

	return false;
}

bool UFrontendLoadingScreenSubsystem::ShouldShowLoadingScreen()
{	
	const UFrontendLoadingScreenSettings* LoadingScreenSettings = GetDefault<UFrontendLoadingScreenSettings>();

	// 默认在编辑器里跳过遮罩，因此 PIE 没看到加载画面不一定是资源缺失。
	if (GIsEditor && !LoadingScreenSettings->bShouldShowLoadingScreenInEditor)
	{
		return false;
	}

	// 仍在加载时暂停世界渲染；这不等于暂停游戏或屏蔽玩家输入。
	if (CheckTheNeedToShowLoadingScreen())
	{
		GetGameInstance()->GetGameViewportClient()->bDisableWorldRendering = true;

		return true;
	}

	// 基本就绪后恢复世界渲染，再额外显示一段时间；这里并未检测纹理是否加载完毕。
	CurrentLoadingReason = TEXT("Waiting for Texture Streaming");

	//There's no need to show the loading screen. Allow the world to be rendered to our viewport here
	GetGameInstance()->GetGameViewportClient()->bDisableWorldRendering = false;

	// 使用实际经过的时间计时；第一次到达这里记下起点，后续 Tick 累计同一段等待。
	const float CurrentTime = FPlatformTime::Seconds();

	if (HoldLoadingScreenStartUpTime < 0.f)
	{
		HoldLoadingScreenStartUpTime = CurrentTime;
	}

	const float ElapsedTime = CurrentTime - HoldLoadingScreenStartUpTime;

	if (ElapsedTime < LoadingScreenSettings->HoldLoadingScreenExtraSeconds)
	{
		return true;
	}

	return false;
}

// 按顺序检查；遇到第一个未满足的条件就返回 true，让遮罩继续显示并说明原因。
// 全部通过只表示这里列出的条件已满足，不代表所有游戏资源都已准备完成。
bool UFrontendLoadingScreenSubsystem::CheckTheNeedToShowLoadingScreen()
{	
	if (bIsCurrentlyLoadingMap)
	{
		CurrentLoadingReason = TEXT("Loading Level");

		return true;
	}

	UWorld* OwningWorld = GetGameInstance()->GetWorld();

	if (!OwningWorld)
	{
		CurrentLoadingReason = TEXT("Initializing World");

		return true;
	}

	if (!OwningWorld->HasBegunPlay())
	{
		CurrentLoadingReason = TEXT("World hasn't begun play yet");

		return true;
	}

	if (!OwningWorld->GetFirstPlayerController())
	{
		CurrentLoadingReason = TEXT("Player Controller is not valid yet");

		return true;
	}

	// 更细的业务就绪条件（GameState、角色、组件等）可在此扩展，目前尚未检查。

	return false;
}

void UFrontendLoadingScreenSubsystem::TryDisplayLoadingScreenIfNone()
{
	// 一次加载期间复用同一个遮罩，避免每帧都创建 Widget。
	if (CachedCreatedLoadingScreenWidget)
	{
		return;
	}

	const UFrontendLoadingScreenSettings* LoadingScreenSettings = GetDefault<UFrontendLoadingScreenSettings>();

	// 配置保存的是蓝图类路径；先取得类，再以 GameInstance 为上下文创建一个 Widget 实例。
	TSubclassOf<UUserWidget> LoadedWidgetClass = LoadingScreenSettings->GetLoadingScreenWidgetClassChecked();

	UUserWidget* CreatedWidget = UUserWidget::CreateWidgetInstance(*GetGameInstance(),LoadedWidgetClass,NAME_None);

	check(CreatedWidget);

	// UMG 转成 Slate 控件后可直接加入视口；ZOrder 1000 使其覆盖较低层内容。
	CachedCreatedLoadingScreenWidget = CreatedWidget->TakeWidget();

	GetGameInstance()->GetGameViewportClient()->AddViewportWidgetContent(
		CachedCreatedLoadingScreenWidget.ToSharedRef(),
		1000
	);

	NotifyLoadingScreenVisibilityChanged(true);
}

// 移除时必须交回当初添加的同一个 Slate 控件；Reset 再释放本类持有的共享引用。
void UFrontendLoadingScreenSubsystem::TryRemoveLoadingScreen()
{
	if (!CachedCreatedLoadingScreenWidget)
	{
		return;
	}

	GetGameInstance()->GetGameViewportClient()->RemoveViewportWidgetContent(CachedCreatedLoadingScreenWidget.ToSharedRef());

	CachedCreatedLoadingScreenWidget.Reset();
}

// 遍历本机 LocalPlayer，通知其控制器和当前 Pawn；不会自动通知地图中的所有 Actor。
// 接收者可用接口处理自己的显示/隐藏响应，本函数本身没有冻结角色或屏蔽输入。
void UFrontendLoadingScreenSubsystem::NotifyLoadingScreenVisibilityChanged(bool bIsVisible)
{
	for (ULocalPlayer* ExistingLocalPlayer : GetGameInstance()->GetLocalPlayers())
	{
		if (!ExistingLocalPlayer)
		{
			continue;
		}

		if (APlayerController* PC = ExistingLocalPlayer->GetPlayerController(GetGameInstance()->GetWorld()))
		{
			// Execute_ 统一调用 C++ 或蓝图接口实现；接口只发通知，不询问对象是否已准备好。
			if (PC->Implements<UFrontendLoadingScreenInterface>())
			{	
				if (bIsVisible)
				{
					IFrontendLoadingScreenInterface::Execute_OnLoadingScreenActivated(PC);
				}
				else
				{
					IFrontendLoadingScreenInterface::Execute_OnLoadingScreenDeactivated(PC);
				}
				
			}

			if(APawn* OwningPawn = PC->GetPawn())
			{
				if (OwningPawn->Implements<UFrontendLoadingScreenInterface>())
				{
					if (bIsVisible)
					{
						IFrontendLoadingScreenInterface::Execute_OnLoadingScreenActivated(OwningPawn);
					}
					else
					{
						IFrontendLoadingScreenInterface::Execute_OnLoadingScreenDeactivated(OwningPawn);
					}
				}
			}
		}

		// 当前只通知已有的控制器和 Pawn；其他对象需要另行接入。
	}
}
