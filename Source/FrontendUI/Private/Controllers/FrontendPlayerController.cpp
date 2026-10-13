// Vince Petrelli All Rights Reserved


#include "Controllers/FrontendPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraActor.h"
#include "FontendSettings/FrontendGameUserSettings.h"

void AFrontendPlayerController::OnPossess(APawn* aPawn)
{
	Super::OnPossess(aPawn);

	// 阶段 1：在当前世界查找 Actor Tag 为 Default 的 CameraActor，作为前端场景的固定视角。
	// 这里的 FName Actor Tag 与 UI 路由使用的 FGameplayTag 不是同一种标签机制。
	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClassWithTag(this,ACameraActor::StaticClass(),FName("Default"),FoundCameras);

	// SetViewTarget 改变玩家看向的对象，不改变刚接管的 Pawn；没找到摄像机就不切换。
	if (!FoundCameras.IsEmpty())
	{
		SetViewTarget(FoundCameras[0]);
	}

	UFrontendGameUserSettings* GameUserSettings = UFrontendGameUserSettings::Get();

	// 阶段 2：读取 GameUserSettings 的硬件基准记录；任一结果为 -1 时评测并应用推荐画质。
	// 这是基于已存结果决定是否评测，不是每次 OnPossess 都强制覆盖玩家当前画质选择。
	if (GameUserSettings->GetLastCPUBenchmarkResult() == -1.f || GameUserSettings->GetLastGPUBenchmarkResult() == -1.f)
	{
		GameUserSettings->RunHardwareBenchmark();
		GameUserSettings->ApplyHardwareBenchmarkResults();
	}
}
