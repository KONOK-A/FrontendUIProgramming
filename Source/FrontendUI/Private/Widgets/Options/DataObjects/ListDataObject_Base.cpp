// Vince Petrelli All Rights Reserved


#include "Widgets/Options/DataObjects/ListDataObject_Base.h"
#include "FontendSettings/FrontendGameUserSettings.h"

void UListDataObject_Base::InitDataObject()
{
	// 这是项目自己的初始化流程，不是 UE 自动回调；Collection 加入子项时明确调用它。
	// 虽然入口在 Base，虚函数仍会执行实际类型的版本，例如 StringBool 先准备 ON/OFF 候选。
	OnDataObjectInitialized();
}

void UListDataObject_Base::AddEditCondition(const FOptionsDataEditConditionDescriptor& InEditCondition)
{
	// 这里只保存规则；要跟随另一项变化，还须单独调用 AddEditDependencyData 订阅。
	EditConditionDescArray.Add(InEditCondition);
}

void UListDataObject_Base::AddEditDependencyData(UListDataObject_Base* InDependencyData)
{
	// 当前项监听依赖项的修改；避免对同一依赖重复绑定当前对象。
	if (!InDependencyData->OnListDataModified.IsBoundToObject(this))
	{
		InDependencyData->OnListDataModified.AddUObject(this,&ThisClass::OnEditDependencyDataModified);
	}
}

// 这个查询可能强制写入设置并触发通知，不只是返回 bool；行控件和详情视图都会调用它。
bool UListDataObject_Base::IsDataCurrentlyEditable()
{
	bool bIsEditable = true;

	if (EditConditionDescArray.IsEmpty())
	{
		return bIsEditable;
	}

	// 评估全部有效条件：任一失败就禁用，并把每个失败原因合并给详情视图显示。
	FString CachedDisabledRichReason;

	for (const FOptionsDataEditConditionDescriptor& Condition : EditConditionDescArray)
	{
		if (!Condition.IsValid() || Condition.IsEditConditionMet())
		{
			continue;
		}

		bIsEditable = false;

		CachedDisabledRichReason.Append(Condition.GetDisabledRichReason());
		
		SetDisabledRichText(FText::FromString(CachedDisabledRichReason));

		// 条件还可以附带强制值，例如无边框模式要求分辨率采用最大候选。
		// 具体写法交给虚函数：String 更新缓存、调用 setter 并通知；Base 默认不支持强制写值。
		if (Condition.HasForcedStringValue())
		{
			const FString ForcedStringValue = Condition.GetDisabledForcedStringValue();

			//If the current value this data object has can be set to the forced value
			if (CanSetToForcedStringValue(ForcedStringValue))
			{
				OnSetToForcedStringValue(ForcedStringValue);
			}
		}
	}

	return bIsEditable;
}

void UListDataObject_Base::OnDataObjectInitialized()
{
}

void UListDataObject_Base::NotifyListDataModified(UListDataObject_Base* ModifiedData, EOptionsListDataModifyReason ModifyReason)
{
	// 先同步广播，让 UI 和依赖项处理变化；广播返回后才按开关调用 ApplySettings。
	// 行控件监听后刷新显示，OptionsScreen 监听后更新 Reset，其他数据项也可监听此通知。
	// ModifyReason 区分直接修改、恢复默认、依赖修改，便于接收者识别来源。
	OnListDataModified.Broadcast(ModifiedData,ModifyReason);

	if (bShouldApplyChangeImmediatly)
	{
		UFrontendGameUserSettings::Get()->ApplySettings(true);
	}
}

void UListDataObject_Base::OnEditDependencyDataModified(UListDataObject_Base* ModifiedDependencyData, EOptionsListDataModifyReason ModifyReason)
{
	// 例如分辨率订阅了 WindowMode：传来的 ModifiedDependencyData 是 WindowMode，而不是分辨率自身。
	// 基类只转发依赖通知，由分辨率行重新查询可编辑状态；这里不主动求条件。
	// Integer/Scalar 的重写会先处理值的回读或刷新，再通过 Super 保留这条通知路径。
	OnDependencyDataModified.Broadcast(ModifiedDependencyData,ModifyReason);
}
