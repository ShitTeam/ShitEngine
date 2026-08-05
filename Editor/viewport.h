#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QImage>
#include <QWidget>

#include <QRect>

class QMouseEvent;
class QWheelEvent;

namespace Shit { class Scene; class GameObject; class CameraComponent; }

/// 场景视口：显示引擎离屏渲染结果，支持场景视图编辑器交互：
///   - 点击拾取（logicalClicked）
///   - 中键拖拽平移编辑器相机，滚轮缩放
///   - 选中对象上绘制移动 Gizmo（X/Y 轴手柄），拖动写回 Transform
class Viewport : public QWidget
{
    Q_OBJECT
public:
    explicit Viewport(QWidget *parent = nullptr);

    /// 更新显示一帧引擎渲染结果
    void setFrame(const QImage &frame);

    /// 设置编辑器交互上下文（场景视图用；会从中找编辑器相机）
    void setEditScene(Shit::Scene *scene);

    /// 设置选中对象（在其上绘制移动 Gizmo）
    void setSelectedObject(Shit::GameObject *object);

signals:
    /// 点击视口（逻辑像素坐标，0..1280 × 0..720；供拾取用）
    void logicalClicked(float x, float y);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    /// 逻辑像素坐标 → 控件坐标（逆 letterbox）
    QPoint logicalToWidget(float lx, float ly) const;
    /// 控件坐标 → 逻辑像素坐标
    QPointF widgetToLogical(const QPoint &pos) const;
    /// 编辑器相机（m_editScene 中第一个 CameraComponent）
    Shit::CameraComponent *editorCamera() const;
    /// 绘制选中对象的移动 Gizmo
    void drawGizmo(QPainter &painter);
    /// 点到线段距离（Gizmo 手柄命中判定）
    static float distToSegment(const QPointF &p, const QPointF &a, const QPointF &b);

    enum class DragMode { None, GizmoX, GizmoY, Pan };
    DragMode m_drag = DragMode::None;
    QPoint m_dragStartWidget;       ///< 拖动起点（控件坐标）
    float m_dragStartPosX = 0.0f;   ///< 对象拖动前世界位置
    float m_dragStartPosY = 0.0f;
    float m_panStartCamX = 0.0f;    ///< 相机平移前位置
    float m_panStartCamY = 0.0f;

    QImage m_frame;
    QRect m_drawRect;               ///< 帧实际绘制区域（letterbox 居中后）
    Shit::Scene *m_editScene = nullptr;
    Shit::GameObject *m_selected = nullptr;
};

#endif // VIEWPORT_H