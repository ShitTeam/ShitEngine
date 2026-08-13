#include "animatorgraphview.h"

#include <ShitEngine/Animation/Animator.h>

#include <QMenu>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPen>
#include <QPolygon>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {

constexpr qreal kNodeW = 150.0;
constexpr qreal kNodeH = 46.0;
constexpr qreal kAnyStateW = 130.0;
constexpr qreal kAnyStateH = 36.0;
constexpr qreal kDragThreshold = 8.0;  ///< 右键拖拽 vs 菜单的像素阈值

} // namespace

AnimatorGraphView::AnimatorGraphView(QWidget *parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setSceneRect(-800, -500, 1600, 1000);
    setBackgroundBrush(QColor(0x1b, 0x1e, 0x24));
    setFrameShape(QFrame::NoFrame);
    m_scene->setItemIndexMethod(QGraphicsScene::NoIndex);
}

void AnimatorGraphView::setAnimator(Shit::Animator *animator)
{
    m_animator = animator;
    m_selectedState = -1;
    m_selectedTransition = -1;
    m_dragging = false;
    rebuildGraph();
}

void AnimatorGraphView::rebuildGraph()
{
    clearItems();
    if (!m_animator) return;
    layoutFromAnimator();
    rebuildEdges();
    refreshHighlights();
    m_scene->update();
}

void AnimatorGraphView::clearItems()
{
    m_scene->clear();
    m_nodes.clear();
    m_edges.clear();
    m_dragPreview = nullptr;
    m_anyStateNode = nullptr;
}

void AnimatorGraphView::layoutFromAnimator()
{
    // ── "Any State" 锚点节点 ──
    auto *anyRect = new QGraphicsRectItem(0, 0, kAnyStateW, kAnyStateH);
    anyRect->setPos(-740.0, -460.0);
    anyRect->setPen(QPen(QColor(0x55, 0x5c, 0x6a), 1.0, Qt::DashLine));
    anyRect->setBrush(QColor(0x22, 0x26, 0x2e));
    auto *anyLabel = new QGraphicsSimpleTextItem(tr("Any State"), anyRect);
    anyLabel->setFont(QFont("Segoe UI", 8));
    anyLabel->setBrush(QColor(0x8a, 0x9a, 0xaa));
    anyLabel->setPos((kAnyStateW - anyLabel->boundingRect().width()) / 2.0,
                     (kAnyStateH - anyLabel->boundingRect().height()) / 2.0);
    m_scene->addItem(anyRect);
    m_anyStateNode = anyRect;

    const int n = m_animator->stateCount();
    for (int i = 0; i < n; ++i) {
        const Shit::AnimatorState *s = m_animator->stateAt(i);
        if (!s) continue;
        NodeItem node;
        node.stateIndex = i;

        auto *rect = new QGraphicsRectItem(0, 0, kNodeW, kNodeH);
        rect->setPos(s->graphX, s->graphY);
        rect->setFlag(QGraphicsItem::ItemIsMovable);
        rect->setFlag(QGraphicsItem::ItemIsSelectable);
        rect->setFlag(QGraphicsItem::ItemSendsGeometryChanges);

        auto *label = new QGraphicsSimpleTextItem(QString::fromStdString(s->name), rect);
        label->setFont(QFont("Segoe UI", 9));
        // 居中
        label->setPos((kNodeW - label->boundingRect().width()) / 2.0,
                      (kNodeH - label->boundingRect().height()) / 2.0);
        if (s->isEntry)
            label->setBrush(QColor(0xff, 0xc1, 0x6b));

        // 入口状态：橙色菱形标记
        if (s->isEntry) {
            QPolygonF diamond;
            const qreal s = 6.0;
            diamond << QPointF(0, -s) << QPointF(s, 0)
                    << QPointF(0, s) << QPointF(-s, 0);
            auto *badge = new QGraphicsPolygonItem(diamond, rect);
            badge->setBrush(QColor(0xff, 0xc1, 0x6b));
            badge->setPen(Qt::NoPen);
            badge->setPos(4.0, kNodeH / 2.0);
            node.entryBadge = badge;
        }

        m_scene->addItem(rect);
        node.rect = rect;
        node.label = label;
        m_nodes.push_back(node);
    }
}

void AnimatorGraphView::rebuildEdges()
{
    for (auto &e : m_edges)
        if (e.path) m_scene->removeItem(e.path);
    m_edges.clear();

    const int n = m_animator ? m_animator->transitionCount() : 0;
    for (int i = 0; i < n; ++i) {
        const Shit::AnimatorTransition *t = m_animator->transitionAt(i);
        if (!t) continue;
        // 定位端点：from 状态中心（-1 → Any State 锚点）；to 状态中心
        QPointF fromP, toP;
        if (t->fromState >= 0 && t->fromState < static_cast<int>(m_nodes.size())) {
            QGraphicsRectItem *r = m_nodes[static_cast<size_t>(t->fromState)].rect;
            fromP = r->mapToScene(r->rect().center());
        } else if (m_anyStateNode) {
            fromP = m_anyStateNode->mapToScene(m_anyStateNode->rect().center());
        } else {
            fromP = QPointF(-760.0, -460.0);  // 回退
        }
        if (t->toState >= 0 && t->toState < static_cast<int>(m_nodes.size())) {
            QGraphicsRectItem *r = m_nodes[static_cast<size_t>(t->toState)].rect;
            toP = r->mapToScene(r->rect().center());
        } else {
            continue;
        }
        auto *path = new QGraphicsPathItem(makeArrowPath(fromP, toP));
        path->setPen(QPen(QColor(0x8a, 0x9a, 0xaa), 1.8));
        path->setFlag(QGraphicsItem::ItemIsSelectable);
        path->setData(0, i);  // 存转换索引
        m_scene->addItem(path);
        EdgeItem edge;
        edge.transitionIndex = i;
        edge.path = path;
        m_edges.push_back(edge);
    }
}

QPainterPath AnimatorGraphView::makeArrowPath(const QPointF &from, const QPointF &to)
{
    QPainterPath p;
    p.moveTo(from);
    // 中点按 from→to 方向偏移做曲线控制点
    const QPointF ctrl((from.x() + to.x()) / 2.0, (from.y() + to.y()) / 2.0);
    QPointF mid = from + (to - from) * 0.5;
    // 曲线过 ctrl（贝塞尔顶点 = (from + 2*ctrl + to)/4）
    QPointF bezierVertex = (from + 2.0 * ctrl + to) / 4.0;
    p.quadTo(ctrl, to);

    // 箭头三角（在 to 端，指向来源方向）
    QPointF dir = (from - to);
    const qreal len = std::max(std::hypot(dir.x(), dir.y()), 1.0);
    dir /= len;
    QPointF perp(-dir.y(), dir.x());
    const qreal arrowLen = 12.0;
    const qreal arrowW = 6.0;
    QPolygonF tri;
    tri << to
        << to + dir * arrowLen + perp * arrowW
        << to + dir * arrowLen - perp * arrowW;
    Q_UNUSED(mid); Q_UNUSED(bezierVertex);
    p.addPolygon(tri);
    return p;
}

void AnimatorGraphView::mousePressEvent(QMouseEvent *event)
{
    if (!m_animator) { QGraphicsView::mousePressEvent(event); return; }

    const QPointF scenePos = mapToScene(event->pos());
    const int state = stateAt(scenePos);

    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        m_dragFromState = -1;
        // 命中状态 → 选中并交给 QGraphicsView 拖拽；否则选中命中箭头
        if (state >= 0) {
            selectState(state);
            QGraphicsView::mousePressEvent(event);  // 让 rect 自己处理移动
        } else {
            const int edge = transitionAt(scenePos);
            if (edge >= 0) selectTransition(edge);
            else { m_selectedState = -1; m_selectedTransition = -1; refreshHighlights(); emit stateSelected(-1); emit transitionSelected(-1); }
        }
        return;
    }

    if (event->button() == Qt::RightButton) {
        // 开始创建转换：起点为命中的状态（否则任意状态）
        m_dragging = true;
        m_dragFromState = state;
        m_dragStartPos = scenePos;
        // 预览线
        auto *prev = new QGraphicsPathItem(makeArrowPath(scenePos, scenePos));
        prev->setPen(QPen(QColor(0xff, 0xc1, 0x6b), 1.5, Qt::DashLine));
        m_scene->addItem(prev);
        m_dragPreview = prev;
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void AnimatorGraphView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && m_dragPreview) {
        const QPointF p = mapToScene(event->pos());
        m_dragPreview->setPath(makeArrowPath(m_dragStartPos, p));
        return;
    }
    // 拖拽节点移动 → 提交坐标 + 实时重绘相连边
    bool anyMoved = false;
    for (auto &node : m_nodes) {
        if (!node.rect || !node.rect->isSelected()) continue;
        const Shit::AnimatorState *s = m_animator->stateAt(node.stateIndex);
        if (!s) continue;
        if (std::fabs(s->graphX - static_cast<float>(node.rect->pos().x())) < 0.01f
            && std::fabs(s->graphY - static_cast<float>(node.rect->pos().y())) < 0.01f)
            continue;
        Shit::AnimatorState ns = *s;
        ns.graphX = static_cast<float>(node.rect->pos().x());
        ns.graphY = static_cast<float>(node.rect->pos().y());
        m_animator->setState(node.stateIndex, ns);
        anyMoved = true;
    }
    if (anyMoved)
        rebuildEdges();
    QGraphicsView::mouseMoveEvent(event);
}

void AnimatorGraphView::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && event->button() == Qt::RightButton) {
        m_dragging = false;
        if (m_dragPreview) { m_scene->removeItem(m_dragPreview); delete m_dragPreview; m_dragPreview = nullptr; }

        const QPointF p = mapToScene(event->pos());
        const qreal dist = std::hypot(p.x() - m_dragStartPos.x(), p.y() - m_dragStartPos.y());

        // 短右键点击（未拖拽）→ 弹出上下文菜单
        if (dist < kDragThreshold) {
            const int state = stateAt(p);
            const int edge = transitionAt(p);
            QMenu menu(this);
            if (state >= 0) {
                // 在状态节点上右键
                QAction *delState = menu.addAction(tr("删除状态"));
                connect(delState, &QAction::triggered, this, [this, state] {
                    if (m_animator->removeState(state)) {
                        m_selectedState = m_selectedTransition = -1;
                        emit graphChanged();
                        rebuildGraph();
                        emit stateSelected(-1);
                    }
                });
                // 添加从该状态到目标状态的下级菜单
                QMenu *addTransMenu = menu.addMenu(tr("添加转换到..."));
                for (int i = 0; i < m_animator->stateCount(); ++i) {
                    if (i == state) continue;
                    const Shit::AnimatorState *ts = m_animator->stateAt(i);
                    if (!ts) continue;
                    QAction *act = addTransMenu->addAction(QString::fromStdString(ts->name));
                    connect(act, &QAction::triggered, this, [this, state, i] {
                        if (m_animator->addTransition(state, i) >= 0)
                            emit graphChanged();
                        rebuildGraph();
                    });
                }
            } else if (edge >= 0) {
                // 在转换箭头上右键
                QAction *delTrans = menu.addAction(tr("删除转换"));
                connect(delTrans, &QAction::triggered, this, [this, edge] {
                    if (m_animator->removeTransition(edge)) {
                        m_selectedTransition = -1;
                        emit graphChanged();
                        rebuildGraph();
                        emit transitionSelected(-1);
                    }
                });
            } else {
                // 空白处右键 → 添加状态
                QAction *addState = menu.addAction(tr("添加状态"));
                connect(addState, &QAction::triggered, this, [this] {
                    if (m_animator->addState("State") >= 0) {
                        emit graphChanged();
                        rebuildGraph();
                    }
                });
            }
            menu.exec(event->globalPosition().toPoint());
            return;
        }

        // 拖拽创建转换
        const int toState = stateAt(p);
        if (m_dragFromState != toState && toState >= 0) {
            if (m_animator->addTransition(m_dragFromState, toState) >= 0)
                emit graphChanged();
        }
        rebuildGraph();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void AnimatorGraphView::wheelEvent(QWheelEvent *event)
{
    const qreal factor = std::pow(1.0015, event->angleDelta().y());
    scaleView(factor);
}

void AnimatorGraphView::scaleView(qreal scaleFactor)
{
    const qreal minS = 0.3, maxS = 3.0;
    const qreal cur = transform().m11();
    const qreal next = std::clamp(cur * scaleFactor, minS, maxS);
    if (std::fabs(next - cur) < 0.0001) return;
    scale(next / cur, next / cur);
}

void AnimatorGraphView::keyPressEvent(QKeyEvent *event)
{
    // Delete 删除选中转换或状态
    if (event->key() == Qt::Key_Delete) {
        if (m_selectedTransition >= 0 && m_animator) {
            if (m_animator->removeTransition(m_selectedTransition)) {
                m_selectedTransition = -1;
                emit graphChanged();
                rebuildGraph();
                emit transitionSelected(-1);
            }
        } else if (m_selectedState >= 0 && m_animator) {
            if (m_animator->removeState(m_selectedState)) {
                m_selectedState = -1;
                emit graphChanged();
                rebuildGraph();
                emit stateSelected(-1);
            }
        }
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

int AnimatorGraphView::stateAt(const QPointF &scenePos) const
{
    QGraphicsItem *item = m_scene->itemAt(scenePos, QTransform());
    while (item) {
        for (size_t i = 0; i < m_nodes.size(); ++i)
            if (m_nodes[i].rect == item) return static_cast<int>(i);
        item = item->parentItem();
    }
    return -1;
}

int AnimatorGraphView::transitionAt(const QPointF &scenePos) const
{
    QGraphicsItem *item = m_scene->itemAt(scenePos, QTransform());
    while (item) {
        QVariant v = item->data(0);
        if (v.isValid() && v.canConvert<int>())
            return v.toInt();
        item = item->parentItem();
    }
    return -1;
}

void AnimatorGraphView::refreshHighlights()
{
    // 状态节点高亮
    for (size_t i = 0; i < m_nodes.size(); ++i) {
        const bool sel = (static_cast<int>(i) == m_selectedState);
        m_nodes[i].rect->setPen(QPen(sel ? QColor(0x4f, 0xc3, 0xf7) : QColor(0x3a, 0x41, 0x4d),
                                     sel ? 2.5 : 1.0));
    }
    // 转换箭头高亮
    for (auto &e : m_edges) {
        const bool sel = (e.transitionIndex == m_selectedTransition);
        e.path->setPen(QPen(sel ? QColor(0x4f, 0xc3, 0xf7) : QColor(0x8a, 0x9a, 0xaa),
                            sel ? 3.0 : 1.8));
    }
    m_scene->update();
}

void AnimatorGraphView::selectState(int index)
{
    m_selectedState = index;
    m_selectedTransition = -1;
    refreshHighlights();
    emit stateSelected(index);
    emit transitionSelected(-1);
}

void AnimatorGraphView::selectTransition(int index)
{
    m_selectedTransition = index;
    m_selectedState = -1;
    refreshHighlights();
    emit transitionSelected(index);
    emit stateSelected(-1);
}


