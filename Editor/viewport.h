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
class QResizeEvent;
class QToolButton;
class QButtonGroup;

namespace Shit { class Scene; class GameObject; class CameraComponent; }

/// 场景视口：显示引擎离屏渲染结果，支持场景视图编辑器交互：
///   - 点击拾取（logicalClicked）
///   - 中键拖拽平移编辑器相机，滚轮缩放
///   - 选中对象上的 Gizmo（移动/旋转/缩放三模式）
///   - 视图内左上角工具条切换 Gizmo 模式（Unity 风格；Q/W/E 快捷键另行注册）
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

    /// 切换 Gizmo 模式（移动/旋转/缩放；同步视图内工具条选中态）
    void setGizmoMode(GizmoMode mode) { m_gizmoMode = mode; syncGizmoBar(); update(); }
    GizmoMode gizmoMode() const { return m_gizmoMode; }

    /// 是否显示视图内 Gizmo 工具条（运行视口为 false——运行态无编辑，按钮徒增干扰）
    void setGizmoBarVisible(bool visible) { if (m_gizmoBar) m_gizmoBar->setVisible(visible); }

    /// 播放态编辑锁（P25d）：关闭后 Gizmo 与碰撞体手柄不可拖拽（拾取仍可用）
    void setEditEnabled(bool enabled) { m_editEnabled = enabled; update(); }

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
    /// .prefab 预置资产被拖入视口（逻辑像素坐标；供实例化用，P25c）
    void prefabDropped(const QString &path, float logicalX, float logicalY);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
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
    /// 绘制碰撞体调试轮廓（刚体类型着色；视图内工具条「碰撞体」开关）
    void drawPhysicsDebug(QPainter &painter);
    /// 点到线段距离（Gizmo 手柄命中判定）
    static float distToSegment(const QPointF &p, const QPointF &a, const QPointF &b);
    /// P25b：计算选中对象碰撞体手柄的绘制/命中几何（并校验存活）。
    /// 成功返回 true 并输出中心屏幕点 / 旋转弧度 / 像素比例。
    bool colliderHandleGeom(QPointF &center, float &rotRad, float &pixelScale) const;

    enum class DragMode { None, GizmoX, GizmoY, Move, Rotate, ScaleX, ScaleY,
                          ColliderBox, ColliderCircle, Pan };
    DragMode m_drag = DragMode::None;
    QPoint m_dragStartWidget;       ///< 拖动起点（控件坐标）
    float m_dragStartPosX = 0.0f;   ///< 对象拖动前世界位置
    float m_dragStartPosY = 0.0f;
    float m_dragStartRotation = 0.0f;  ///< 旋转模式：起始旋转与起始量化角
    float m_dragStartSnapped = 0.0f;
    float m_dragStartScaleX = 1.0f;    ///< 缩放模式：起始缩放
    float m_dragStartScaleY = 1.0f;
    float m_dragStartSizeX = 0.0f;     ///< 碰撞体手柄：起始尺寸/半径与角索引
    float m_dragStartSizeY = 0.0f;
    int m_dragCorner = 0;
    float m_dragPixelScale = 1.0f;     ///< 碰撞体手柄：逻辑像素→控件像素比例（拖拽期缓存）
    float m_panStartCamX = 0.0f;    ///< 相机平移前位置
    float m_panStartCamY = 0.0f;

    QImage m_frame;
    QRect m_drawRect;               ///< 帧实际绘制区域（letterbox 居中后）
    Shit::Scene *m_editScene = nullptr;
    Shit::GameObject *m_selected = nullptr;
    GizmoMode m_gizmoMode = GizmoMode::Move;
    bool m_editEnabled = true;   ///< 播放态编辑锁（P25d）

    // P14：视图内 Gizmo 工具条（左上角，Unity 风格）
    QWidget *m_gizmoBar = nullptr;
    QButtonGroup *m_gizmoBarGroup = nullptr;
    QToolButton *m_gizmoMoveBtn = nullptr;
    QToolButton *m_gizmoRotateBtn = nullptr;
    QToolButton *m_gizmoScaleBtn = nullptr;
    QToolButton *m_colliderToggleBtn = nullptr;   ///< 碰撞体轮廓显示开关（不属互斥组）
    bool m_showColliders = true;

    /// 构建左上角工具条（构造时调用）；setGizmoMode 同步按钮选中态
    void setupGizmoBar();
    void syncGizmoBar();
};

#endif // VIEWPORT_H