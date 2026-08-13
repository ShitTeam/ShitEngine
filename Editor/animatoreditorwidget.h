#ifndef ANIMATOREDITORWIDGET_H
#define ANIMATOREDITORWIDGET_H

#include <QWidget>

namespace Shit { class AnimationComponent; }

/// 帧动画剪辑编辑入口（P29，对齐方案 A）：检查器内不再内嵌 AnimationComponent 的多剪辑表单，
/// 只保留一个「打开 Animation 窗口」按钮（Unity 的 Open Animation 同款），
/// 点击经 openEditorRequested() 信号由 mainwindow 显示并聚焦 AnimationDock。
/// 帧动画剪辑统一在独立 Animation 窗口（.anim 资产）中制作/编辑。
class AnimatorEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AnimatorEditorWidget(Shit::AnimationComponent *comp, QWidget *parent = nullptr);

    /// 保留刷新入口（检查器每帧调用；本控件无需回读，为空操作）
    void refresh();

signals:
    /// 用户点击「打开 Animation 窗口」，请求外部显示 AnimationDock
    void openEditorRequested();

private:
    Shit::AnimationComponent *m_comp;
};

#endif // ANIMATOREDITORWIDGET_H
