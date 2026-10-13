// Vince Petrelli All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OptionsDataRegistry.generated.h"

class UListDataObject_Base;
class UListDataObject_Collection;
/**
 * 构建四个设置页的数据树，并按页签 ID 展开为 ListView 使用的列表。
 * OptionsScreen 首次需要数据时创建本对象；它只组装数据，不创建按钮、滑条或实际行控件。
 * 普通设置在这里配置候选、默认值与反射读写函数；Control 则根据本地玩家的键位配置动态生成。
 */
UCLASS()
class FRONTENDUI_API UOptionsDataRegistry : public UObject
{
	GENERATED_BODY()
	
public:
	/**
	 * 按 Gameplay、Audio、Video、Control 顺序构建并注册页签及其子项。
	 * OptionsScreen 创建本对象后调用一次；重复调用会追加重复页签，不会清空旧数据。
	 * @param InOwningLocalPlayer 非空的所属本地玩家；须具有可用的 Enhanced Input 子系统和 UserSettings。
	 */
	void InitOptionsDataRegistry(ULocalPlayer* InOwningLocalPlayer);

	const TArray<UListDataObject_Collection*>& GetRegisteredOptionsTabCollections() const { return RegisteredOptionsTabCollections;}

	/**
	 * 按页签 DataID 查找集合，并按深度优先、父项先于后代的顺序展开列表源。
	 * 分类标题与叶子项均保留；不包含页签根集合，遍历时跳过空子项。
	 * @param InSelectedTabID 已注册页签的 DataID；不存在时触发 checkf，不是空数组回退。
	 * @return 按树中添加顺序展开的子项数组，供 OptionsScreen 调用 SetListItems。
	 */
	TArray<UListDataObject_Base*> GetListSourceItemsBySelectedTabID(const FName& InSelectedTabID) const;

private:
	/**
	 * 把父节点的后代按深度优先顺序追加到同一数组，不加入父节点自身，也不清空已有结果。
	 * 调用方须提供无环数据树；函数不去重，不检测循环关系。
	 * @param InParentData 待展开父节点；为空或没有子项时保持输出不变。
	 * @param OutFoundChildListData 接收非空后代的输出数组，分类先于其内部设置项。
	 */
	void FindChildListDataRecursively(UListDataObject_Base* InParentData,TArray<UListDataObject_Base*>& OutFoundChildListData) const;

	/**
	 * 创建难度选项与图片说明演示项，配置后追加 Gameplay 页签。
	 */
	void InitGameplayCollectionTab();
	/**
	 * 创建音量滑条及声音开关，并追加 Audio 页签；音量显示区间为 0..1，输出区间为 0..2。
	 */
	void InitAudioCollectionTab();
	/**
	 * 创建显示、画质和高级图形项，注册窗口模式条件及总体/单项画质依赖后追加 Video 页签。
	 * 分辨率候选初始化要求平台返回至少一个支持的全屏尺寸。
	 */
	void InitVideoCollectionTab();
	/**
	 * 遍历玩家全部已保存 profile 的映射及 slot，按键鼠/手柄过滤并生成 Control 页。
	 * 数据项保存源 profile 用于读取；映射名作为 DataID，不在此跨 profile 或 slot 去重。
	 * @param InOwningLocalPlayer 非空本地玩家；其 Enhanced Input 子系统、UserSettings 和遍历到的 profile 均须有效。
	 */
	void InitControlCollectionTab(ULocalPlayer* InOwningLocalPlayer);

	/**
	 * 按注册顺序保留页签根节点；Transient 不保存到磁盘，UPROPERTY 引用维持数据树的 GC 可达性。
	 */
	UPROPERTY(Transient)
	TArray<UListDataObject_Collection*> RegisteredOptionsTabCollections;
};
