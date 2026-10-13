// Vince Petrelli All Rights Reserved


#include "FontendSettings/FrontendLoadingScreenSettings.h"
#include "Blueprint/UserWidget.h"

TSubclassOf<UUserWidget> UFrontendLoadingScreenSettings::GetLoadingScreenWidgetClassChecked() const
{
    checkf(!SoftLoadingScreenWidgetClass.IsNull(),TEXT("Forgot to assign a valid widget blueprint in the project settings as loading screen"));

    // 软引用此时才同步加载成可用于 CreateWidget 的类；调用会等待资源读取完成再返回。
    // 菜单采用异步打开；加载遮罩在这里需要立即取得类，因此这两条路径的加载方式不同。
    TSubclassOf<UUserWidget> LoadedLoadingScreenWidget = SoftLoadingScreenWidgetClass.LoadSynchronous();

    return LoadedLoadingScreenWidget;
}
