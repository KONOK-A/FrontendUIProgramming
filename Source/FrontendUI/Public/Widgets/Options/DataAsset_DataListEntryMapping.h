// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DataAsset_DataListEntryMapping.generated.h"

class UListDataObject_Base;
class UWidget_ListEntry_Base;
/**
 * 在编辑器中配置“数据对象类型 → 列表行控件类型”，让数据与具体行蓝图分开。
 * FrontendCommonListView 收到一个数据对象后查询此资产，得到行控件的类，再由列表生成或复用控件实例。
 * 键是数据类，值是行控件类；例如给 String 配置一行后，其 Bool、Enum 派生类也能沿继承链复用。
 */
UCLASS()
class FRONTENDUI_API UDataAsset_DataListEntryMapping : public UDataAsset
{
	GENERATED_BODY()
	
public:
	/**
	 * 从数据对象的实际类型向父类逐级查找行控件类，首个存在的映射即终止查找。
	 * 只选择类，不创建控件；若首个映射值为空，也不会继续尝试父类映射。
	 * @param InDataObject 非空的设置数据对象，通过 check 校验。
	 * @return 最近匹配项的控件类；没有匹配或匹配值为空时返回空类，ListView 随后采用默认行类。
	 */
	TSubclassOf<UWidget_ListEntry_Base> FindEntryWidgetClassByDataObject(UListDataObject_Base* InDataObject) const;

private:
	/**
	 * 编辑器中配置的数据类到行控件类的映射；派生数据可沿继承链复用父类配置。
	 */
	UPROPERTY(EditDefaultsOnly)
	TMap< TSubclassOf<UListDataObject_Base>, TSubclassOf<UWidget_ListEntry_Base> > DataObjectListEntryMap;
};
