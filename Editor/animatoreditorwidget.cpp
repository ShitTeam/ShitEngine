#include "animatoreditorwidget.h"

#include <ShitEngine/Component/AnimationComponent.h>

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

AnimatorEditorWidget::AnimatorEditorWidget(Shit::AnimationComponent *comp, QWidget *parent)
    : QWidget(parent)
    , m_comp(comp)
{
    auto *hint = new QLabel(tr("帧动画在独立 Animation 窗口中制作（精灵表选帧 + Dope Sheet 时间轴，保存为 .anim 资产）。"), this);
    hint->setWordWrap(true);
    hint->setStyleSheet("color:#9aa7b4;");

    auto *openBtn = new QPushButton(tr("打开 Animation 窗口"), this);
    openBtn->setToolTip(tr("在独立窗口中以时间轴方式制作/编辑帧动画剪辑（.anim）"));
    connect(openBtn, &QPushButton::clicked, this, &AnimatorEditorWidget::openEditorRequested);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(hint);
    layout->addWidget(openBtn);
    layout->addStretch();
}

void AnimatorEditorWidget::refresh()
{
    // 入口控件无状态需回读；动画数据由 AnimationDock 维护。
    Q_UNUSED(m_comp)
}
