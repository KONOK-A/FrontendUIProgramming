// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FrontendPlayerController.generated.h"

/**
 * 前端玩家控制器：接管 Pawn 时选择菜单摄像机，并按需初始化硬件画质设置。
 * PlayerController 表示玩家的控制入口，Pawn 是被控制的场景对象；菜单视角可独立切给 CameraActor。
 * 这里的 C++ 不创建主布局，也不打开页面；页面可以通过 ActivatableBase 查询此类型的所属控制器。
 */
UCLASS()
class FRONTENDUI_API AFrontendPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	//~ Begin APlayerController Interface
	/**
	 * 接管 Pawn 后选择菜单摄像机，并在硬件基准结果尚未记录时评测及应用推荐画质。
	 * @param aPawn 本次接管的 Pawn，先交给父类完成接管流程。
	 * @note 使用首个 Actor Tag 为 Default 的 CameraActor；未找到时保留原视角。
	 * @note 任一 CPU/GPU 基准结果为 -1 才运行评测；该初始化依赖实际发生的 Possess 流程。
	 */
	virtual void OnPossess(APawn* aPawn) override;
	//~ End APlayerController Interface
};
