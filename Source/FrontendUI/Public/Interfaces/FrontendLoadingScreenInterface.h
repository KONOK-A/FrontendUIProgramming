// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FrontendLoadingScreenInterface.generated.h"

/** 加载画面通知接口的反射外壳；具体事件契约声明于 IFrontendLoadingScreenInterface。 */
UINTERFACE(BlueprintType,MinimalAPI)
class UFrontendLoadingScreenInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 接收加载画面显示/隐藏通知。接口只约定“有哪些消息”，不会替接收者实现具体行为。
 * FrontendLoadingScreenSubsystem 先检查控制器/Pawn 是否实现本接口，再用 Execute_ 分发消息。
 * 若要理解通知结果，继续查看具体接收类的蓝图事件或 C++ _Implementation。
 */
class FRONTENDUI_API IFrontendLoadingScreenInterface
{
	GENERATED_BODY()

	// BlueprintNativeEvent 可在蓝图中实现，也可在 C++ 的同名 _Implementation 函数中实现。
public:
	/**
	 * 加载子系统将新遮罩加入视口后，通知实现此接口的本地控制器或 Pawn。
	 * 接收者可在蓝图或 C++ _Implementation 中响应；本事件不执行联网同步或默认输入屏蔽。
	 */
	UFUNCTION(BlueprintNativeEvent)
	void OnLoadingScreenActivated();
	/*virtual void OnLoadingScreenActivated_Implementation();*/

	/**
	 * 加载子系统判定本轮无需继续显示遮罩后发出的结束通知。
	 * 即使本轮未实际创建遮罩也可能收到此事件，接收者不应假定此前一定收到 Activated。
	 */
	UFUNCTION(BlueprintNativeEvent)
	void OnLoadingScreenDeactivated();
	/*virtual void OnLoadingScreenDeactivated_Implementation();*/
};
