#ifndef ANIMATORWIDGET_H
#define ANIMATORWIDGET_H

#include <QWidget>

namespace Shit { class Animator; }

/// 状态机编辑入口（方案 A）：检查器内不再内嵌完整状态机编辑 UI，
/// 只保留一个"打开 Animator 窗口"按钮（Unity Inspector 的 Open Animator 同款），
/// 点击后经 openEditorRequested() 信号由 mainwindow 显示并聚焦 AnimatorDock。
/// 所有状态机编辑统一收敛到独立 Dock（含可视化状态机图）。
class AnimatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AnimatorWidget(Shit::Animator *animator, QWidget *parent = nullptr);

    /// 保留刷新入口（检查器每帧调用；本控件无需回读任何字段，为空操作）
    void refresh();

signals:
    /// 用户点击"打开 Animator 窗口"，请求外部显示 AnimatorDock
    void openEditorRequested();

private:
    Shit::Animator *m_animator;
};

#endif // ANIMATORWIDGET_H
