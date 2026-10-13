// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/DataObjects/ListDataObject_Base.h"
#include "CommonInputTypeEnum.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ListDataObject_KeyRemap.generated.h"

class UEnhancedInputUserSettings;
class UEnhancedPlayerMappableKeyProfile;
/**
 * 键位数据直接连接 Enhanced Input 用户设置，不走普通 Value 的反射读写桥接。
 * Registry 从玩家保存的 profile 中为符合设备类型的映射创建列表项；KeyRemap 行把捕获到的新 FKey 交给它。
 * 当前键和默认键来自映射本身；修改与恢复默认都通过 Enhanced Input 的 SaveSettings 单独保存。
 */
UCLASS()
class FRONTENDUI_API UListDataObject_KeyRemap : public UListDataObject_Base
{
	GENERATED_BODY()
	
public:
	/**
	 * 缓存所属设置、读取 profile、映射名与 slot；不复制当前键，也不执行映射或保存。
	 * 后续读取与 Reset 要求该 profile 中的同名同 slot 映射持续存在。
	 * @param InOwningInputUserSettings 非空的本地玩家 Enhanced Input 用户设置，负责映射与保存。
	 * @param InKeyProfile 非空的映射所属 profile，用于重新查找当前键及默认键。
	 * @param InDesiredInputKeyType 行控件捕获输入及查找图标时使用的设备类型。
	 * @param InOwningPlayerKeyMapping 所属映射，提取 MappingName 和 Slot 作为后续定位信息。
	 */
	void InitKeyRemapData(UEnhancedInputUserSettings* InOwningInputUserSettings,UEnhancedPlayerMappableKeyProfile* InKeyProfile,ECommonInputType InDesiredInputKeyType,const FPlayerKeyMapping& InOwningPlayerKeyMapping);

	/**
	 * 按当前映射键、缓存设备类型与当前手柄名称，从 Common Input 平台配置查找图标。
	 * 须已初始化且映射可找到；用户设置与 Common Input 子系统由 check 验证。
	 * @return 匹配图标的 Brush；查找失败时返回默认 Brush 并输出调试提示。
	 */
	FSlateBrush GetIconFromCurrentKey() const;

	/**
	 * 以缓存映射名和 slot 请求 MapPlayerKey，随后立即 SaveSettings 并广播直接修改。
	 * 用户设置须有效；请求未显式指定缓存 profile，也未检查失败标签，所以通知不保证映射已成功。
	 * @param InNewKey 捕获界面提交的新键；本函数不校验键的有效性、设备类型或冲突。
	 */
	void BindNewInputKey(const FKey& InNewKey);

	//~ Begin UListDataObject_Base Interface
	/**
	 * 查询当前映射自带的默认键是否有效；调用前须完成初始化且映射仍可查到。
	 * @return 映射的 DefaultKey 有效时为 true。
	 */
	virtual bool HasDefaultValue() const override;
	/**
	 * 查询可定位映射是否具有有效默认键且已自定义，不修改映射。
	 * @return 有默认键并且映射 IsCustomized 为 true 时为 true。
	 */
	virtual bool CanResetBackToDefaultValue() const override;
	/**
	 * 可重置时直接恢复缓存 profile 内的映射，保存用户设置并广播 ResetToDefault。
	 * @return 执行恢复、保存与通知路径时为 true，不验证持久化结果；无需重置时为 false。
	 */
	virtual bool TryResetBackToDefaultValue() override;
	//~ End UListDataObject_Base Interface

private:
	/**
	 * 在缓存 profile 中用 MappingName 与 Slot 重新定位映射，不保存易过期的当前键副本。
	 * profile 必须有效，由 check 验证；当前调用方均要求映射存在并直接使用结果。
	 * @return profile 内的映射指针；未找到时结果可能为空，本函数不提供回退映射。
	 */
	FPlayerKeyMapping* GetOwningKeyMapping() const;

	/**
	 * 运行时保留所属玩家的 Enhanced Input 设置，用于改键与独立 SaveSettings 保存。
	 */
	UPROPERTY(Transient)
	UEnhancedInputUserSettings* CachedOwningInputUserSettings;

	/**
	 * 读取和直接 Reset 所属的 profile；BindNewInputKey 的请求参数并未显式指定它。
	 */
	UPROPERTY(Transient)
	UEnhancedPlayerMappableKeyProfile* CachedOwningKeyProfile;

	/**
	 * 由 Registry 的键鼠/手柄分类指定，供捕获页面过滤输入及平台图标匹配使用。
	 */
	ECommonInputType CachedDesiredInputKeyType;

	/**
	 * profile 中的映射名称，与 slot 一起定位映射。
	 */
	FName CachedOwningMappingName;

	/**
	 * 同一映射的键位槽，初始化时从源映射复制；读取、改键和 Reset 共用。
	 */
	EPlayerMappableKeySlot CachedOwningMappableKeySlot;

public:
	FORCEINLINE ECommonInputType GetDesiredInputKeyType() const { return CachedDesiredInputKeyType;}
};
