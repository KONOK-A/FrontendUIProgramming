// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Widgets/Options/DataObjects/ListDataObject_Base.h"
#include "ListDataObject_Collection.generated.h"

/**
 * 数据树的容器节点，既可表示整个页签，也可表示页签内的分类标题。
 * 例如 Audio 页集合包含 Volume 分类，Volume 再包含 OverallVolume 等值项。
 * 这里只存直接子项；Registry 递归读取整棵树，再交给 ListView 按顺序生成分类行和设置行。
 */
UCLASS()
class FRONTENDUI_API UListDataObject_Collection : public UListDataObject_Base
{
	GENERATED_BODY()
	
public:
	/**
	 * 初始化子项，再设置其父节点并保留引用；不去重，也不检测循环父子关系。
	 * 用于 Registry 的无环数据树；候选、默认值及 Helper 应在本次调用前配置好。
	 * @param InChildListData 非空子项；初始化期间 ParentData 尚未由本函数设置。
	 */
	void AddChildListData(UListDataObject_Base* InChildListData);

	//~ Begin UListDataObject_Base Interface
	/**
	 * 读取本集合的直接子项，不递归。
	 * @return 按添加顺序排列的数组副本；Registry 负责继续展开后代。
	 */
	virtual TArray<UListDataObject_Base*> GetAllChildListData() const;
	/**
	 * 判断直接子项数组是否非空，不验证数组元素。
	 * @return 数组非空时为 true。
	 */
	virtual bool HasAnyChildListData() const;
	//~ End UListDataObject_Base Interface

private:
	/**
	 * 按添加顺序持有直接子项；Transient 不持久化，UPROPERTY 使引用可被 GC 追踪。
	 */
	UPROPERTY(Transient)
	TArray<UListDataObject_Base*> ChildListDataArray;
};
