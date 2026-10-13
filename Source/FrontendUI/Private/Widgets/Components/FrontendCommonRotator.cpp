// Vince Petrelli All Rights Reserved


#include "Widgets/Components/FrontendCommonRotator.h"
#include "CommonTextBlock.h"

void UFrontendCommonRotator::SetSelectedOptionByText(const FText& InTextOption)
{
	// 调用方持有的是显示文本，CommonRotator 选择选项用的是索引，因此先在候选标签中做转换。
	// lambda 是本次搜索的比较规则；用 FText::EqualTo 比较文本，不把显示文本当作保存设置的内部键。
	const int32 FoundIndex = TextLabels.IndexOfByPredicate(
		[InTextOption](const FText& TextItem)->bool
		{
			return TextItem.EqualTo(InTextOption);
		}
	);

	// 匹配成功时使用父类的选中流程。上游 String 行会检查 bUserInitiated，区分程序刷新与用户操作。
	if (FoundIndex != INDEX_NONE)
	{
		SetSelectedItem(FoundIndex);
	}
	else
	{
		// 文本不在预设列表时仅更新显示，不改变当前选项索引。
		MyText->SetText(InTextOption);
	}
}
