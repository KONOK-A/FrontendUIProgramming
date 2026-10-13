// Vince Petrelli All Rights Reserved


#include "Widgets/Options/DataAsset_DataListEntryMapping.h"
#include "Widgets/Options/DataObjects/ListDataObject_Base.h"

// ListView 传入当前数据对象，返回的是控件“类”而非已创建的控件；实例生成仍由 ListView 处理。
TSubclassOf<UWidget_ListEntry_Base> UDataAsset_DataListEntryMapping::FindEntryWidgetClassByDataObject(UListDataObject_Base* InDataObject) const
{	
	check(InDataObject);

	// 先找实际类型，再沿继承链向上找；Bool、Enum 等可复用 String 的行控件映射。
	for (UClass* DataObjectClass = InDataObject->GetClass(); DataObjectClass; DataObjectClass = DataObjectClass->GetSuperClass())
	{
		if (TSubclassOf<UListDataObject_Base> ConvertedDataObjectClass = TSubclassOf<UListDataObject_Base>(DataObjectClass))
		{
			if (DataObjectListEntryMap.Contains(ConvertedDataObjectClass))
			{
				return DataObjectListEntryMap.FindRef(ConvertedDataObjectClass);
			}
		}
	}

	// 无匹配时返回空类，由调用方 ListView 使用默认行控件。
	return TSubclassOf<UWidget_ListEntry_Base>();
}
