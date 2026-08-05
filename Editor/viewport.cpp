#include "viewport.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {

/// 点到线段距离（平方），用于 Gizmo 手柄命中
float distToSegmentSq(const QPointF &p, const QPointF &a, const QPointF &b)
{
    const QPointF ab = b - a;
    const float len2 = static_cast<float>(ab.x() * ab.x() + ab.y() * ab.y());
    if (len2 <= 1e-6f)
        return static_cast<float>((p - a).manhattanLength());
    const float t = static_cast<float>(((p.x() - a.x()) * ab.x() + (p.y() - a.y()) * ab.y()) / len2);
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    const QPointF proj = a + ab * clamped;
    const float dx = static_cast<float>(p.x() - proj.x());
    const float dy = static_cast<float>(p.y() - proj.y());
    return dx * dx + dy * dy;
}

} // namespace

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

void Viewport::setEditScene(Shit::Scene *scene)
{
    m_editScene = scene;
}

void Viewport::setSelectedObject(Shit::GameObject *object)
{
    m_selected = object;
    update();
}

Shit::CameraComponent *Viewport::editorCamera() const
{
    if (!m_editScene) return nullptr;
    for (auto &go : m_editScene->getGameObjects())
        if (auto *cam = go->getComponent<Shit::CameraComponent>())
            return cam;
    return nullptr;
}

QPoint Viewport::logicalToWidget(float lx, float ly) const
{
    if (m_frame.isNull() || m_drawRect.isEmpty())
        return { 0, 0 };
    const int wx = m_drawRect.left() + static_cast<int>(lx / m_frame.width() * m_drawRect.width());
    const int wy = m_drawRect.top() + static_cast<int>(ly / m_frame.height() * m_drawRect.height());
    return { wx, wy };
}

QPointF Viewport::widgetToLogical(const QPoint &pos) const
{
    if (m_frame.isNull() || m_drawRect.isEmpty())
        return { 0.0, 0.0 };
    const float lx = static_cast<float>(pos.x() - m_drawRect.left())
                   / m_drawRect.width() * m_frame.width();
    const float ly = static_cast<float>(pos.y() - m_drawRect.top())
                   / m_drawRect.height() * m_frame.height();
    return { lx, ly };
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
        drawGizmo(painter);
    } else {
        m_drawRect = QRect();
        painter.setPen(QColor(90, 94, 100));
        painter.drawText(rect(), Qt::AlignCenter, tr("引擎预览加载中…"));
    }
}

void Viewport::drawGizmo(QPainter &painter)
{
    if (!m_selected || !m_editScene) return;
    auto *camera = editorCamera();
    auto *transform = m_selected->getComponent<Shit::TransformComponent>();
    if (!camera || !transform) return;

    const Shit::Vector2 pos = transform->getPosition();
    const Shit::Vector2 sp = camera->worldToScreen(pos);
    const QPoint p = logicalToWidget(sp.x, sp.y);
    const int len = 30;

    // 中心方块 + X(Y)轴手柄
    painter.setBrush(Qt::white);
    painter.drawRect(p.x() - 3, p.y() - 3, 6, 6);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(255, 90, 90), 2));   // X 轴（红）
    painter.drawLine(p, p + QPoint(len, 0));
    painter.drawLine(p + QPoint(len, 0), p + QPoint(len - 6, -4));
    painter.drawLine(p + QPoint(len, 0), p + QPoint(len - 6, 4));

    painter.setPen(QPen(QColor(90, 255, 90), 2));   // Y 轴（绿）
    painter.drawLine(p, p + QPoint(0, len));
    painter.drawLine(p + QPoint(0, len), p + QPoint(-4, len - 6));
    painter.drawLine(p + QPoint(0, len), p + QPoint(4, len - 6));
}

void Viewport::mousePressEvent(QMouseEvent *event)
{
    const QPoint pos = event->pos();
    if (event->button() == Qt::MiddleButton) {
        if (!m_frame.isNull() && m_drawRect.contains(pos)) {
            m_drag = DragMode::Pan;
            m_dragStartWidget = pos;
            if (auto *camera = editorCamera())
                if (auto *t = camera->getOwner()->getComponent<Shit::TransformComponent>()) {
                    const Shit::Vector2 c = t->getPosition();
                    m_panStartCamX = c.x;
                    m_panStartCamY = c.y;
                }
        }
        QWidget::mousePressEvent(event);
        return;
    }

    if (event->button() == Qt::LeftButton && m_selected && !m_frame.isNull()
        && m_drawRect.contains(pos)) {
        // Gizmo 手柄命中检测
        auto *camera = editorCamera();
        auto *transform = m_selected->getComponent<Shit::TransformComponent>();
        if (camera && transform) {
            const Shit::Vector2 sp = camera->worldToScreen(transform->getPosition());
            const QPointF p = logicalToWidget(sp.x, sp.y);
            const QPointF px(p.x() + 30, p.y());
            const QPointF py(p.x(), p.y() + 30);
            const QPointF m(pos.x(), pos.y());
            if (distToSegmentSq(m, p, px) < 64.0f) {      // 8px 命中半径
                m_drag = DragMode::GizmoX;
                m_dragStartWidget = pos;
                m_dragStartPosX = transform->getPosition().x;
                m_dragStartPosY = transform->getPosition().y;
                QWidget::mousePressEvent(event);
                return;
            }
            if (distToSegmentSq(m, p, py) < 64.0f) {
                m_drag = DragMode::GizmoY;
                m_dragStartWidget = pos;
                m_dragStartPosX = transform->getPosition().x;
                m_dragStartPosY = transform->getPosition().y;
                QWidget::mousePressEvent(event);
                return;
            }
        }
        // 未命中 Gizmo → 拾取
        const QPointF logical = widgetToLogical(pos);
        emit logicalClicked(static_cast<float>(logical.x()), static_cast<float>(logical.y()));
    }
    QWidget::mousePressEvent(event);
}

void Viewport::mouseMoveEvent(QMouseEvent *event)
{
    if (m_drag == DragMode::None || m_frame.isNull() || m_drawRect.isEmpty()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    auto *camera = editorCamera();
    if (!camera) return;
    const float ppu = camera->getPixelPerUnit();
    if (ppu <= 0.0f) return;

    // 控件位移 → 逻辑像素 → 世界单位
    const int dx = event->pos().x() - m_dragStartWidget.x();
    const int dy = event->pos().y() - m_dragStartWidget.y();
    const float logicalDx = static_cast<float>(dx) / m_drawRect.width() * m_frame.width();
    const float logicalDy = static_cast<float>(dy) / m_drawRect.height() * m_frame.height();
    const float worldDx = logicalDx / ppu;
    const float worldDy = logicalDy / ppu;

    if (m_drag == DragMode::GizmoX || m_drag == DragMode::GizmoY) {
        if (auto *transform = m_selected ? m_selected->getComponent<Shit::TransformComponent>() : nullptr) {
            const float nx = (m_drag == DragMode::GizmoX) ? m_dragStartPosX + worldDx : m_dragStartPosX;
            const float ny = (m_drag == DragMode::GizmoY) ? m_dragStartPosY + worldDy : m_dragStartPosY;
            transform->setPosition({ nx, ny });
            update();
        }
    } else if (m_drag == DragMode::Pan) {
        if (auto *t = camera->getOwner()->getComponent<Shit::TransformComponent>()) {
            t->setPosition({ m_panStartCamX - worldDx, m_panStartCamY - worldDy });
            update();
        }
    }
    QWidget::mouseMoveEvent(event);
}

void Viewport::mouseReleaseEvent(QMouseEvent *event)
{
    m_drag = DragMode::None;
    QWidget::mouseReleaseEvent(event);
}

void Viewport::wheelEvent(QWheelEvent *event)
{
    if (auto *camera = editorCamera()) {
        const float factor = (event->angleDelta().y() > 0) ? 1.1f : (1.0f / 1.1f);
        camera->setZoom(std::clamp(camera->getZoom() * factor, 0.1f, 10.0f));
        update();
        event->accept();
        return;
    }
    QWidget::wheelEvent(event);
}
