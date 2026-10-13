// Vince Petrelli All Rights Reserved


#include "Widgets/Options/DataObjects/ListDataObject_KeyRemap.h"
#include "CommonInputBaseTypes.h"
#include "CommonInputSubsystem.h"

#include "FrontendDebugHelper.h"

// Registry 遍历保存的映射时调用：保存其所属设置、profile 和定位信息，供后续图标、改键、Reset 共用。
// DesiredInputKeyType 还会被行控件传给捕获页面，用来区分本项接收键鼠输入还是手柄输入。
void UListDataObject_KeyRemap::InitKeyRemapData(UEnhancedInputUserSettings* InOwningInputUserSettings, UEnhancedPlayerMappableKeyProfile* InKeyProfile, ECommonInputType InDesiredInputKeyType, const FPlayerKeyMapping& InOwningPlayerKeyMapping)
{
	// profile 是玩家键位配置；MappingName 标识动作映射，Slot 区分同一映射的不同键位槽。
	CachedOwningInputUserSettings = InOwningInputUserSettings;
	CachedOwningKeyProfile = InKeyProfile;
	CachedDesiredInputKeyType = InDesiredInputKeyType;
	CachedOwningMappingName = InOwningPlayerKeyMapping.GetMappingName();
	CachedOwningMappableKeySlot = InOwningPlayerKeyMapping.GetSlot();
}

// KeyRemap 行绑定数据或收到修改通知后调用；返回按钮可用的 Brush，没有匹配素材时返回空 Brush。
FSlateBrush UListDataObject_KeyRemap::GetIconFromCurrentKey() const
{	
	check(CachedOwningInputUserSettings);

	FSlateBrush FoundBrush;

	UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(CachedOwningInputUserSettings->GetLocalPlayer());

	check(CommonInputSubsystem);

	// 图标由当前按键、输入设备类型和手柄名称共同决定，素材来自 Common Input 平台设置。
	const bool bHasFoundBrush = UCommonInputPlatformSettings::Get()->TryGetInputBrush(
		FoundBrush,
		GetOwningKeyMapping()->GetCurrentKey(),
		CachedDesiredInputKeyType,
		CommonInputSubsystem->GetCurrentGamepadName()
	);

	if (!bHasFoundBrush)
	{
		Debug::Print(
			TEXT("Unable to find an icon for the key ") +
			GetOwningKeyMapping()->GetCurrentKey().GetDisplayName().ToString() +
			TEXT(" Empty brush was applied.")
		);
	}

	return FoundBrush;
}

// 捕获页面把 FKey 回传给行控件，行的 OnKeyToRemapPressed 再调用这里；本类不负责捕获输入。
void UListDataObject_KeyRemap::BindNewInputKey(const FKey& InNewKey)
{
	check(CachedOwningInputUserSettings);

	FMapPlayerKeyArgs KeyArgs;
	KeyArgs.MappingName = CachedOwningMappingName;
	KeyArgs.Slot = CachedOwningMappableKeySlot;
	KeyArgs.NewKey = InNewKey;

	// 读取使用缓存 profile，写入参数在此只指定映射名、槽和新键，未显式指定 profile。
	FGameplayTagContainer Container;
	// 键位修改后立即单独保存 Enhanced Input 用户设置，不等待 Options 退出时的 ApplySettings。
	CachedOwningInputUserSettings->MapPlayerKey(KeyArgs,Container);
	CachedOwningInputUserSettings->SaveSettings();

	NotifyListDataModified(this);
}

bool UListDataObject_KeyRemap::HasDefaultValue() const
{
	return GetOwningKeyMapping()->GetDefaultKey().IsValid();
}

bool UListDataObject_KeyRemap::CanResetBackToDefaultValue() const
{
	return HasDefaultValue() && GetOwningKeyMapping()->IsCustomized();
}

// 单行重置按钮与整页 Reset 都调用同一入口；默认键来自映射，未在 Registry 另配默认字符串。
bool UListDataObject_KeyRemap::TryResetBackToDefaultValue()
{	
	if (CanResetBackToDefaultValue())
	{	
		check(CachedOwningInputUserSettings);

		// 恢复的是该映射自带的 DefaultKey，随后仍单独保存并通知行控件刷新。
		GetOwningKeyMapping()->ResetToDefault();

		CachedOwningInputUserSettings->SaveSettings();

		NotifyListDataModified(this,EOptionsListDataModifyReason::ResetToDefault);

		return true;
	}

	return false;
}

// 每次根据缓存的名称和 slot 到所属 profile 查找原映射，图标和 Reset 因而读取同一条映射的当前状态。
FPlayerKeyMapping* UListDataObject_KeyRemap::GetOwningKeyMapping() const
{	
	check(CachedOwningKeyProfile);

	FMapPlayerKeyArgs KeyArgs;
	KeyArgs.MappingName = CachedOwningMappingName;
	KeyArgs.Slot = CachedOwningMappableKeySlot;

	return CachedOwningKeyProfile->FindKeyMapping(KeyArgs);
}
