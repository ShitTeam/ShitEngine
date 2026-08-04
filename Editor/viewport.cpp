#include "viewport.h"

#include <QPainter>

Viewport::Viewport(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("viewport");
    setMinimumSize(320, 240);
}

void Viewport::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(40, 42, 48));
    // P2：此处改为绘制引擎渲染结果（或由独立 SDL 窗口承载）
    painter.setPen(QColor(90, 94, 100));
    painter.drawText(rect(), Qt::AlignCenter, tr("视口预览（P2 接入引擎）"));
}
