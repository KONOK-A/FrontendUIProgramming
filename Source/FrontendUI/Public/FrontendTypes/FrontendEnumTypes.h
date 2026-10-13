#pragma once

/**
 * 请求确认框使用的按钮组合，由 UI 子系统生成对应展示数据。
 * 这描述弹窗类型；点击后的业务选择由 EConfirmScreenButtonType 表示。
 */
UENUM(BlueprintType)
enum class EConfirmScreenType : uint8
{
	/** 单个 OK 通知按钮，点击返回 Closed。 */
	Ok,
	/** Yes/No 决策按钮，分别返回 Confirmed/Cancelled。 */
	YesNo,
	/** Ok/Cancel 决策按钮，分别返回 Confirmed/Cancelled。 */
	OKCancel,
	/** 非有效请求类型；当前子系统不为其创建信息对象，随后检查失败。 */
	Unkown UMETA(Hidden)
};

/** 确认框点击结果；回调接收者据此决定后续业务，不依赖按钮显示文字判断操作。 */
UENUM(BlueprintType)
enum class EConfirmScreenButtonType : uint8
{
	/** 用户选择肯定动作，如 Yes 或 Ok。 */
	Confirmed,
	/** 用户选择否定动作，如 No 或 Cancel。 */
	Cancelled,
	/** 用户关闭单按钮通知，不表示同意业务操作。 */
	Closed,
	/** 按钮描述尚未赋予结果时使用的占位值。 */
	Unknown UMETA(Hidden)
};

/** 设置数据修改通知的来源，用于依赖传播及界面刷新；不代表异步任务状态。 */
UENUM(BlueprintType)
enum class EOptionsListDataModifyReason : uint8
{
	/** 当前设置项被直接修改，例如用户点击候选或拖动滑条。 */
	DirectlyModified,
	/** 因关联设置的回读或编辑条件强制值引起的修改。 */
	DependencyModified,
	/** 数据对象执行恢复默认流程时发出的修改。 */
	ResetToDefault
};