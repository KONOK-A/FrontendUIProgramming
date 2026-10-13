// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FrontendLoadingScreenSubsystem.generated.h"

/**
 * 地图切换期间的加载画面管理器。GameInstance 比单个地图活得更久，适合接收切图前后的事件。
 * 入口顺序：Initialize 订阅地图事件 → OnMapPreLoaded 标记加载 → Tick 持续检查 → 就绪后移除遮罩。
 * UGameInstanceSubsystem 提供自动创建/销毁的生命周期；FTickableGameObject 补上逐帧更新能力。
 * 普通菜单由 FrontendUISubsystem 的界面栈管理；本类将加载 Widget 直接放到游戏视口上。
 */
UCLASS()
class FRONTENDUI_API UFrontendLoadingScreenSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	/** 加载原因通知的签名；CurrentLoadingReason 是描述当前等待条件的文本。 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadingReasonUpdatedDelegate,const FString&,CurrentLoadingReason);

	/**
	 * 在需要显示遮罩的每次更新中同步广播当前原因，文本未变化时也会广播。
	 * 蓝图可据此更新提示；本事件不提供百分比，也不代表某项资源加载完成。
	 */
	UPROPERTY(BlueprintAssignable)
	FOnLoadingReasonUpdatedDelegate OnLoadingReasonUpdated;

	// UE 管理子系统时调用这一组函数：决定是否创建、绑定事件、解除事件。
	//~ Begin USubsytem Interface
	/**
	 * 在非专用服务器且未发现派生实现时创建本子系统。
	 * @param Outer - 引擎提供的所属 GameInstance；类型必须匹配 CastChecked 的要求。
	 * @return 是否由当前类提供加载画面服务；不会为查询派生类而加载蓝图资产。
	 */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const;
	/**
	 * 注册地图加载开始和结束事件，由地图事件或 Tick 再触发显示判断。
	 * @param Collection - 引擎传入的子系统集合；本实现未使用它建立其他子系统依赖。
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection);
	/** 结束服务时移除本对象的地图事件绑定；本函数不移除遮罩或恢复世界渲染。 */
	virtual void Deinitialize();
	//~ End USubsytem Interface

	// Tick 不是 while 循环：引擎每帧调用一次，函数返回后其他游戏逻辑可以继续执行。
	//~ Begin FTickableGameObject Interface
	/** 返回所属 GameInstance 的当前 World；没有 GameInstance 时返回 nullptr。 */
	virtual UWorld* GetTickableGameObjectWorld() const override;
	/**
	 * 在引擎允许 Tick 时更新遮罩状态及提示文本。
	 * @param DeltaTime - 本帧经过的秒数；本实现不使用它，额外等待按 FPlatformTime 计时。
	 */
	virtual void Tick( float DeltaTime ) override;
	/** 为模板对象返回 Never，其余返回 Conditional；运行时还会显式启停 Tick。 */
	virtual ETickableTickType GetTickableTickType() const override;
	/** 仅在 GameInstance 和 GameViewportClient 均存在时满足条件式 Tick 要求。 */
	virtual bool IsTickable() const override;
	/** 提供 Tick 性能统计标识，不参与加载就绪判断。 */
	virtual TStatId GetStatId() const override;
	//~ End FTickableGameObject Interface

private:
	/**
	 * 为本 GameInstance 的新一轮切图启用 Tick、设置加载标志并立即尝试显示遮罩。
	 * @param WorldContext - 本次地图加载上下文；所属 GameInstance 不匹配时忽略事件。
	 * @param MapName - 引擎报告的地图名称；本实现未使用它选择界面或提示。
	 */
	void OnMapPreLoaded(const FWorldContext& WorldContext, const FString& MapName);

	/**
	 * 清除本次地图加载标志，后续就绪检查及额外等待仍由 Tick 完成。
	 * @param LoadedWorld - 已加载世界；为空或属于其他 GameInstance 时不修改状态。
	 */
	void OnMapPostLoaded(UWorld* LoadedWorld);

	/**
	 * 统一更新显示状态：显示期间广播原因，结束时移除遮罩、重置计时、通知并停用 Tick。
	 * 引擎 PreLoadScreen 活跃时暂不处理；结束通知不以此前创建过遮罩为前提。
	 * @warning 调用方需提供有效的 GameInstance 及游戏视口。
	 */
	void TryUpdateLoadingScreen();

	/** 查询引擎 PreLoadScreen；管理器不存在时返回 false，不查询 MoviePlayer 视频状态。 */
	bool IsPreLoadScreenActive() const;

	/**
	 * 结合编辑器开关、基本就绪条件和额外保持时间决定是否继续遮罩。
	 * 检查期间会更新世界渲染开关、原因文本和保持时间起点，不是无副作用的查询。
	 * @return 仍需显示或保持遮罩时为 true；编辑器中禁用显示时直接返回 false。
	 * @warning 需有有效游戏视口；此判断不检测全部纹理流送或业务资源完成情况。
	 */
	bool ShouldShowLoadingScreen();

	/**
	 * 检查地图标志、World、BeginPlay 和首个 PlayerController，记录第一个未满足的原因。
	 * @return 任一基本条件未满足时为 true；全部通过时为 false，不包含额外等待时间。
	 */
	bool CheckTheNeedToShowLoadingScreen();

	/**
	 * 没有缓存遮罩时同步加载配置类、创建 Widget 并以 ZOrder 1000 加入视口，然后发出显示通知。
	 * @warning 需有有效 GameInstance/视口与已配置的 Widget 类；创建失败由 check 暴露。
	 */
	void TryDisplayLoadingScreenIfNone();

	/** 从视口移除缓存的 Slate 控件并释放引用；无缓存时不操作，也不在此发送结束通知。 */
	void TryRemoveLoadingScreen();

	/**
	 * 通知本机 LocalPlayer 的现有控制器及其 Pawn，仅调用实现加载接口的对象。
	 * @param bIsVisible - true 分发 Activated，false 分发 Deactivated；接收者自行处理响应。
	 * @note 通过 Execute_ 调用蓝图或 C++ 实现，不进行网络广播，也不自动阻止输入。
	 */
	void NotifyLoadingScreenVisibilityChanged(bool bIsVisible);

	/** 匹配本 GameInstance 的 PreLoadMap 置 true，PostLoadMap 清 false。 */
	bool bIsCurrentlyLoadingMap = false;

	/** 额外保持阶段的 FPlatformTime 起点，单位为秒；-1 表示尚未开始该阶段。 */
	float HoldLoadingScreenStartUpTime = -1.f;

	/** 当前等待条件的说明文本，供 OnLoadingReasonUpdated 广播；不是加载进度。 */
	FString CurrentLoadingReason;

	/**
	 * 缓存加入视口的 Slate 控件，防止重复创建并保证移除时使用同一实例。
	 * Slate 使用 TSharedPtr 引用计数，不使用 UObject 的 UPROPERTY/GC 引用机制。
	 */
	TSharedPtr<SWidget> CachedCreatedLoadingScreenWidget;
};
