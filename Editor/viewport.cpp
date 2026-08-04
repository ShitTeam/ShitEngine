#include "viewport.h"

#include <QPainter>

Viewport::Viewport(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("viewport");
    setMinimumSize(320, 240);
}

void Viewport::setFrame(const QImage &frame)
{
    m_frame = frame;
    update();
}

void Viewport::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(40, 42, 48));

    if (!m_frame.isNull()) {
        // 最近邻缩放（像素风不糊）
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(rect(), m_frame);
    } else {
        painter.setPen(QColor(90, 94, 100));
        painter.drawText(rect(), Qt::AlignCenter, tr("引擎预览加载中…"));
    }
}
