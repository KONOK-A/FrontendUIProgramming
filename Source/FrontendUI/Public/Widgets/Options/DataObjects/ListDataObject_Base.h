// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FrontendTypes/FrontendEnumTypes.h"
#include "FrontendTypes/FrontendStructTypes.h"
#include "ListDataObject_Base.generated.h"

// 为这些基础属性生成成对的 Get/Set；宏本身不负责通知或应用设置。
#define LIST_DATA_ACCESSOR(DataType,PropertyName) \
	FORCEINLINE DataType Get##PropertyName() const { return PropertyName;} \
	void Set##PropertyName(DataType In##PropertyName) { PropertyName = In##PropertyName;}

/**
 * 设置数据的公共基类：保存展示信息、编辑条件，并向界面和依赖项发送变化通知。
 * 它是 UObject 数据而非控件：Registry 创建它，OptionsScreen 把它交给 ListView，行控件读取和修改它。
 * 派生类补充不同能力：Collection 存子项，Value 系列读写普通设置，KeyRemap 读写按键映射。
 * 页面只持有 Base 指针也能调用各类的初始化和 Reset：virtual 会根据对象的实际类型选择实现。
 */
UCLASS(Abstract)
class FRONTENDUI_API UListDataObject_Base : public UObject
{
	GENERATED_BODY()
	
public:
	/**
	 * 同步通知数据对象与修改原因的原生多播委托；监听者可在广播期间刷新界面或继续传播依赖。
	 */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnListDataModifiedDelegate,UListDataObject_Base*,EOptionsListDataModifyReason)
	/**
	 * NotifyListDataModified 调用时广播；数据项通常传入自身，行控件刷新内容，页面更新 Reset 状态。
	 * 广播完成后才按立即应用开关执行 ApplySettings；收到通知不代表底层写入已验证成功。
	 */
	FOnListDataModifiedDelegate OnListDataModified;

	/**
	 * 基类处理依赖修改时转发源数据与原原因，供行控件重新查询可编辑状态。
	 * 源数据通常是被订阅项，不是当前对象；派生处理提前返回时不会触发此事件。
	 */
	FOnListDataModifiedDelegate OnDependencyDataModified;

	LIST_DATA_ACCESSOR(FName,DataID)
	LIST_DATA_ACCESSOR(FText,DataDisplayName)
	LIST_DATA_ACCESSOR(FText,DescriptionRichText)
	LIST_DATA_ACCESSOR(FText,DisabledRichText)
	LIST_DATA_ACCESSOR(TSoftObjectPtr<UTexture2D>,SoftDescriptionImage)
	LIST_DATA_ACCESSOR(UListDataObject_Base*,ParentData)

	/**
	 * 执行项目自己的数据初始化入口，转调实际类型的 OnDataObjectInitialized。
	 * Collection 加入子项时调用；应先配置候选、默认值和读写 Helper，调用时父节点尚未由集合设置。
	 * 此入口没有重复初始化保护，不是 UE 自动生命周期回调。
	 */
	void InitDataObject();

	/**
	 * 取得直接子项，递归展开由 Registry 负责。
	 * @return 基类返回空数组；Collection 返回按添加顺序排列的直接子项副本。
	 */
	virtual TArray<UListDataObject_Base*> GetAllChildListData() const { return TArray<UListDataObject_Base*>();}
	/**
	 * 判断是否有可递归展开的直接子项。
	 * @return 基类为 false，Collection 根据其子项数组是否非空返回结果。
	 */
	virtual bool HasAnyChildListData() const { return false;}

	/**
	 * 设置后续修改通知是否立即应用游戏用户设置；本次调用本身不应用设置。
	 * @param bShouldApplyRightAway 为 true 时，每次 NotifyListDataModified 广播结束后调用 ApplySettings(true)。
	 */
	void SetShouldApplySettingsImmediately(bool bShouldApplyRightAway) { bShouldApplyChangeImmediatly = bShouldApplyRightAway;}

	/**
	 * 查询是否配置了恢复默认所需的数据，由值类型或键位类型实现。
	 * @return 基类为 false；派生类分别检查可选默认字符串或有效默认按键。
	 */
	virtual bool HasDefaultValue() const { return false;}
	/**
	 * 查询当前项是否偏离可用默认值，供 OptionsScreen 收集当前页可重置项。
	 * @return 基类为 false；派生类说明自身的比较规则，不保证具备可写 Helper。
	 */
	virtual bool CanResetBackToDefaultValue() const { return false;}
	/**
	 * 由单行或整页 Reset 流程请求恢复默认，派生类负责实际写入与通知。
	 * @return 基类为 false；派生类的 true 表示执行了自身重置路径，不代表外部保存结果已验证。
	 */
	virtual bool TryResetBackToDefaultValue() { return false;}
	
	/**
	 * 追加编辑条件副本；只注册规则，不立即求值，也不自动订阅规则引用的数据。
	 * @param InEditCondition Registry 组装的判断函数、禁用原因与可选强制字符串。
	 */
	void AddEditCondition(const FOptionsDataEditConditionDescriptor& InEditCondition);

	/**
	 * 订阅另一数据项的修改，以触发本项的依赖处理；同一源已绑定当前对象时不重复绑定。
	 * @param InDependencyData 非空的依赖源；该函数不检查空指针，也不创建编辑条件。
	 */
	void AddEditDependencyData(UListDataObject_Base* InDependencyData);

	/**
	 * 按注册顺序求值全部有效条件，并合并失败条件的禁用说明。
	 * 该查询可能调用派生类写入强制值、广播修改并触发立即 Apply；条件恢复后不还原旧值。
	 * 全部条件通过时不清空已缓存的 DisabledRichText，调用方应以返回值决定是否展示它。
	 * @return 没有有效失败条件时为 true；任一有效条件失败即为 false。
	 */
	bool IsDataCurrentlyEditable();

protected:
	/**
	 * InitDataObject 转调的初始化扩展点；基类为空，Collection 无需额外初始化。
	 * 派生类的 override 根据实际类型准备候选、当前缓存及显示文案。
	 */
	virtual void OnDataObjectInitialized();

	/**
	 * 同步广播数据修改，再按立即应用开关调用 ApplySettings(true)。
	 * 不会比较新旧值或验证底层写入；监听者可能在广播期间进一步触发依赖修改。
	 * @param ModifiedData 交给监听者的数据对象，当前派生类均传入 this。
	 * @param ModifyReason 修改来源，默认表示直接修改；重置和联动路径可传入专用原因。
	 */
	virtual void NotifyListDataModified(UListDataObject_Base* ModifiedData,EOptionsListDataModifyReason ModifyReason = EOptionsListDataModifyReason::DirectlyModified);

	/**
	 * 判断失败条件给出的强制值是否需要由本类型处理。
	 * @param InForcedValue 条件中配置的原始字符串。
	 * @return 基类始终为 false；String 仅比较缓存是否不同，不验证候选合法性。
	 */
	virtual bool CanSetToForcedStringValue(const FString& InForcedValue) const { return false;}

	/**
	 * 在条件失败且允许强制写值时执行派生类型的更新；基类不做任何操作。
	 * @param InForcedValue 待写入的原始字符串，解释方式由具体值类型决定。
	 */
	virtual void OnSetToForcedStringValue(const FString& InForcedValue) {}

	/**
	 * 接收订阅源的修改并转发 OnDependencyDataModified；基类不直接求值编辑条件。
	 * 派生类可先回读或刷新自身，需继续转发时调用 Super。
	 * @param ModifiedDependencyData 发出源修改通知的数据对象，通常不是当前对象。
	 * @param ModifyReason 源通知携带的修改原因，基类原样转发。
	 */
	virtual void OnEditDependencyDataModified(UListDataObject_Base* ModifiedDependencyData,EOptionsListDataModifyReason ModifyReason);

private:
	/**
	 * Registry 设置的稳定标识；页签使用它查找集合，不使用显示文案定位。
	 */
	FName DataID;
	/**
	 * 页签、分类或设置行的用户可见标题。
	 */
	FText DataDisplayName;
	/**
	 * 详情区显示的设置说明，可包含富文本标记。
	 */
	FText DescriptionRichText;
	/**
	 * 最近一次条件失败时汇总的富文本原因；只在当前不可编辑时展示。
	 */
	FText DisabledRichText;
	/**
	 * 详情图片的软引用，由详情视图按需加载，不由数据对象主动加载。
	 */
	TSoftObjectPtr<UTexture2D> SoftDescriptionImage;

	/**
	 * 集合完成子项初始化后设置的父节点引用；顶层页签通常没有父节点。
	 */
	UPROPERTY(Transient)
	UListDataObject_Base* ParentData;

	/**
	 * 默认不立即应用；开启后每次本项修改广播结束均调用游戏用户设置的 ApplySettings。
	 */
	bool bShouldApplyChangeImmediatly = false;

	/**
	 * 按添加顺序评估的运行时编辑条件；条件依赖关系需另行通过 AddEditDependencyData 订阅。
	 */
	UPROPERTY(Transient)
	TArray<FOptionsDataEditConditionDescriptor> EditConditionDescArray;
};
