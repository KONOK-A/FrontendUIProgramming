// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Widget_ActivatableBase.generated.h"

class AFrontendPlayerController;
/**
 * 前端页面共用的 CommonUI 可激活控件基类，确认框、设置页、改键页都从这里派生。
 * Activatable 的“激活/停用”是页面参与显示和输入管理的生命周期，不等同于 UObject 创建/销毁。
 * 页面交给 CommonUI 容器管理；派生类可在 NativeOnActivated/NativeOnDeactivated 中处理进出页面。
 * 此基类只补充所属前端控制器入口，具体界面布局和交互仍由派生 C++ 类及控件蓝图承担。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UWidget_ActivatableBase : public UCommonActivatableWidget
{
	GENERATED_BODY()
	
protected:
	/**
	 * 查询并弱缓存页面所属的前端玩家控制器，供派生页面及蓝图使用。
	 * @return 缓存有效时返回缓存对象；否则重新查询，未设置玩家或类型不符时返回 nullptr。
	 * @note 缓存仅在失效时刷新；有效期间更换 OwningPlayer 不会使此函数立即改用新玩家。
	 */
	UFUNCTION(BlueprintPure)
	AFrontendPlayerController* GetOwningFrontendPlayerController();

private:
	/** 所属前端控制器的弱引用缓存，不阻止 GC 回收；失效后由访问函数重新查询。 */
	TWeakObjectPtr<AFrontendPlayerController> CachedOwningFrontendPC;
};
