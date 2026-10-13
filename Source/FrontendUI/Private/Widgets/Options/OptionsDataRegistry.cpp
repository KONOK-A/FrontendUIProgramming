// Vince Petrelli All Rights Reserved


#include "Widgets/Options/OptionsDataRegistry.h"
#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"
#include "Widgets/Options/DataObjects/ListDataObject_String.h"
#include "Widgets/Options/OptionsDataInteractionHelper.h"
#include "FontendSettings/FrontendGameUserSettings.h"
#include "FrontendFunctionLibrary.h"
#include "FrontendGameplayTags.h"
#include "Widgets/Options/DataObjects/ListDataObject_Scalar.h"
#include "Widgets/Options/DataObjects/ListDataObject_StringResolution.h"
#include "Internationalization/StringTableRegistry.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "Widgets/Options/DataObjects/ListDataObject_KeyRemap.h"

#include "FrontendDebugHelper.h"

// 将已知的设置函数名变成 Helper 的路径：GET_FUNCTION_NAME_STRING_CHECKED 检查 C++ 函数名是否存在。
// 运行时还要通过 UE 反射按路径调用；自定义设置 getter/setter 因此声明为 UFUNCTION。
// 每次宏调用创建一个 Helper，所以 getter 与 setter 各自保存自己的函数路径。
#define MAKE_OPTIONS_DATA_CONTROL(SetterOrGetterFuncName) \
	MakeShared<FOptionsDataInteractionHelper>(GET_FUNCTION_NAME_STRING_CHECKED(UFrontendGameUserSettings,SetterOrGetterFuncName))

// 描述从字符串表按 key 读取 FText，便于在资产中维护和本地化。
#define GET_DESCRIPTION(InKey) LOCTABLE("/Game/UI/StringTables/ST_OptionsScreenDescription.ST_OptionsScreenDescription", InKey)

// OptionsScreen 的 GetOrCreateDataRegistry 只在首次创建 Registry 后调用此入口。
// 四个 Init 函数组装各页数据；只有 Control 需要本地玩家，用来取得该玩家的 Enhanced Input 设置。
void UOptionsDataRegistry::InitOptionsDataRegistry(ULocalPlayer* InOwningLocalPlayer)
{
	InitGameplayCollectionTab();
	InitAudioCollectionTab();
	InitVideoCollectionTab();
	InitControlCollectionTab(InOwningLocalPlayer);
}

// 将选中页签的数据树按父在前、子在后的顺序展开；分类节点也作为列表标题保留。
TArray<UListDataObject_Base*> UOptionsDataRegistry::GetListSourceItemsBySelectedTabID(const FName& InSelectedTabID) const
{
	// 页签按钮使用集合的 DataID；这里按 ID 找集合，不靠玩家看到的 Gameplay/Audio 等显示文本。
	UListDataObject_Collection* const* FoundTabCollectionPtr = RegisteredOptionsTabCollections.FindByPredicate(
		[InSelectedTabID](UListDataObject_Collection* AvailableTabCollection)->bool
		{
			return AvailableTabCollection->GetDataID() == InSelectedTabID;
		}
	);

	checkf(FoundTabCollectionPtr,TEXT("No valid tab found under the ID %s"),*InSelectedTabID.ToString());

	UListDataObject_Collection* FoundTabCollection = *FoundTabCollectionPtr;

	TArray<UListDataObject_Base*> AllChildListItems;

	for (UListDataObject_Base* ChildListData : FoundTabCollection->GetAllChildListData())
	{
		if (!ChildListData)
		{
			continue;
		}

		AllChildListItems.Add(ChildListData);

		if (ChildListData->HasAnyChildListData())
		{
			FindChildListDataRecursively(ChildListData,AllChildListItems);
		}
	}

	return AllChildListItems;
}

// 递归把每层直接子项追加到同一个结果数组，先加入分类，再展开分类内部，不重复加入当前父节点。
// 例如 Audio 最终成为 Volume、OverallVolume、MusicVolume、SoundFXVolume、Sound、两个开关。
void UOptionsDataRegistry::FindChildListDataRecursively(UListDataObject_Base* InParentData, TArray<UListDataObject_Base*>& OutFoundChildListData) const
{
	if (!InParentData || !InParentData->HasAnyChildListData())
	{
		return;
	}

	for (UListDataObject_Base* SubChildListData : InParentData->GetAllChildListData())
	{
		if (!SubChildListData)
		{
			continue;
		}

		OutFoundChildListData.Add(SubChildListData);

		if (SubChildListData->HasAnyChildListData())
		{
			FindChildListDataRecursively(SubChildListData,OutFoundChildListData);
		}
	}
}

// Gameplay 页可作为阅读入口，沿 Difficulty 串起整个设置流程：
// Registry 配置 String 数据 → 加入集合触发初始化 → Helper 调 GetCurrentGameDifficulty 取得当前值。
// 切页后 String 行显示候选；用户点 Next → AdvanceToNextOption → Helper 调 SetCurrentGameDifficulty。
// 数据调用 Base::NotifyListDataModified → 行刷新、页面更新 Reset → 按立即应用开关执行 ApplySettings。
void UOptionsDataRegistry::InitGameplayCollectionTab()
{
	UListDataObject_Collection* GameplayTabCollection = NewObject<UListDataObject_Collection>();
	GameplayTabCollection->SetDataID(FName("GameplayTabCollection"));
	GameplayTabCollection->SetDataDisplayName(FText::FromString(TEXT("Gameplay")));

	//This is the full code for constructing data interactor helper
	/*TSharedPtr<FOptionsDataInteractionHelper> ConstructedHelper = 
	MakeShared<FOptionsDataInteractionHelper>(
		GET_FUNCTION_NAME_STRING_CHECKED(UFrontendGameUserSettings,GetCurrentGameDifficulty)
	);*/
	
	// 典型注册顺序：创建数据 → 身份与文案 → 候选和默认值 → getter/setter → 加入集合。
	// AddChildListData 会立即初始化；最后加入可确保初始化时能读到完整配置。
	//Game Difficulty
	{
		UListDataObject_String* GameDifficulty = NewObject<UListDataObject_String>();
		GameDifficulty->SetDataID(FName("GameDifficulty"));
		GameDifficulty->SetDataDisplayName(FText::FromString(TEXT("Difficulty")));
		GameDifficulty->SetDescriptionRichText(FText::FromString(TEXT("Adjusts the difficulty of the game experience.\n\n<Bold>Easy:</> Focuses on the story experience. Provides the most relaxing combat.\n\n<Bold>Normal:</> Offers slightly harder combat experience\n\n<Bold>Hard:</> Offers a much more challenging combat experience\n\n<Bold>Vert Hard:</> Provides the most challenging combat experience. Not recommended for first play through.")));
		// 每对参数分别是设置实际值和显示文本；难度中两者恰好相同，Bool 中则可能是 true 与 Enabled。
		GameDifficulty->AddDynamicOption(TEXT("Easy"),FText::FromString(TEXT("Easy")));
		GameDifficulty->AddDynamicOption(TEXT("Normal"),FText::FromString(TEXT("Normal")));
		GameDifficulty->AddDynamicOption(TEXT("Hard"),FText::FromString(TEXT("Hard")));
		GameDifficulty->AddDynamicOption(TEXT("Very Hard"),FText::FromString(TEXT("Very Hard")));
		// Normal 是本 UI 注册的 Reset 目标，也是在 getter 为空时使用的初始候补；这里不会调用 setter。
		GameDifficulty->SetDefaultValueFromString(TEXT("Normal"));
		// 把读/写函数交给数据对象后，String 行无需知道 UFrontendGameUserSettings 的具体字段。
		GameDifficulty->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetCurrentGameDifficulty));
		GameDifficulty->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetCurrentGameDifficulty));
		GameDifficulty->SetShouldApplySettingsImmediately(true);

		GameplayTabCollection->AddChildListData(GameDifficulty);
	}

	// 这个演示项只配说明和图片，没有候选或 getter/setter；它没有普通可切换设置的完整读写链路。
	//Test Item
	{
		UListDataObject_String* TestItem = NewObject<UListDataObject_String>();
		TestItem->SetDataID(FName("TestItem"));
		TestItem->SetDataDisplayName(FText::FromString(TEXT("Test Image Item")));	
		TestItem->SetSoftDescriptionImage(UFrontendFunctionLibrary::GetOptionsSoftImageByTag(FrontendGameplayTags::Frontend_Image_TestImage));
		TestItem->SetDescriptionRichText(FText::FromString(TEXT("The image to display can be specified in the project settings. It can be anything the developer assigned in there")));

		GameplayTabCollection->AddChildListData(TestItem);
	}

	RegisteredOptionsTabCollections.Add(GameplayTabCollection);
}

// Audio 页：Volume 分类放滑条，Sound 分类放布尔选项；分类本身也是 Collection。
// 这些音量项只连接设置字段，滑条变化本身不能证明实际声音控制已接入。
void UOptionsDataRegistry::InitAudioCollectionTab()
{
	UListDataObject_Collection* AudioTabCollection = NewObject<UListDataObject_Collection>();
	AudioTabCollection->SetDataID(FName("AudioTabCollection"));
	AudioTabCollection->SetDataDisplayName(FText::FromString(TEXT("Audio")));

	//Volume Category
	{
		UListDataObject_Collection* VolumeCategoryCollection = NewObject<UListDataObject_Collection>();
		VolumeCategoryCollection->SetDataID(FName("VolumeCategoryCollection"));
		VolumeCategoryCollection->SetDataDisplayName(FText::FromString(TEXT("Volume")));

		AudioTabCollection->AddChildListData(VolumeCategoryCollection);

		// Scalar 示例：显示 0..1 格式化为 0..100%，写入范围却是 0..2。
		// 所以输出默认值 1 对应界面 50%；拖到 75% 会写入 1.5。步长 0.01 属于显示区间。
		// 数字格式和小数位由行控件读取后设置；下方两个音量项复用同一注册模式。
		//Overall Volume
		{
			UListDataObject_Scalar* OverallVolume = NewObject<UListDataObject_Scalar>();
			OverallVolume->SetDataID(FName("OverallVolume"));
			OverallVolume->SetDataDisplayName(FText::FromString(TEXT("Overall Volume")));
			OverallVolume->SetDescriptionRichText(FText::FromString(TEXT("This is description for Overall Volume")));
			OverallVolume->SetDisplayValueRange(TRange<float>(0.f,1.f));
			OverallVolume->SetOutputValueRange(TRange<float>(0.f,2.f));
			OverallVolume->SetSliderStepSize(0.01f);
			OverallVolume->SetDefaultValueFromString(LexToString(1.f));
			OverallVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			OverallVolume->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal());  //No Decimal: 50%  //One Decimal: 50.5%
			OverallVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetOverallVolume));
			OverallVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetOverallVolume));
			OverallVolume->SetShouldApplySettingsImmediately(true);

			VolumeCategoryCollection->AddChildListData(OverallVolume);
		}

		//Music Volume
		{
			UListDataObject_Scalar* MusicVolume = NewObject<UListDataObject_Scalar>();
			MusicVolume->SetDataID(FName("MusicVolume"));
			MusicVolume->SetDataDisplayName(FText::FromString(TEXT("Music Volume")));
			MusicVolume->SetDescriptionRichText(FText::FromString(TEXT("This is description for Music Volume")));
			MusicVolume->SetDisplayValueRange(TRange<float>(0.f,1.f));
			MusicVolume->SetOutputValueRange(TRange<float>(0.f,2.f));
			MusicVolume->SetSliderStepSize(0.01f);
			MusicVolume->SetDefaultValueFromString(LexToString(1.f));
			MusicVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			MusicVolume->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal());  //No Decimal: 50%  //One Decimal: 50.5%
			MusicVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetMusicVolume));
			MusicVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetMusicVolume));
			MusicVolume->SetShouldApplySettingsImmediately(true);

			VolumeCategoryCollection->AddChildListData(MusicVolume);
		}

		//Sound FX Volume
		{
			UListDataObject_Scalar* SoundFXVolume = NewObject<UListDataObject_Scalar>();
			SoundFXVolume->SetDataID(FName("SoundFXVolume"));
			SoundFXVolume->SetDataDisplayName(FText::FromString(TEXT("Sound Effects Volume")));
			SoundFXVolume->SetDescriptionRichText(FText::FromString(TEXT("This is description for Sound Effects Volume")));
			SoundFXVolume->SetDisplayValueRange(TRange<float>(0.f,1.f));
			SoundFXVolume->SetOutputValueRange(TRange<float>(0.f,2.f));
			SoundFXVolume->SetSliderStepSize(0.01f);
			SoundFXVolume->SetDefaultValueFromString(LexToString(1.f));
			SoundFXVolume->SetDisplayNumericType(ECommonNumericType::Percentage);
			SoundFXVolume->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal());  //No Decimal: 50%  //One Decimal: 50.5%
			SoundFXVolume->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetSoundFXVolume));
			SoundFXVolume->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetSoundFXVolume));
			SoundFXVolume->SetShouldApplySettingsImmediately(true);

			VolumeCategoryCollection->AddChildListData(SoundFXVolume);
		}
	}

	//Sound Category
	{
		UListDataObject_Collection* SoundCategoryCollection = NewObject<UListDataObject_Collection>();
		SoundCategoryCollection->SetDataID(FName("SoundCategoryCollection"));
		SoundCategoryCollection->SetDataDisplayName(FText::FromString(TEXT("Sound")));

		AudioTabCollection->AddChildListData(SoundCategoryCollection);

		// Bool 仍使用 String 行；只改变 true/false 的显示文案，就能复用同一组前后切换与 Reset 逻辑。
		//Allow Background Audio
		{
			UListDataObject_StringBool* AllowBackgroundAudio = NewObject<UListDataObject_StringBool>();
			AllowBackgroundAudio->SetDataID(FName("AllowBackgroundAudio"));
			AllowBackgroundAudio->SetDataDisplayName(FText::FromString(TEXT("Allow Background Audio")));
			AllowBackgroundAudio->OverrideTrueDisplayText(FText::FromString(TEXT("Enabled")));
			AllowBackgroundAudio->OverrideFalseDisplayText(FText::FromString(TEXT("Disabled")));
			AllowBackgroundAudio->SetFalseAsDefaultValue();
			AllowBackgroundAudio->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetAllowBackgroundAudio));
			AllowBackgroundAudio->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetAllowBackgroundAudio));
			AllowBackgroundAudio->SetShouldApplySettingsImmediately(true);

			SoundCategoryCollection->AddChildListData(AllowBackgroundAudio);
		}

		//Use HDR Audio
		{
			UListDataObject_StringBool* UseHDRAudioMode = NewObject<UListDataObject_StringBool>();
			UseHDRAudioMode->SetDataID(FName("UseHDRAudioMode"));
			UseHDRAudioMode->SetDataDisplayName(FText::FromString(TEXT("Use HDR Audio Mode")));
			UseHDRAudioMode->OverrideTrueDisplayText(FText::FromString(TEXT("Enabled")));
			UseHDRAudioMode->OverrideFalseDisplayText(FText::FromString(TEXT("Disabled")));
			UseHDRAudioMode->SetFalseAsDefaultValue();
			UseHDRAudioMode->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetUseHDRAudioMode));
			UseHDRAudioMode->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetUseHDRAudioMode));
			UseHDRAudioMode->SetShouldApplySettingsImmediately(true);

			SoundCategoryCollection->AddChildListData(UseHDRAudioMode);
		}
	}

	RegisteredOptionsTabCollections.Add(AudioTabCollection);
}

// Video 页：Display 处理显示模式及条件依赖，Graphics 处理画质联动，Advanced 放额外开关。
void UOptionsDataRegistry::InitVideoCollectionTab()
{
	UListDataObject_Collection* VideoTabCollection = NewObject<UListDataObject_Collection>();
	VideoTabCollection->SetDataID(FName("VideoTabCollection"));
	VideoTabCollection->SetDataDisplayName(FText::FromString(TEXT("Video")));

	UListDataObject_StringEnum* CreatedWindowMode = nullptr;

	//Display Category
	{
		UListDataObject_Collection* DisplayCategoryCollection = NewObject<UListDataObject_Collection>();
		DisplayCategoryCollection->SetDataID(FName("DisplayCategoryCollection"));
		DisplayCategoryCollection->SetDataDisplayName(FText::FromString(TEXT("Display")));

		VideoTabCollection->AddChildListData(DisplayCategoryCollection);

		// Descriptor 保存这段 lambda，等行控件查询可编辑状态时才执行；Editor/PIE 下结果为 false。
		// 同一条规则分别添加给 WindowMode 和 ScreenResolution，不必把判断逻辑复制到两个行控件。
		FOptionsDataEditConditionDescriptor PackagedBuildOnlyCondition;
		PackagedBuildOnlyCondition.SetEditConditionFunc(
			[]()->bool
			{
				const bool bIsInEditor = GIsEditor || GIsPlayInEditorWorld;

				return !bIsInEditor;
			}
		);
		PackagedBuildOnlyCondition.SetDisabledRichReason(TEXT("\n\n<Disabled>This setting can only be adjusted in a packaged build.</>"));

		//Window Mode
		{	
			/*const FText WindowModeDesc = LOCTABLE("/Game/UI/StringTables/ST_OptionsScreenDescription.ST_OptionsScreenDescription", "WindowModeDescKey");*/

			UListDataObject_StringEnum* WindowMode = NewObject<UListDataObject_StringEnum>();
			WindowMode->SetDataID(FName("WindowMode"));
			WindowMode->SetDataDisplayName(FText::FromString(TEXT("Window Mode")));
			WindowMode->SetDescriptionRichText(GET_DESCRIPTION("WindowModeDescKey"));
			// Enum 数据用反射把枚举项转为名称字符串；显示文本 Fullscreen Mode 可以与枚举名称不同。
			WindowMode->AddEnumOption(EWindowMode::Fullscreen,FText::FromString(TEXT("Fullscreen Mode")));
			WindowMode->AddEnumOption(EWindowMode::WindowedFullscreen,FText::FromString(TEXT("Borderless Window")));
			WindowMode->AddEnumOption(EWindowMode::Windowed,FText::FromString(TEXT("Windowed")));
			WindowMode->SetDefaultValueFromEnumOption(EWindowMode::WindowedFullscreen);
			WindowMode->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetFullscreenMode));
			WindowMode->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetFullscreenMode));
			WindowMode->SetShouldApplySettingsImmediately(true);

			WindowMode->AddEditCondition(PackagedBuildOnlyCondition);

			CreatedWindowMode = WindowMode;

			DisplayCategoryCollection->AddChildListData(WindowMode);
		}

		//Screen Resolution
		{
			UListDataObject_StringResolution* ScreenResolution = NewObject<UListDataObject_StringResolution>();
			ScreenResolution->SetDataID(FName("ScreenResolution"));
			ScreenResolution->SetDataDisplayName(FText::FromString(TEXT("Screen Resolution")));
			ScreenResolution->SetDescriptionRichText(GET_DESCRIPTION("ScreenResolutionsDescKey"));
			ScreenResolution->InitResolutionValues();
			ScreenResolution->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetScreenResolution));
			ScreenResolution->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetScreenResolution));
			ScreenResolution->SetShouldApplySettingsImmediately(true);

			ScreenResolution->AddEditCondition(PackagedBuildOnlyCondition);

			FOptionsDataEditConditionDescriptor WindowModeEditCondition;
			WindowModeEditCondition.SetEditConditionFunc(
				[CreatedWindowMode]()->bool
				{
					const bool bIsBoderlessWindow = CreatedWindowMode->GetCurrentValueAsEnum<EWindowMode::Type>() == EWindowMode::WindowedFullscreen;

					return !bIsBoderlessWindow;
				}
			);
			WindowModeEditCondition.SetDisabledRichReason(TEXT("\n\n<Disabled>Screen Resolution is not adjustable when the 'Window Mode' is set to Borderless Window.The value must match with the maximum allowed resolution.</>"));
			WindowModeEditCondition.SetDisabledForcedStringValue(ScreenResolution->GetMaximumAllowedResolution());

			// 条件决定能否编辑及禁用时的强制值；订阅则让 WindowMode 变化时通知该行重查条件。
			// 条件失败时，强制值才在 IsDataCurrentlyEditable 查询过程中写入。
			// 例如切到 Borderless：WindowMode 广播 → 分辨率行重查条件 → 强制最大候选并禁用该行。
			// 后续切回可编辑模式只解除禁用，没有缓存并恢复此前用户选择的尺寸。
			ScreenResolution->AddEditCondition(WindowModeEditCondition);

			ScreenResolution->AddEditDependencyData(CreatedWindowMode);

			DisplayCategoryCollection->AddChildListData(ScreenResolution);
		}
	}

	//Graphics Category
	{
		UListDataObject_Collection* GraphicsCategoryCollection = NewObject<UListDataObject_Collection>();
		GraphicsCategoryCollection->SetDataID(FName("GraphicsCategoryCollection"));
		GraphicsCategoryCollection->SetDataDisplayName(FText::FromString(TEXT("Graphics")));

		VideoTabCollection->AddChildListData(GraphicsCategoryCollection);

		// Brightness 也用 Scalar，但输出是 gamma 1.7..2.7；setter 直接写 GEngine->DisplayGamma。
		// 这里没有开启立即 Apply；该字段的实际显示效果由 setter 的行为决定。
		//Display Gamma
		{
			UListDataObject_Scalar* DisplayGamma = NewObject<UListDataObject_Scalar>();
			DisplayGamma->SetDataID(FName("DisplayGamma"));
			DisplayGamma->SetDataDisplayName(FText::FromString(TEXT("Brightness")));
			DisplayGamma->SetDescriptionRichText(GET_DESCRIPTION("DisplayGammaDescKey"));
			DisplayGamma->SetDisplayValueRange(TRange<float>(0.f,1.f));
			DisplayGamma->SetOutputValueRange(TRange<float>(1.7f,2.7f)); //The default value Unreal has is: 2.2f
			DisplayGamma->SetSliderStepSize(0.01f);
			DisplayGamma->SetDisplayNumericType(ECommonNumericType::Percentage);
			DisplayGamma->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal());
			DisplayGamma->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetCurrentDisplayGamma));
			DisplayGamma->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetCurrentDisplayGamma));
			DisplayGamma->SetDefaultValueFromString(LexToString(2.2f));

			GraphicsCategoryCollection->AddChildListData(DisplayGamma);
		}

		UListDataObject_StringInteger* CreatedOverallQuality = nullptr;

		// 这组总体/单项画质及 3D Resolution 未注册默认值，所以不会进入普通值项的页签 Reset 集合。
		//Overall Quality
		{
			UListDataObject_StringInteger* OverallQuality = NewObject<UListDataObject_StringInteger>();
			OverallQuality->SetDataID(FName("OverallQuality"));
			OverallQuality->SetDataDisplayName(FText::FromString(TEXT("Overall Quality")));
			OverallQuality->SetDescriptionRichText(GET_DESCRIPTION("OverallQualityDescKey"));
			OverallQuality->AddIntegerOption(0,FText::FromString(TEXT("Low")));
			OverallQuality->AddIntegerOption(1,FText::FromString(TEXT("Medium")));
			OverallQuality->AddIntegerOption(2,FText::FromString(TEXT("High")));
			OverallQuality->AddIntegerOption(3,FText::FromString(TEXT("Epic")));
			OverallQuality->AddIntegerOption(4,FText::FromString(TEXT("Cinematic")));
			OverallQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetOverallScalabilityLevel));
			OverallQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetOverallScalabilityLevel));
			OverallQuality->SetShouldApplySettingsImmediately(true);

			GraphicsCategoryCollection->AddChildListData(OverallQuality);

			CreatedOverallQuality = OverallQuality;
		}

		//Resolution Scale
		{
			UListDataObject_Scalar* ResolutionScale = NewObject<UListDataObject_Scalar>();
			ResolutionScale->SetDataID(FName("ResolutionScale"));
			ResolutionScale->SetDataDisplayName(FText::FromString(TEXT("3D Resolution")));
			ResolutionScale->SetDescriptionRichText(GET_DESCRIPTION("ResolutionScaleDescKey"));
			ResolutionScale->SetDisplayValueRange(TRange<float>(0.f,1.f));
			ResolutionScale->SetOutputValueRange(TRange<float>(0.f,1.f));
			ResolutionScale->SetSliderStepSize(0.01f);
			ResolutionScale->SetDisplayNumericType(ECommonNumericType::Percentage);
			ResolutionScale->SetNumberFormattingOptions(UListDataObject_Scalar::NoDecimal());
			ResolutionScale->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetResolutionScaleNormalized));
			ResolutionScale->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetResolutionScaleNormalized));
			ResolutionScale->SetShouldApplySettingsImmediately(true);

			// 这里只做单向订阅：总体画质变动后，3D Resolution 从设置回读最新比例。
			ResolutionScale->AddEditDependencyData(CreatedOverallQuality);

			GraphicsCategoryCollection->AddChildListData(ResolutionScale);
		}

		// 整数画质示例：总体画质影响各子项，子项改变又可能让总体显示 Custom，因此双向订阅。
		// 下方第一条订阅让本项跟随总体变化；第二条反向订阅让总体重新判断是否还能匹配预设档位。
		// StringInteger 回读值相同就停止通知；后续七个画质项沿用这里的注册与联动模式。
		//Global Illumination Quality
		{
			UListDataObject_StringInteger* GlobalIlluminationQuality = NewObject<UListDataObject_StringInteger>();
			GlobalIlluminationQuality->SetDataID(FName("GlobalIlluminationQuality"));
			GlobalIlluminationQuality->SetDataDisplayName(FText::FromString(TEXT("Global Illumination")));
			GlobalIlluminationQuality->SetDescriptionRichText(GET_DESCRIPTION("GlobalIlluminationQualityDescKey"));
			GlobalIlluminationQuality->AddIntegerOption(0,FText::FromString(TEXT("Low")));
			GlobalIlluminationQuality->AddIntegerOption(1,FText::FromString(TEXT("Medium")));
			GlobalIlluminationQuality->AddIntegerOption(2,FText::FromString(TEXT("High")));
			GlobalIlluminationQuality->AddIntegerOption(3,FText::FromString(TEXT("Epic")));
			GlobalIlluminationQuality->AddIntegerOption(4,FText::FromString(TEXT("Cinematic")));
			GlobalIlluminationQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetGlobalIlluminationQuality));
			GlobalIlluminationQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetGlobalIlluminationQuality));
			GlobalIlluminationQuality->SetShouldApplySettingsImmediately(true);

			GlobalIlluminationQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(GlobalIlluminationQuality);

			GraphicsCategoryCollection->AddChildListData(GlobalIlluminationQuality);
		}

		//Shadow Quality
		{
			UListDataObject_StringInteger* ShadowQuality = NewObject<UListDataObject_StringInteger>();
			ShadowQuality->SetDataID(FName("ShadowQuality"));
			ShadowQuality->SetDataDisplayName(FText::FromString(TEXT("Shadow Quality")));
			ShadowQuality->SetDescriptionRichText(GET_DESCRIPTION("ShadowQualityDescKey"));
			ShadowQuality->AddIntegerOption(0,FText::FromString(TEXT("Low")));
			ShadowQuality->AddIntegerOption(1,FText::FromString(TEXT("Medium")));
			ShadowQuality->AddIntegerOption(2,FText::FromString(TEXT("High")));
			ShadowQuality->AddIntegerOption(3,FText::FromString(TEXT("Epic")));
			ShadowQuality->AddIntegerOption(4,FText::FromString(TEXT("Cinematic")));
			ShadowQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetShadowQuality));
			ShadowQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetShadowQuality));
			ShadowQuality->SetShouldApplySettingsImmediately(true);

			ShadowQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(ShadowQuality);

			GraphicsCategoryCollection->AddChildListData(ShadowQuality);
		}

		//AntiAliasing Quality
		{
			UListDataObject_StringInteger* AntiAliasingQuality = NewObject<UListDataObject_StringInteger>();
			AntiAliasingQuality->SetDataID(FName("AntiAliasingQuality"));
			AntiAliasingQuality->SetDataDisplayName(FText::FromString(TEXT("Anti Aliasing")));
			AntiAliasingQuality->SetDescriptionRichText(GET_DESCRIPTION("AntiAliasingDescKey"));
			AntiAliasingQuality->AddIntegerOption(0,FText::FromString(TEXT("Low")));
			AntiAliasingQuality->AddIntegerOption(1,FText::FromString(TEXT("Medium")));
			AntiAliasingQuality->AddIntegerOption(2,FText::FromString(TEXT("High")));
			AntiAliasingQuality->AddIntegerOption(3,FText::FromString(TEXT("Epic")));
			AntiAliasingQuality->AddIntegerOption(4,FText::FromString(TEXT("Cinematic")));
			AntiAliasingQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetAntiAliasingQuality));
			AntiAliasingQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetAntiAliasingQuality));
			AntiAliasingQuality->SetShouldApplySettingsImmediately(true);

			AntiAliasingQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(AntiAliasingQuality);

			GraphicsCategoryCollection->AddChildListData(AntiAliasingQuality);
		}

		//View Distance Quality
		{
			UListDataObject_StringInteger* ViewDistanceQuality = NewObject<UListDataObject_StringInteger>();
			ViewDistanceQuality->SetDataID(FName("ViewDistanceQuality"));
			ViewDistanceQuality->SetDataDisplayName(FText::FromString(TEXT("View Distance")));
			ViewDistanceQuality->SetDescriptionRichText(GET_DESCRIPTION("ViewDistanceDescKey"));
			ViewDistanceQuality->AddIntegerOption(0,FText::FromString(TEXT("Near")));
			ViewDistanceQuality->AddIntegerOption(1,FText::FromString(TEXT("Medium")));
			ViewDistanceQuality->AddIntegerOption(2,FText::FromString(TEXT("Far")));
			ViewDistanceQuality->AddIntegerOption(3,FText::FromString(TEXT("Very Far")));
			ViewDistanceQuality->AddIntegerOption(4,FText::FromString(TEXT("Cinematic")));
			ViewDistanceQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetViewDistanceQuality));
			ViewDistanceQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetViewDistanceQuality));
			ViewDistanceQuality->SetShouldApplySettingsImmediately(true);

			ViewDistanceQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(ViewDistanceQuality);

			GraphicsCategoryCollection->AddChildListData(ViewDistanceQuality);
		}

		//Texture Quality
		{
			UListDataObject_StringInteger* TextureQuality = NewObject<UListDataObject_StringInteger>();
			TextureQuality->SetDataID(FName("TextureQuality"));
			TextureQuality->SetDataDisplayName(FText::FromString(TEXT("Texture Quality")));
			TextureQuality->SetDescriptionRichText(GET_DESCRIPTION("TextureQualityDescKey"));
			TextureQuality->AddIntegerOption(0, FText::FromString(TEXT("Low")));
			TextureQuality->AddIntegerOption(1, FText::FromString(TEXT("Medium")));
			TextureQuality->AddIntegerOption(2, FText::FromString(TEXT("High")));
			TextureQuality->AddIntegerOption(3, FText::FromString(TEXT("Epic")));
			TextureQuality->AddIntegerOption(4, FText::FromString(TEXT("Cinematic")));
			TextureQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetTextureQuality));
			TextureQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetTextureQuality));
			TextureQuality->SetShouldApplySettingsImmediately(true);

			TextureQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(TextureQuality);

			GraphicsCategoryCollection->AddChildListData(TextureQuality);
		}

		//Visual Effects Quality
		{
			UListDataObject_StringInteger* VisualEffectQuality = NewObject<UListDataObject_StringInteger>();
			VisualEffectQuality->SetDataID(FName("VisualEffectQuality"));
			VisualEffectQuality->SetDataDisplayName(FText::FromString(TEXT("Visual Effect Quality")));
			VisualEffectQuality->SetDescriptionRichText(GET_DESCRIPTION("VisualEffectQualityDescKey"));
			VisualEffectQuality->AddIntegerOption(0, FText::FromString(TEXT("Low")));
			VisualEffectQuality->AddIntegerOption(1, FText::FromString(TEXT("Medium")));
			VisualEffectQuality->AddIntegerOption(2, FText::FromString(TEXT("High")));
			VisualEffectQuality->AddIntegerOption(3, FText::FromString(TEXT("Epic")));
			VisualEffectQuality->AddIntegerOption(4, FText::FromString(TEXT("Cinematic")));
			VisualEffectQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetVisualEffectQuality));
			VisualEffectQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetVisualEffectQuality));
			VisualEffectQuality->SetShouldApplySettingsImmediately(true);

			VisualEffectQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(VisualEffectQuality);

			GraphicsCategoryCollection->AddChildListData(VisualEffectQuality);
		}

		//Reflection Quality
		{
			UListDataObject_StringInteger* ReflectionQuality = NewObject<UListDataObject_StringInteger>();
			ReflectionQuality->SetDataID(FName("ReflectionQuality"));
			ReflectionQuality->SetDataDisplayName(FText::FromString(TEXT("Reflection Quality")));
			ReflectionQuality->SetDescriptionRichText(GET_DESCRIPTION("ReflectionQualityDescKey"));
			ReflectionQuality->AddIntegerOption(0, FText::FromString(TEXT("Low")));
			ReflectionQuality->AddIntegerOption(1, FText::FromString(TEXT("Medium")));
			ReflectionQuality->AddIntegerOption(2, FText::FromString(TEXT("High")));
			ReflectionQuality->AddIntegerOption(3, FText::FromString(TEXT("Epic")));
			ReflectionQuality->AddIntegerOption(4, FText::FromString(TEXT("Cinematic")));
			ReflectionQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetReflectionQuality));
			ReflectionQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetReflectionQuality));
			ReflectionQuality->SetShouldApplySettingsImmediately(true);

			ReflectionQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(ReflectionQuality);

			GraphicsCategoryCollection->AddChildListData(ReflectionQuality);
		}

		//Post Processing Quality
		{
			UListDataObject_StringInteger* PostProcessingQuality = NewObject<UListDataObject_StringInteger>();
			PostProcessingQuality->SetDataID(FName("PostProcessingQuality"));
			PostProcessingQuality->SetDataDisplayName(FText::FromString(TEXT("Post Processing Quality")));
			PostProcessingQuality->SetDescriptionRichText(GET_DESCRIPTION("PostProcessingQualityDescKey"));
			PostProcessingQuality->AddIntegerOption(0, FText::FromString(TEXT("Low")));
			PostProcessingQuality->AddIntegerOption(1, FText::FromString(TEXT("Medium")));
			PostProcessingQuality->AddIntegerOption(2, FText::FromString(TEXT("High")));
			PostProcessingQuality->AddIntegerOption(3, FText::FromString(TEXT("Epic")));
			PostProcessingQuality->AddIntegerOption(4, FText::FromString(TEXT("Cinematic")));
			PostProcessingQuality->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetPostProcessingQuality));
			PostProcessingQuality->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetPostProcessingQuality));
			PostProcessingQuality->SetShouldApplySettingsImmediately(true);

			PostProcessingQuality->AddEditDependencyData(CreatedOverallQuality);

			CreatedOverallQuality->AddEditDependencyData(PostProcessingQuality);

			GraphicsCategoryCollection->AddChildListData(PostProcessingQuality);
		}
	}

	//Advanced Graphics Category
	{
		UListDataObject_Collection* AdvancedGraphicsCategoryCollection = NewObject<UListDataObject_Collection>();
		AdvancedGraphicsCategoryCollection->SetDataID(FName("AdvancedGraphicsCategoryCollection"));
		AdvancedGraphicsCategoryCollection->SetDataDisplayName(FText::FromString(TEXT("Advanced Graphics")));

		VideoTabCollection->AddChildListData(AdvancedGraphicsCategoryCollection);

		//Vertical Sync
		{
			UListDataObject_StringBool* VerticalSync = NewObject<UListDataObject_StringBool>();
			VerticalSync->SetDataID(FName("VerticalSync"));
			VerticalSync->SetDataDisplayName(FText::FromString(TEXT("V-Sync")));
			VerticalSync->SetDescriptionRichText(GET_DESCRIPTION("VerticalSyncDescKey"));
			VerticalSync->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(IsVSyncEnabled));
			VerticalSync->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetVSyncEnabled));
			VerticalSync->SetFalseAsDefaultValue();
			VerticalSync->SetShouldApplySettingsImmediately(true);

			FOptionsDataEditConditionDescriptor FullscreenOnlyCondition;
			FullscreenOnlyCondition.SetEditConditionFunc(
				[CreatedWindowMode]()->bool
				{
					return CreatedWindowMode->GetCurrentValueAsEnum<EWindowMode::Type>() == EWindowMode::Fullscreen;
				}
			);
			FullscreenOnlyCondition.SetDisabledRichReason(TEXT("\n\n<Disabled>This feature only works if the 'Window Mode' is set to 'Fullscreen'.</>"));
			FullscreenOnlyCondition.SetDisabledForcedStringValue(TEXT("false"));

			// 此处仅注册条件，没有订阅 WindowMode；条件会在界面再次查询可编辑状态时求值。
			VerticalSync->AddEditCondition(FullscreenOnlyCondition);

			AdvancedGraphicsCategoryCollection->AddChildListData(VerticalSync);
		}

		//Frame Rate Limit
		{
			UListDataObject_String* FrameRateLimit = NewObject<UListDataObject_String>();
			FrameRateLimit->SetDataID(FName("FrameRateLimit"));
			FrameRateLimit->SetDataDisplayName(FText::FromString(TEXT("Frame Rate Limit")));
			FrameRateLimit->SetDescriptionRichText(GET_DESCRIPTION("FrameRateLimitDescKey"));
			FrameRateLimit->AddDynamicOption(LexToString(30.f),FText::FromString(TEXT("30 FPS")));
			FrameRateLimit->AddDynamicOption(LexToString(60.f),FText::FromString(TEXT("60 FPS")));
			FrameRateLimit->AddDynamicOption(LexToString(90.f),FText::FromString(TEXT("90 FPS")));
			FrameRateLimit->AddDynamicOption(LexToString(120.f),FText::FromString(TEXT("120 FPS")));
			FrameRateLimit->AddDynamicOption(LexToString(0.f),FText::FromString(TEXT("No Limit")));
			FrameRateLimit->SetDefaultValueFromString(LexToString(0.f));
			FrameRateLimit->SetDataDynamicGetter(MAKE_OPTIONS_DATA_CONTROL(GetFrameRateLimit));
			FrameRateLimit->SetDataDynamicSetter(MAKE_OPTIONS_DATA_CONTROL(SetFrameRateLimit));
			FrameRateLimit->SetShouldApplySettingsImmediately(true);

			AdvancedGraphicsCategoryCollection->AddChildListData(FrameRateLimit);
		}
	}

	RegisteredOptionsTabCollections.Add(VideoTabCollection);
}

// Control 页从当前本地玩家的全部已保存 profiles 动态生成按键项，再按键鼠/手柄分类。
// 遍历 profile → 映射行 → 各 slot 的映射，每项保留自己的读取配置与键位槽。
void UOptionsDataRegistry::InitControlCollectionTab(ULocalPlayer* InOwningLocalPlayer)
{
	UListDataObject_Collection* ControlTabCollection = NewObject<UListDataObject_Collection>();
	ControlTabCollection->SetDataID(FName("ControlTabCollection"));
	ControlTabCollection->SetDataDisplayName(FText::FromString(TEXT("Control")));

	// 从页面所属的 LocalPlayer 获取子系统及 UserSettings，键位数据因而属于这个玩家。
	UEnhancedInputLocalPlayerSubsystem* EISubsystem = InOwningLocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	check(EISubsystem);

	UEnhancedInputUserSettings* EIUserSettings = EISubsystem->GetUserSettings();

	check(EIUserSettings);

	//Keyboard Mouse Category
	{
		UListDataObject_Collection* KeyboardMouseCategoryCollection = NewObject<UListDataObject_Collection>();
		KeyboardMouseCategoryCollection->SetDataID(FName("KeyboardMouseCategoryCollection"));
		KeyboardMouseCategoryCollection->SetDataDisplayName(FText::FromString(TEXT("Keyboard & Mouse")));

		ControlTabCollection->AddChildListData(KeyboardMouseCategoryCollection);

		//Keyboard mouse inputs
		{	
			// 这里用 S 作为键盘类型样本；bMatchBasicKeyTypes 按基本类型过滤，并非只显示 S 键。
			FPlayerMappableKeyQueryOptions KeyboardMouseOnly;
			KeyboardMouseOnly.KeyToMatch = EKeys::S;
			KeyboardMouseOnly.bMatchBasicKeyTypes = true;

			/*FPlayerMappableKeyQueryOptions GamepadOnly;
			GamepadOnly.KeyToMatch = EKeys::Gamepad_FaceButton_Bottom;
			GamepadOnly.bMatchBasicKeyTypes = true;*/

			for (const TPair<FGameplayTag, UEnhancedPlayerMappableKeyProfile*>& ProfilePair : EIUserSettings->GetAllSavedKeyProfiles())
			{
				UEnhancedPlayerMappableKeyProfile* MappableKeyProfile = ProfilePair.Value;

				check(MappableKeyProfile);

				// 一行对应一个映射名称，行内 Mappings 可包含不同 slot；通过设备过滤的每条映射各生成一项。
				for (const TPair<FName, FKeyMappingRow>& MappingRowPair : MappableKeyProfile->GetPlayerMappingRows())
				{
					for (const FPlayerKeyMapping& KeyMapping : MappingRowPair.Value.Mappings)
					{	
						if (MappableKeyProfile->DoesMappingPassQueryOptions(KeyMapping, KeyboardMouseOnly))
						{
							/*Debug::Print(
								TEXT(" Mapping ID: ") + KeyMapping.GetMappingName().ToString() +
								TEXT(" Display Name: ") + KeyMapping.GetDisplayName().ToString() +
								TEXT(" Bound Key: ") + KeyMapping.GetCurrentKey().GetDisplayName().ToString()
							);*/

							UListDataObject_KeyRemap* KeyRemapDataObject = NewObject<UListDataObject_KeyRemap>();
							KeyRemapDataObject->SetDataID(KeyMapping.GetMappingName());
							KeyRemapDataObject->SetDataDisplayName(KeyMapping.GetDisplayName());
							KeyRemapDataObject->InitKeyRemapData(EIUserSettings,MappableKeyProfile,ECommonInputType::MouseAndKeyboard,KeyMapping);

							KeyboardMouseCategoryCollection->AddChildListData(KeyRemapDataObject);
						}		
					}
				}
			}
		}
	}

	//Gamepad Category
	{
		UListDataObject_Collection* GamepadCategoryCollection = NewObject<UListDataObject_Collection>();
		GamepadCategoryCollection->SetDataID(FName("GamepadCategoryCollection"));
		GamepadCategoryCollection->SetDataDisplayName(FText::FromString(TEXT("Gamepad")));

		ControlTabCollection->AddChildListData(GamepadCategoryCollection);

		//Gamepad inputs
		{	
			FPlayerMappableKeyQueryOptions GamepadOnly;
			GamepadOnly.KeyToMatch = EKeys::Gamepad_FaceButton_Bottom;
			GamepadOnly.bMatchBasicKeyTypes = true;

			for (const TPair<FGameplayTag, UEnhancedPlayerMappableKeyProfile*>& ProfilePair : EIUserSettings->GetAllSavedKeyProfiles())
			{
				UEnhancedPlayerMappableKeyProfile* MappableKeyProfile = ProfilePair.Value;

				check(MappableKeyProfile);

				for (const TPair<FName, FKeyMappingRow>& MappingRowPair : MappableKeyProfile->GetPlayerMappingRows())
				{
					for (const FPlayerKeyMapping& KeyMapping : MappingRowPair.Value.Mappings)
					{	
						if (MappableKeyProfile->DoesMappingPassQueryOptions(KeyMapping, GamepadOnly))
						{
							/*Debug::Print(
								TEXT(" Mapping ID: ") + KeyMapping.GetMappingName().ToString() +
								TEXT(" Display Name: ") + KeyMapping.GetDisplayName().ToString() +
								TEXT(" Bound Key: ") + KeyMapping.GetCurrentKey().GetDisplayName().ToString()
							);*/

							UListDataObject_KeyRemap* KeyRemapDataObject = NewObject<UListDataObject_KeyRemap>();
							KeyRemapDataObject->SetDataID(KeyMapping.GetMappingName());
							KeyRemapDataObject->SetDataDisplayName(KeyMapping.GetDisplayName());
							KeyRemapDataObject->InitKeyRemapData(EIUserSettings,MappableKeyProfile,ECommonInputType::Gamepad,KeyMapping);

							GamepadCategoryCollection->AddChildListData(KeyRemapDataObject);
						}		
					}
				}
			}
		}
	}

	RegisteredOptionsTabCollections.Add(ControlTabCollection);
}
