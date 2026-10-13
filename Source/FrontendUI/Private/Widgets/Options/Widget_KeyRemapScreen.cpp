// Vince Petrelli All Rights Reserved


#include "Widgets/Options/Widget_KeyRemapScreen.h"
#include "CommonRichTextBlock.h"
#include "Framework/Application/IInputProcessor.h"
#include "CommonInputSubsystem.h"
#include "ICommonInputModule.h"
#include "CommonUITypes.h"

#include "FrontendDebugHelper.h"

/**
 * 在 Slate 把输入交给普通控件前，为 KeyRemapScreen 捕获按键和鼠标按钮按下事件。
 * 通过单播委托同步报告通过筛选的键或取消原因；按下事件均被消费，不直接修改玩家映射。
 * 普通 C++ 对象由页面与 Slate 的共享引用管理，捕获范围不包含滚轮或模拟轴。
 */
class FKeyRemapScreenInputPreprocessor : public IInputProcessor
{
public:
	/**
	 * 保存本轮捕获配置；构造本身不向 Slate 注册，页面激活后负责注册与绑定结果委托。
	 * @param InInputTypeToListenTo 目标设备；键鼠和手柄有专用筛选，其余类型不额外过滤。
	 * @param InOwningLocalPlayer 用于查询 CommonInputSubsystem 的本地玩家；处理非 Escape 键时须仍有效。
	 */
	FKeyRemapScreenInputPreprocessor(ECommonInputType InInputTypeToListenTo,ULocalPlayer* InOwningLocalPlayer)
	: CachedInputTypeToListenTo(InInputTypeToListenTo)
	, CachedWeakOwningLocalPlayer(InOwningLocalPlayer)
	{ }

	/** 设备筛选通过时使用的单播按键通知类型。 */
	DECLARE_DELEGATE_OneParam(FOnInputPreProcessorKeyPressedDelegate,const FKey& /*PressedKey*/)
	/** 在 Slate 输入处理过程中同步执行；页面接收后安排延迟报告与停用。 */
	FOnInputPreProcessorKeyPressedDelegate OnInputPreProcessorKeyPressed;

	/** Escape 或设备不匹配时使用的单播取消通知类型。 */
	DECLARE_DELEGATE_OneParam(FOnInputPreProcessorKeySelectCanceledDelegate,const FString& /*CanceledReason*/)
	/** 在输入处理过程中同步回传取消原因；预处理器自身不注销或停用页面。 */
	FOnInputPreProcessorKeySelectCanceledDelegate OnInputPreProcessorKeySelectCanceled;

protected:
	/** 满足 IInputProcessor 的逐帧接口；本预处理器只响应输入事件，不执行逐帧逻辑。 */
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{

	}

	/**
	 * 将键盘或手柄按下事件交给统一筛选入口。
	 * @param SlateApp 派发事件的 Slate 应用；此实现未使用。
	 * @param InKeyEvent 提供本次按下的 FKey，不另行排除重复按下事件。
	 * @return 始终为 true；通过筛选、取消以及未绑定结果委托时都会消费此事件。
	 */
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		ProcessPressedKey(InKeyEvent.GetKey());

		return true;
	}

	/**
	 * 将鼠标按钮按下事件的触发键交给统一筛选入口，包含手柄产生的模拟鼠标左键。
	 * @param SlateApp 派发事件的 Slate 应用；此实现未使用。
	 * @param MouseEvent 通过 GetEffectingButton 提供本次触发按钮的事件。
	 * @return 始终为 true，设备不匹配或未绑定结果委托也不会继续向普通控件派发。
	 */
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		ProcessPressedKey(MouseEvent.GetEffectingButton());

		return true;
	}

	/**
	 * 先以 Escape 取消，再按目标设备筛选，最后同步执行已绑定的结果委托，不另行检查 FKey 有效性。
	 * 键鼠目标拒绝手柄键以及当前输入类型为手柄的事件；手柄目标拒绝非手柄键。
	 * 例外是手柄模式产生的鼠标左键，会转成 CommonUI 默认手柄点击键；其他目标类型直接放行。
	 * 非 Escape 路径要求玩家的 CommonInputSubsystem 有效；模拟点击转换还要求默认点击动作行有效。
	 * @param InPressedKey 按键或鼠标按钮事件提取出的键；每次调用独立处理，不屏蔽已排队结果之后的输入。
	 */
	void ProcessPressedKey(const FKey& InPressedKey)
	{
		// Escape 优先作为取消键处理，不进入设备过滤或按键映射。
		if (InPressedKey == EKeys::Escape)
		{
			OnInputPreProcessorKeySelectCanceled.ExecuteIfBound(TEXT("Key Remap has been canceled"));

			return;
		}

		// CommonInputSubsystem 表示该 LocalPlayer 当前使用的输入方式。
		// FKey 自身的设备分类与当前输入方式一起参与判断，尤其用于识别手柄模拟点击。
		UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(CachedWeakOwningLocalPlayer.Get());

		check(CommonInputSubsystem);

		ECommonInputType CurrentInputType = CommonInputSubsystem->GetCurrentInputType();

		// 按目标设备过滤；不匹配的输入会取消此次捕获并回传原因。
		switch (CachedInputTypeToListenTo)
		{
		case ECommonInputType::MouseAndKeyboard:
			
			if (InPressedKey.IsGamepadKey() || CurrentInputType == ECommonInputType::Gamepad)
			{
				OnInputPreProcessorKeySelectCanceled.ExecuteIfBound(TEXT("Detected Gamepad Key pressed for keyboard inputs. Key Remap has been cancled."));

				return;
			}

			break;

		case ECommonInputType::Gamepad:
			
			// 手柄模式下收到模拟鼠标左键时，转换成 CommonUI 配置的默认手柄点击键。
			if (CurrentInputType == ECommonInputType::Gamepad && InPressedKey == EKeys::LeftMouseButton)
			{	
				FCommonInputActionDataBase* InputActionData = ICommonInputModule::GetSettings().GetDefaultClickAction().GetRow<FCommonInputActionDataBase>(TEXT(""));

				check(InputActionData);

				OnInputPreProcessorKeyPressed.ExecuteIfBound(InputActionData->GetDefaultGamepadInputTypeInfo().GetKey());

				return;
			}

			if (!InPressedKey.IsGamepadKey())
			{
				OnInputPreProcessorKeySelectCanceled.ExecuteIfBound(TEXT("Detected non Gamepad Key pressed for Gamepad inputs. Key Remap has been cancled."));

				return;
			}

			break;
		
		default:
			break;
		}

		// ExecuteIfBound 在已绑定时调用弹窗的 OnValidKeyPressedDetected；尚未在此处保存键位。
		OnInputPreProcessorKeyPressed.ExecuteIfBound(InPressedKey);
	}
private:
	/** 构造时固定的目标设备；页面之后修改配置不会更新这个已注册实例。 */
	ECommonInputType CachedInputTypeToListenTo;
	/** 查询当前输入方式所需的本地玩家弱引用；不延长玩家存活期，处理非 Escape 键时仍要求其子系统有效。 */
	TWeakObjectPtr<ULocalPlayer> CachedWeakOwningLocalPlayer;
};

void UWidget_KeyRemapScreen::SetDesiredInputTypeToFilter(ECommonInputType InDesiredInputType)
{
	CachedDesiredInputType = InDesiredInputType;
}

void UWidget_KeyRemapScreen::NativeOnActivated()
{
	Super::NativeOnActivated();

	// 第一步：每次激活都建立本轮捕获，输入接管跟随激活/停用，而非只在实例初始化时做。
	// BindUObject 把预处理器的成功/取消委托连接到当前弹窗的两个处理函数。
	CachedInputPreprocessor = MakeShared<FKeyRemapScreenInputPreprocessor>(CachedDesiredInputType,GetOwningLocalPlayer());
	CachedInputPreprocessor->OnInputPreProcessorKeyPressed.BindUObject(this,&ThisClass::OnValidKeyPressedDetected);
	CachedInputPreprocessor->OnInputPreProcessorKeySelectCanceled.BindUObject(this,&ThisClass::OnKeySelectCanceled);

	FSlateApplication::Get().RegisterInputPreProcessor(CachedInputPreprocessor,-1);

	// 第二步：用同一个目标设备生成提示；富文本标签交给蓝图控件配置的样式表解释。
	FString InputDeviceName;

	switch (CachedDesiredInputType)
	{
	case ECommonInputType::MouseAndKeyboard:
		InputDeviceName = TEXT("Mouse & Keyboard");
		break;

	case ECommonInputType::Gamepad:
		InputDeviceName = TEXT("Gamepad");
		break;
	
	default:
		break;
	}

	const FString DisplayRichMessage = FString::Printf(
		TEXT("<KeyRemapDefault>Press any</> <KeyRemapHighlight>%s</> <KeyRemapDefault>key.</>"),*InputDeviceName
	);

	CommonRichText_RemapMessage->SetText(FText::FromString(DisplayRichMessage));
}

void UWidget_KeyRemapScreen::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	// 停用时向 Slate 注销，后续输入便不再由这个预处理器接管；随后释放本页面的共享引用。
	// 清掉指针本身不能替代注销：Slate 是否继续调用它，取决于注册关系。
	if (CachedInputPreprocessor)
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(CachedInputPreprocessor);

		CachedInputPreprocessor.Reset();
	}
}

// 由预处理器的成功委托调用；把按键按值带入延迟回调，再交给打开弹窗的列表行。
void UWidget_KeyRemapScreen::OnValidKeyPressedDetected(const FKey& PressedKey)
{
	RequestDeactivateWidget(
		[this,PressedKey]()
		{	
			OnKeyRemapScreenKeyPressed.ExecuteIfBound(PressedKey);
		}
	);
}

// Escape 或设备不匹配会走此入口；取消原因同样延迟回传，随后与成功路径共用停用流程。
void UWidget_KeyRemapScreen::OnKeySelectCanceled(const FString& CanceledReason)
{
	RequestDeactivateWidget(
		[this,CanceledReason]()
		{	
			OnKeyRemapScreenKeySelectCanceled.ExecuteIfBound(CanceledReason);
		}
	);
}

void UWidget_KeyRemapScreen::RequestDeactivateWidget(TFunction<void()> PreDeactivateCallback)
{
	// 此函数是在 Slate 输入事件处理中被请求的，先安排到 CoreTicker 的后续 tick，
	// 避免在当前事件派发过程中立即停用控件、注销正在参与输入处理的预处理器。
	// 执行顺序是“结果回调 → DeactivateWidget → NativeOnDeactivated 注销”，不是先注销再回调。
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda(
			[PreDeactivateCallback,this](float DeltaTime)->bool
			{
				// 成功回调会让列表行提交映射；取消回调会让列表行请求原因提示弹窗。
				PreDeactivateCallback();

				DeactivateWidget();

				// 返回 false 表示这个 ticker 只执行一次。
				return false;
			}
		)
	);
}
