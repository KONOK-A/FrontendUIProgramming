// Vince Petrelli All Rights Reserved


#include "Widgets/Options/ListEntries/Widget_ListEntry_KeyRemap.h"
#include "Widgets/Options/DataObjects/ListDataObject_KeyRemap.h"
#include "Widgets/Components/FrontendCommonButtonBase.h"
#include "Subsytems/FrontendUISubsystem.h"
#include "FrontendGameplayTags.h"
#include "FrontendFunctionLibrary.h"
#include "Widgets/Options/Widget_KeyRemapScreen.h"

#include "FrontendDebugHelper.h"

void UWidget_ListEntry_KeyRemap::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 固定按钮事件只在实例初始化时绑定；点击时操作的是当前分配给此行的缓存数据。
	CommonButton_RemapKey->OnClicked().AddUObject(this,&ThisClass::OnRemapKeyButtonClicked);
	CommonButton_ResetKeyBinding->OnClicked().AddUObject(this,&ThisClass::OnResetKeyBindingButtonClicked);
}

void UWidget_ListEntry_KeyRemap::OnOwningListDataObjectSet(UListDataObject_Base* InOwningListDataObject)
{
	Super::OnOwningListDataObjectSet(InOwningListDataObject);

	// ListView 分配/复用此行时，先由 Base 连接通知，再取得 KeyRemap 专属数据并显示当前按键图标。
	CachedOwningKeyRemapDataObject = CastChecked<UListDataObject_KeyRemap>(InOwningListDataObject);

	CommonButton_RemapKey->SetButtionDisplayImage(CachedOwningKeyRemapDataObject->GetIconFromCurrentKey());
}

// 数据对象发出修改通知后重新查询图标：重映射与恢复默认共用这条显示刷新路径。
void UWidget_ListEntry_KeyRemap::OnOwningListDataObjectModified(UListDataObject_Base* OwningModifiedData, EOptionsListDataModifyReason ModifyReason)
{
	if (CachedOwningKeyRemapDataObject)
	{
		CommonButton_RemapKey->SetButtionDisplayImage(CachedOwningKeyRemapDataObject->GetIconFromCurrentKey());
	}
}

void UWidget_ListEntry_KeyRemap::OnRemapKeyButtonClicked()
{	
	SelectThisEntryWidget();

	// 第一步：用 GameplayTag 查询捕获弹窗的软类引用，再请求 UI 子系统异步加载并推入 Modal 层。
	// 软引用允许页面类按需加载；当前函数发出请求后，准备工作在后面的状态回调中继续。
	UFrontendUISubsystem::Get(this)->PushSoftWidgetToStackAsync(
		FrontendGameplayTags::Frontend_WidgetStack_Modal,
		UFrontendFunctionLibrary::GetFrontendSoftWidgetClassByTag(FrontendGameplayTags::Frontend_Widget_KeyRemapScreen),
		[this](EAsyncPushWidgetState PushState, UWidget_ActivatableBase* PushedWidget)
		{
			// 第二步：只在“已创建、尚未入栈”阶段配置弹窗，确保激活时已经知道要捕获的设备。
			// 绑定的两个结果函数构成返回通道；捕获本身由弹窗内的 Slate 预处理器完成。
			if (PushState == EAsyncPushWidgetState::OnCreatedBeforePush)
			{
				UWidget_KeyRemapScreen* CreatedKeyRemapScreen = CastChecked<UWidget_KeyRemapScreen>(PushedWidget);
				CreatedKeyRemapScreen->OnKeyRemapScreenKeyPressed.BindUObject(this,&ThisClass::OnKeyToRemapPressed);
				CreatedKeyRemapScreen->OnKeyRemapScreenKeySelectCanceled.BindUObject(this,&ThisClass::OnKeyRemapCanceled);

				if (CachedOwningKeyRemapDataObject)
				{
					CreatedKeyRemapScreen->SetDesiredInputTypeToFilter(CachedOwningKeyRemapDataObject->GetDesiredInputKeyType());
				}
			}
		}
	);
}

void UWidget_ListEntry_KeyRemap::OnResetKeyBindingButtonClicked()
{	
	SelectThisEntryWidget();

	if (!CachedOwningKeyRemapDataObject)
	{
		return;
	}

	// 先判断当前映射是否偏离默认值；已经是默认值时仅显示说明，不再进行恢复请求。
	if (!CachedOwningKeyRemapDataObject->CanResetBackToDefaultValue())
	{
		UFrontendUISubsystem::Get(this)->PushConfirmScreenToModalStackAynsc(
			EConfirmScreenType::Ok,
			FText::FromString(TEXT("Reset Key Mapping")),
			FText::FromString(TEXT("The key binding for ") + CachedOwningKeyRemapDataObject->GetDataDisplayName().ToString() + TEXT(" is already set to default.")),
			[](EConfirmScreenButtonType ClickedButton){}
		);

		return;
	}

	// 需要恢复时先请求确认，只有回调收到 Confirmed 才交给数据对象恢复并保存。
	// 行控件不直接改按钮图标；数据修改广播会使 OnOwningListDataObjectModified 刷新它。
	UFrontendUISubsystem::Get(this)->PushConfirmScreenToModalStackAynsc(
		EConfirmScreenType::YesNo,
		FText::FromString(TEXT("Reset Key Mapping")),
		FText::FromString(TEXT("Are you sure you want to reset the key binding for ") + CachedOwningKeyRemapDataObject->GetDataDisplayName().ToString() + TEXT(" ?")),
		[this](EConfirmScreenButtonType ClickedButton)
		{
			if (ClickedButton == EConfirmScreenButtonType::Confirmed)
			{
				CachedOwningKeyRemapDataObject->TryResetBackToDefaultValue();
			}
		}
	);

}

void UWidget_ListEntry_KeyRemap::OnKeyToRemapPressed(const FKey& PressedKey)
{
	if (CachedOwningKeyRemapDataObject)
	{
		// 第三步：捕获弹窗延后一 tick 回传有效键后，请求数据对象修改 Enhanced Input 用户映射。
		// 数据对象负责保存和广播；当前行更新图标，设置页的订阅同时更新 Reset 是否可用。
		CachedOwningKeyRemapDataObject->BindNewInputKey(PressedKey);
	}
}

// 捕获弹窗回传取消原因时请求一个 OK 提示；这条路径不调用 BindNewInputKey。
void UWidget_ListEntry_KeyRemap::OnKeyRemapCanceled(const FString& CanceledReason)
{
	UFrontendUISubsystem::Get(this)->PushConfirmScreenToModalStackAynsc(
		EConfirmScreenType::Ok,
		FText::FromString(TEXT("Key Remap")),
		FText::FromString(CanceledReason),
		[](EConfirmScreenButtonType ClickedButton){}
	);
}
