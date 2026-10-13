// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "FrontendTabListWidgetBase.generated.h"

class UFrontendCommonButtonBase;

/**
 * 设置页的页签容器：使用统一按钮类注册页签，并为生成的按钮设置显示名称。
 * OptionsScreen 激活时按 Registry 的集合 ID 创建页签，监听 OnTabSelected 后替换列表的数据源。
 * 本类负责“有哪些页签和按钮”，不负责设置页内容；TabID 用于查找数据，显示名称用于玩家阅读。
 */
UCLASS(Abstract, BlueprintType, meta = (DisableNaiveTick))
class FRONTENDUI_API UFrontendTabListWidgetBase : public UCommonTabListWidgetBase
{
	GENERATED_BODY()
	
public:
	/**
	 * 用配置的按钮类请求注册页签，再按 ID 找回按钮并设置显示名称。
	 * @param InTabID 集合的稳定 ID；OptionsScreen 调用前会排除已经注册的 ID。
	 * @param InTabDisplayName 玩家可见的页签名称，交由按钮应用文字及大小写规则。
	 * @note 要求有效的 TabButtonEntryWidgetClass；不绑定 ContentWidget，内容切换由上游处理。
	 * @note 不向调用方返回 RegisterTab 的成功状态；找不到兼容按钮时跳过文字更新。
	 */
	void RequestRegisterTab(const FName& InTabID,const FText& InTabDisplayName);

private:
	//~ Begin UWidget Interface
#if WITH_EDITOR	
	/**
	 * 编译控件蓝图时检查页签按钮类是否配置。
	 * @param CompileLog 接收父类诊断及缺失 TabButtonEntryWidgetClass 的编译错误。
	 */
	virtual void ValidateCompiledDefaults(class IWidgetCompilerLog& CompileLog) const override;
#endif
	//~ End UWidget Interface

	/** 编辑器/蓝图可用的预览数量配置；本类 C++ 未读取它，不能据此推断实际预览数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Tab List Settings", meta = (AllowPrivateAccess = "true", ClampMin = "1", ClampMax = "10"))
	int32 DebugEditorPreviewTabCount = 3;

	/**
	 * 注册页签时使用的按钮类，须配置可实例化的 UFrontendCommonButtonBase 派生类。
	 * TSubclassOf 保存类而非现成按钮；注册后按 TabID 获取按钮实例并设置显示文字。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Tab List Settings", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UFrontendCommonButtonBase> TabButtonEntryWidgetClass;
};
