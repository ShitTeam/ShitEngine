#ifndef ANIMATORGRAPHVIEW_H
#define ANIMATORGRAPHVIEW_H

#include <QGraphicsView>

class QGraphicsScene;
class QGraphicsRectItem;
class QGraphicsPathItem;
class QGraphicsItem;
class QPointF;

namespace Shit { class Animator; }

/// Unity 风格 Animator 状态机图：QGraphicsView 画布。
/// - 每个状态 = 一个可拖拽的方块节点（入口状态额外标记）
/// - 每个转换 = 一条带箭头的曲线（from→to；from=-1 为"任意→目标"，从场景空白处引出）
/// - 交互：拖拽节点移动；右键从一个状态拖到另一个状态 → 创建转换；点节点/箭头选中
/// - 变化经 animatorChanged() 通知外部（写回 Animator + 撤销）
class AnimatorGraphView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit AnimatorGraphView(QWidget *parent = nullptr);

    /// 绑定/解绑 Animator（解绑传 nullptr）
    void setAnimator(Shit::Animator *animator);

    /// 重绘整张状态机图（从 Animator 读取当前状态/转换/坐标）
    void rebuildGraph();

    /// 当前选中的状态索引；-1 = 无
    int selectedState() const { return m_selectedState; }
    /// 当前选中的转换索引；-1 = 无
    int selectedTransition() const { return m_selectedTransition; }

    /// 程序选中某个状态（外部调用，例如参数面板操作后）
    void selectState(int index);
    /// 程序选中某个转换
    void selectTransition(int index);

signals:
    /// 状态机图被修改（节点移动/连线创建/选中），外部据此写回与刷新
    void graphChanged();
    /// 选中状态改变（参数=选中索引；外部据此刷新右侧属性面板）
    void stateSelected(int index);
    /// 选中转换改变
    void transitionSelected(int index);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    struct NodeItem {
        int stateIndex = -1;
        QGraphicsRectItem *rect = nullptr;
        QGraphicsSimpleTextItem *label = nullptr;
        QGraphicsPolygonItem *entryBadge = nullptr;  ///< 入口状态橙色菱形标记
    };
    struct EdgeItem {
        int transitionIndex = -1;
        QGraphicsPathItem *path = nullptr;
    };

    void layoutFromAnimator();
    void clearItems();
    /// 重建所有转换箭头
    void rebuildEdges();
    /// 缩放画布
    void scaleView(qreal scaleFactor);
    /// 由两个节点中心计算贝塞尔箭头路径
    static QPainterPath makeArrowPath(const QPointF &from, const QPointF &to);
    /// 查找点击命中的节点/箭头
    int stateAt(const QPointF &scenePos) const;
    int transitionAt(const QPointF &scenePos) const;
    /// 高亮选中
    void refreshHighlights();

    Shit::Animator *m_animator = nullptr;
    QGraphicsScene *m_scene = nullptr;
    std::vector<NodeItem> m_nodes;
    std::vector<EdgeItem> m_edges;

    int m_selectedState = -1;
    int m_selectedTransition = -1;

    // 连线创建（右键拖拽）
    int m_dragFromState = -1;   ///< 右键起点状态；-1 表示从空白（任意→目标）
    bool m_dragging = false;
    QPointF m_dragStartPos;
    QGraphicsPathItem *m_dragPreview = nullptr;

    // "Any State" 锚点节点（from=-1 的转换绘制起点）
    QGraphicsRectItem *m_anyStateNode = nullptr;

    bool m_updating = false;
};

#endif // ANIMATORGRAPHVIEW_H
