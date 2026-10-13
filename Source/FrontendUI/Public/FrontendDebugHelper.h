#pragma once

namespace Debug
{
	/**
	 * 向屏幕和 LogTemp 输出同一条调试消息；GEngine 不存在时两种输出都跳过。
	 * @param Msg - 屏幕显示及 Warning 级别日志使用的文字。
	 * @param InKey - -1 添加新屏幕消息；相同非负键更新已有消息，显示时间为 7 秒。
	 * @param InColor - 屏幕文字颜色，不影响日志内容或级别。
	 */
	static void Print(const FString& Msg, int32 InKey = -1, const FColor& InColor = FColor::MakeRandomColor())
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(InKey,7.f,InColor,Msg);

			UE_LOG(LogTemp,Warning,TEXT("%s"),*Msg);
		}
	}
}