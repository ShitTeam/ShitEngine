#include "animatorwidget.h"

#include <ShitEngine/Animation/Animator.h>

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

AnimatorWidget::AnimatorWidget(Shit::Animator *animator, QWidget *parent)
    : QWidget(parent)
    , m_animator(animator)
{
    auto *hint = new QLabel(tr("状态机使用可视化图编辑（状态方块 / 转换箭头 / 参数驱动）。"), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#9aa7b4;");

    auto *openBtn = new QPushButton(tr("打开 Animator 窗口"), this);
    openBtn->setToolTip(tr("在独立窗口中以图形方式编辑状态机（Unity 风格）"));
    connect(openBtn, &QPushButton::clicked, this, &AnimatorWidget::openEditorRequested);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(hint);
    layout->addWidget(openBtn);
    layout->addStretch();
}

void AnimatorWidget::refresh()
{
    // 入口控件无任何状态需要回读；动画数据由 AnimatorDock 每帧刷新。
    Q_UNUSED(m_animator)
}
