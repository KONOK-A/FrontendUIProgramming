// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "FrontendLoadingScreenSettings.generated.h"

/**
 * 开发者在项目设置中指定加载画面的外观和等待规则，Config 字段保存到 DefaultGame.ini。
 * FrontendLoadingScreenSubsystem 用 GetDefault 读取这些配置；本类不负责显示或逐帧检查。
 */
UCLASS(Config = Game,DefaultConfig)
class FRONTENDUI_API UFrontendLoadingScreenSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	/**
	 * 同步加载配置的遮罩类，供子系统后续创建 Widget 实例。
	 * @return LoadSynchronous 返回的类引用；非空路径并不保证加载结果有效。
	 * @warning 必须先配置非空软类路径，否则触发 checkf；同步加载可能等待资源读取。
	 */
	TSubclassOf<UUserWidget> GetLoadingScreenWidgetClassChecked() const;

	/** 用于创建视口遮罩的 Widget 蓝图类软引用，首次需要显示时同步加载。 */
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen Settings")
	TSoftClassPtr<UUserWidget> SoftLoadingScreenWidgetClass;

	/**
	 * 基本就绪条件通过后额外保留遮罩的秒数，默认 3 秒；非正值不增加等待。
	 * 该阶段已恢复世界渲染；此值不是最短总显示时间，也不代表纹理流送进度。
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen Settings")
	float HoldLoadingScreenExtraSeconds = 3.f;

	/** 是否在 GIsEditor 环境中执行遮罩显示流程；默认关闭，因此 PIE 中可不显示。 */
	UPROPERTY(Config, EditAnywhere, Category = "Loading Screen Settings")
	bool bShouldShowLoadingScreenInEditor = false;
};
