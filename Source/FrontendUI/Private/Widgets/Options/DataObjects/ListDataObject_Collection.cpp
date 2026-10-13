// Vince Petrelli All Rights Reserved


#include "Widgets/Options/DataObjects/ListDataObject_Collection.h"

void UListDataObject_Collection::AddChildListData(UListDataObject_Base* InChildListData)
{	
	// 加入集合会立刻初始化子项，所以候选值、默认值和 getter 必须提前配置。
	// 此时 ParentData 尚未设置；初始化结束后才连接父节点并保存子项引用。
	//Notify the child list data to init itself
	InChildListData->InitDataObject();

	//Set the child list data's parent to this
	InChildListData->SetParentData(this);

	ChildListDataArray.Add(InChildListData);
}

// Base 的默认实现返回空数组；Collection 重写后返回直接子项，Registry 再逐层递归。
TArray<UListDataObject_Base*> UListDataObject_Collection::GetAllChildListData() const
{
	return ChildListDataArray;
}

bool UListDataObject_Collection::HasAnyChildListData() const
{
	return !ChildListDataArray.IsEmpty();
}
