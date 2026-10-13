// Vince Petrelli All Rights Reserved


#include "FontendSettings/FrontendGameUserSettings.h"

// 构造时先给音频字段初值；已有的玩家配置可在后续加载流程中覆盖这些初值。
// 难度菜单的默认选项 Normal 则由 OptionsDataRegistry 中的 DataObject 指定。
UFrontendGameUserSettings::UFrontendGameUserSettings()
	: OverallVolume(1.f)
	, MusicVolume(1.f)
	, SoundFXVolume(1.f)
	, bAllowBackgroundAudio(false)
	, bUseHDRAudioMode(false)
{
}

UFrontendGameUserSettings* UFrontendGameUserSettings::Get()
{	
	if (GEngine)
	{
		// DefaultEngine.ini 必须把 GameUserSettingsClassName 配置为本类。
		// CastChecked 验证引擎返回的确实是这个派生类型，配置错误时及时暴露问题。
		return CastChecked<UFrontendGameUserSettings>(GEngine->GetGameUserSettings());
	}

	return nullptr;
}

void UFrontendGameUserSettings::SetOverallVolume(float InVolume)
{
	OverallVolume = InVolume;

	// 这里及下面的音频 setter 目前只保存数值；实际音量控制还需要音频系统接入。
}

void UFrontendGameUserSettings::SetMusicVolume(float InVolume)
{
	MusicVolume = InVolume;
}

void UFrontendGameUserSettings::SetSoundFXVolume(float InVolume)
{
	SoundFXVolume = InVolume;
}

void UFrontendGameUserSettings::SetAllowBackgroundAudio(bool bIsAllowed)
{
	bAllowBackgroundAudio = bIsAllowed;
}

void UFrontendGameUserSettings::SetUseHDRAudioMode(bool bIsAllowed)
{
	bUseHDRAudioMode = bIsAllowed;
}

// 亮度走引擎当前状态：滑条经反射调用这一对函数，并没有本类自己的 Gamma 缓存字段。
float UFrontendGameUserSettings::GetCurrentDisplayGamma() const
{	
	if (GEngine)
	{
		return GEngine->GetDisplayGamma();
	}

	return 0.0f;
}

void UFrontendGameUserSettings::SetCurrentDisplayGamma(float InNewGamma)
{
	if (GEngine)
	{
		// 直接修改当前引擎的显示亮度；本类没有为 Gamma 定义持久化的 Config 字段。
		GEngine->DisplayGamma = InNewGamma;
	}
}
