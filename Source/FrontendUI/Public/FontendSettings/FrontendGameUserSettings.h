// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "FrontendGameUserSettings.generated.h"

/**
 * 玩家设置的实际存放处：画质、窗口模式等沿用 UGameUserSettings，难度和音频由本类补充。
 * OptionsDataRegistry 给 DataObject 配好 getter/setter，DataObject 再经反射读写这里的值。
 * 以难度为例：列表操作 → ListDataObject_String → SetCurrentGameDifficulty → 通知并应用设置。
 * 设置值与显示控件分开，因此关闭设置页面并不等于丢弃这些值。
 */
UCLASS()
class FRONTENDUI_API UFrontendGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()
	
public:
	/** 提供音量及音频开关的构造初值；已保存配置可在后续加载流程中覆盖这些值。 */
	UFrontendGameUserSettings();

	/**
	 * 访问引擎维护的 Frontend 玩家设置对象，不创建新实例。
	 * @return 当前派生设置对象；GEngine 不存在时返回 nullptr。
	 * @warning GameUserSettingsClassName 必须配置为本类或其派生类，否则 CastChecked 检查失败。
	 */
	static UFrontendGameUserSettings* Get();

	// UFUNCTION 让 getter/setter 进入 UE 反射系统，设置列表才能按函数名读写它们。
	// 反射可以理解为“UE 能按名字找到函数”；这里不需要为每种行控件硬写一套直接调用。
	//***** Gameplay Collection Tab ***** //
	/** 返回保存的难度字符串，供 Registry 配置的反射 getter 使用；尚未赋值时可为空。 */
	UFUNCTION()
	FString GetCurrentGameDifficulty() const { return CurrentGameDifficulty;}

	/**
	 * 写入难度字符串，供设置数据对象调用；本函数不应用战斗规则、广播或保存配置。
	 * @param InNewDifficulty - 实际设置值，例如 Normal；不验证其是否属于 Registry 候选。
	 */
	UFUNCTION()
	void SetCurrentGameDifficulty(const FString& InNewDifficulty) { CurrentGameDifficulty = InNewDifficulty;}
	//***** Gameplay Collection Tab ***** //

	//***** Audio Collection Tab ***** //
	/** 返回保存的总音量系数；不查询音频设备当前输出。 */
	UFUNCTION()
	float GetOverallVolume() const { return OverallVolume;}

	/**
	 * 更新总音量字段；当前未接入实际音频控制，也不自动保存配置。
	 * @param InVolume - 待保存的音量系数；UI 注册范围为 0..2，本函数不钳制。
	 */
	UFUNCTION()
	void SetOverallVolume(float InVolume);

	/** 返回保存的音乐音量系数，供音量行经反射读取。 */
	UFUNCTION()
	float GetMusicVolume() const { return MusicVolume;}

	/**
	 * 更新音乐音量字段；当前不调整声音对象或自动保存配置。
	 * @param InVolume - 待保存的音量系数；UI 注册范围为 0..2，本函数不钳制。
	 */
	UFUNCTION()
	void SetMusicVolume(float InVolume);

	/** 返回保存的音效音量系数，供音量行经反射读取。 */
	UFUNCTION()
	float GetSoundFXVolume() const { return SoundFXVolume;}

	/**
	 * 更新音效音量字段；当前不调整声音对象或自动保存配置。
	 * @param InVolume - 待保存的音量系数；UI 注册范围为 0..2，本函数不钳制。
	 */
	UFUNCTION()
	void SetSoundFXVolume(float InVolume);

	/** 返回保存的后台音频偏好，不代表平台音频行为已同步。 */
	UFUNCTION()
	bool GetAllowBackgroundAudio() const { return bAllowBackgroundAudio;}

	/**
	 * 记录后台音频偏好；当前不切换实际后台播放行为。
	 * @param bIsAllowed - 是否期望允许后台音频。
	 */
	UFUNCTION()
	void SetAllowBackgroundAudio(bool bIsAllowed);

	/** 返回保存的 HDR Audio 偏好；当前没有读取音频系统状态。 */
	UFUNCTION()
	bool GetUseHDRAudioMode() const { return bUseHDRAudioMode;}

	/**
	 * 记录 HDR Audio 偏好；当前不切换实际音频处理模式。
	 * @param bIsAllowed - 是否期望启用 HDR Audio。
	 */
	UFUNCTION()
	void SetUseHDRAudioMode(bool bIsAllowed);
	//***** Audio Collection Tab ***** //

	//***** Video Collection Tab ***** //
	/**
	 * 读取 GEngine 当前显示 Gamma，供亮度滑条回读。
	 * @return 引擎显示 Gamma；GEngine 不存在时返回 0，而非配置默认值。
	 */
	UFUNCTION()
	float GetCurrentDisplayGamma() const;

	/**
	 * 直接更新当前引擎的显示 Gamma；GEngine 不存在时跳过。
	 * @param InNewGamma - Gamma 数值；UI 输出范围为 1.7..2.7，此函数不校验范围。
	 * @note 本类没有对应 Config 字段，此写入不建立 Gamma 的重启持久化。
	 */
	UFUNCTION()
	void SetCurrentDisplayGamma(float InNewGamma);

	//***** Video Collection Tab ***** //

private:
	// Config 字段参加基类的 GameUserSettings.ini 加载/保存流程；setter 赋值本身不会写磁盘。
	// 是否立即应用由 DataObject 的开关决定，统一入口见 NotifyListDataModified；不要与项目配置混淆。
	//***** Gameplay Collection Tab ***** //
	/** 保存难度字符串；UI 的 Normal 默认候补由 Registry 定义，并非此字段的构造赋值。 */
	UPROPERTY(Config)
	FString CurrentGameDifficulty;
	//***** Gameplay Collection Tab ***** //

	//***** Audio Collection Tab ***** //
	/** 总音量系数，构造值为 1；UI 将 0..2 映射为 0%..100%，并不直接控制声音。 */
	UPROPERTY(Config)
	float OverallVolume;

	/** 音乐音量系数，构造值为 1；随 GameUserSettings 配置保存。 */
	UPROPERTY(Config)
	float MusicVolume;

	/** 音效音量系数，构造值为 1；随 GameUserSettings 配置保存。 */
	UPROPERTY(Config)
	float SoundFXVolume;

	/** 是否期望允许后台音频，构造值 false；目前仅持有偏好。 */
	UPROPERTY(Config)
	bool bAllowBackgroundAudio;

	/** 是否期望启用 HDR Audio，构造值 false；目前仅持有偏好。 */
	UPROPERTY(Config)
	bool bUseHDRAudioMode;
	//***** Audio Collection Tab ***** //
};
