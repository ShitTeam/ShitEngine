#include "viewport.h"

#include <QMouseEvent>
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
        // 保持宽高比缩放并居中（letterbox，对齐 Unity 视图行为）
        QSize target = m_frame.size();
        target.scale(rect().size(), Qt::KeepAspectRatio);
        m_drawRect = QRect(QPoint(0, 0), target);
        m_drawRect.moveCenter(rect().center());
        // 最近邻缩放（像素风不糊）
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawImage(m_drawRect, m_frame);
    } else {
        m_drawRect = QRect();
        painter.setPen(QColor(90, 94, 100));
        painter.drawText(rect(), Qt::AlignCenter, tr("引擎预览加载中…"));
    }
}

void Viewport::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && !m_frame.isNull()
        && m_drawRect.contains(event->pos())) {
        // 控件坐标 → 逻辑像素坐标（逆 letterbox）
        const float lx = (event->pos().x() - m_drawRect.left())
                       * static_cast<float>(m_frame.width()) / m_drawRect.width();
        const float ly = (event->pos().y() - m_drawRect.top())
                       * static_cast<float>(m_frame.height()) / m_drawRect.height();
        emit logicalClicked(lx, ly);
    }
    QWidget::mousePressEvent(event);
}
