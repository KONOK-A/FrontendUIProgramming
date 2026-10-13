// Vince Petrelli All Rights Reserved


#include "Widgets/Widget_ActivatableBase.h"
#include "Controllers/FrontendPlayerController.h"

// 将 UUserWidget 的通用 OwningPlayer 转成项目控制器类型，供派生页面的 C++ 或蓝图使用。
// 首次访问或缓存失效时重新查找；若页面未设置所属玩家、或玩家类型不符，返回 nullptr。
// TWeakObjectPtr::Get 只读取对象，不延长其寿命；有效缓存期间不会重复查询所属玩家。
AFrontendPlayerController* UWidget_ActivatableBase::GetOwningFrontendPlayerController()
{   
	if (!CachedOwningFrontendPC.IsValid())
	{
		CachedOwningFrontendPC = GetOwningPlayer<AFrontendPlayerController>();
	}

    return CachedOwningFrontendPC.IsValid()? CachedOwningFrontendPC.Get() : nullptr;
}
