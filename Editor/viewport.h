#ifndef VIEWPORT_H
#define VIEWPORT_H

#include <QImage>
#include <QWidget>

#include <QRect>

class QMouseEvent;
class QWheelEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

namespace Shit { class Scene; class GameObject; class CameraComponent; }

/// 场景视口：显示引擎离屏渲染结果，支持场景视图编辑器交互：
///   - 点击拾取（logicalClicked）
///   - 中键拖拽平移编辑器相机，滚轮缩放
///   - 选中对象上的 Gizmo（移动/旋转/缩放三模式，Q/W/E 或工具栏切换）
class Viewport : public QWidget
{
    Q_OBJECT
public:
    /// Gizmo 操作模式（对齐 Unity：Q=移动 W=旋转 E=缩放）
    enum class GizmoMode { Move, Rotate, Scale };

    explicit Viewport(QWidget *parent = nullptr);

    /// 更新显示一帧引擎渲染结果
    void setFrame(const QImage &frame);

    /// 设置编辑器交互上下文（场景视图用；会从中找编辑器相机）
    void setEditScene(Shit::Scene *scene);

    /// 设置选中对象（在其上绘制 Gizmo）
    void setSelectedObject(Shit::GameObject *object);

    /// 切换 Gizmo 模式（移动/旋转/缩放）
    void setGizmoMode(GizmoMode mode) { m_gizmoMode = mode; update(); }
    GizmoMode gizmoMode() const { return m_gizmoMode; }

    /// 控件坐标 → 逻辑像素坐标（播放态输入转发等外部使用）
    QPointF mapToLogical(const QPoint &pos) const { return widgetToLogical(pos); }

signals:
    /// 点击视口（逻辑像素坐标，0..1280 × 0..720；供拾取用）
    void logicalClicked(float x, float y);
    /// Gizmo 拖拽开始（对象位置将被修改 → 撤销 begin）
    void gizmoDragStarted();
    /// Gizmo 拖拽结束（对象位置已被修改 → 会话 dirty / 撤销提交）
    void gizmoDragFinished();
    /// 资源文件（图片）被拖入视口（逻辑像素坐标；供创建精灵用）
    void assetDropped(const QString &path, float logicalX, float logicalY);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;   ///< 接受图片文件拖入
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

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

    enum class DragMode { None, GizmoX, GizmoY, Rotate, ScaleX, ScaleY, Pan };
    DragMode m_drag = DragMode::None;
    QPoint m_dragStartWidget;       ///< 拖动起点（控件坐标）
    float m_dragStartPosX = 0.0f;   ///< 对象拖动前世界位置
    float m_dragStartPosY = 0.0f;
    float m_dragStartRotation = 0.0f;  ///< 旋转模式：起始旋转与起始量化角
    float m_dragStartSnapped = 0.0f;
    float m_dragStartScaleX = 1.0f;    ///< 缩放模式：起始缩放
    float m_dragStartScaleY = 1.0f;
    float m_panStartCamX = 0.0f;    ///< 相机平移前位置
    float m_panStartCamY = 0.0f;

    QImage m_frame;
    QRect m_drawRect;               ///< 帧实际绘制区域（letterbox 居中后）
    Shit::Scene *m_editScene = nullptr;
    Shit::GameObject *m_selected = nullptr;
    GizmoMode m_gizmoMode = GizmoMode::Move;
};

#endif // VIEWPORT_H