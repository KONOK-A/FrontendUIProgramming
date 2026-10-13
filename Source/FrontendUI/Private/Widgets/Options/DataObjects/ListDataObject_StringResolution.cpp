// Vince Petrelli All Rights Reserved


#include "Widgets/Options/DataObjects/ListDataObject_StringResolution.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/Options/OptionsDataInteractionHelper.h"
#include "FontendSettings/FrontendGameUserSettings.h"

#include "FrontendDebugHelper.h"

// Registry 在绑定 getter/setter、加入集合之前调用，先把平台尺寸转换为 String 可使用的候选对。
// 候选的原始值是 (X=1920,Y=1080)，显示文本则是 1920 x 1080，两者分别用于读写和展示。
void UListDataObject_StringResolution::InitResolutionValues()
{	
	TArray<FIntPoint> AvaialbleResolutions;

	UKismetSystemLibrary::GetSupportedFullscreenResolutions(AvaialbleResolutions);

	// 按宽高的平方和排序，最后一项作为这里的“最大分辨率”和恢复默认值。
	AvaialbleResolutions.Sort(
		[](const FIntPoint& A, const FIntPoint& B)->bool
		{
			return A.SizeSquared() < B.SizeSquared();
		}
	);

	for (const FIntPoint& Resolution : AvaialbleResolutions)
	{	
		AddDynamicOption(ResToValueString(Resolution),ResToDisplayText(Resolution));
	}

	MaximumAllowedResolution = ResToValueString(AvaialbleResolutions.Last());

	SetDefaultValueFromString(MaximumAllowedResolution);
}

void UListDataObject_StringResolution::OnDataObjectInitialized()
{
	// 先沿用 String 的“候选/默认值/getter”读取过程，再处理此类特有的候选外尺寸显示。
	Super::OnDataObjectInitialized();

	// 已保存尺寸不在本次候选中时，仍可显示设置中的实际尺寸。
	if (!TrySetDisplayTextFromStringValue(CurrentStringValue))
	{
		CurrentDisplayText = ResToDisplayText(UFrontendGameUserSettings::Get()->GetScreenResolution());
	}
}

FString UListDataObject_StringResolution::ResToValueString(const FIntPoint& InResolution) const
{	
	// 供反射读写的格式必须是 FIntPoint 字符串；界面另用“宽 x 高”显示。
	//Resolution Value from Dynamic Getter: (X=1920,Y=1080)
	return FString::Printf(TEXT("(X=%i,Y=%i)"),InResolution.X,InResolution.Y);
}

FText UListDataObject_StringResolution::ResToDisplayText(const FIntPoint& InResolution) const
{	
	const FString DisplayString = FString::Printf(TEXT("%i x %i"),InResolution.X,InResolution.Y);

	return FText::FromString(DisplayString);
}
