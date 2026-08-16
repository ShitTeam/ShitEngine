#include "viewport.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>

#include <QButtonGroup>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <QResizeEvent>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
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

/// MIME 的 URL 列表中第一个图片文件路径；无则返回 false
bool firstImageFile(const QMimeData *mime, QString &path)
{
    if (!mime || !mime->hasUrls()) return false;
    static const QStringList kImages = { "png", "jpg", "jpeg", "bmp" };
    for (const QUrl &url : mime->urls()) {
        const QString p = url.toLocalFile();
        if (kImages.contains(QFileInfo(p).suffix().toLower())) {
            path = p;
            return true;
        }
    }
    return false;
}

/// MIME 的 URL 列表中第一个 .prefab 文件路径；无则返回 false（P25c）
bool firstPrefabFile(const QMimeData *mime, QString &path)
{
    if (!mime || !mime->hasUrls()) return false;
    for (const QUrl &url : mime->urls()) {
        const QString p = url.toLocalFile();
        if (QFileInfo(p).suffix().compare(QStringLiteral("prefab"), Qt::CaseInsensitive) == 0) {
            path = p;
            return true;
        }
    }
    return false;
}

} // namespace

Viewport::Viewport(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("viewport");
    setMinimumSize(320, 240);
    setAcceptDrops(true);   // P10：接受资源面板拖入的图片文件
    setupGizmoBar();        // P14：左上角 Gizmo 工具条（Unity 风格，内联于场景视口）
}

void Viewport::setupGizmoBar()
{
    m_gizmoBar = new QWidget(this);
    m_gizmoBar->setObjectName("gizmoBar");
    m_gizmoBar->setStyleSheet(QStringLiteral(
        "#gizmoBar { background: rgba(16, 18, 24, 200); border: 1px solid rgba(120, 130, 150, 90);"
        "  border-radius: 4px; }"
        "QToolButton { border: none; color: #c8d0dc; padding: 4px 10px; font-size: 12px;"
        "  border-radius: 3px; }"
        "QToolButton:hover { background: rgba(255, 255, 255, 30); }"
        "QToolButton:checked { background: #2d6cdf; color: white; }"));

    auto addBarButton = [this](const QString &text, const QString &tip) {
auto *btn = new QToolButton(m_gizmoBar);
        btn->setText(text);
        btn->setToolTip(tip);
        btn->setCheckable(true);
        btn->setAutoRaise(true);
        btn->setCursor(Qt::PointingHandCursor);
        return btn;
    };
    m_gizmoMoveBtn = addBarButton(tr("移动"), tr("移动工具（Q）"));
    m_gizmoRotateBtn = addBarButton(tr("旋转"), tr("旋转工具（W）"));
    m_gizmoScaleBtn = addBarButton(tr("缩放"), tr("缩放工具（E）"));

    m_gizmoBarGroup = new QButtonGroup(this);
    m_gizmoBarGroup->setExclusive(true);
    m_gizmoBarGroup->addButton(m_gizmoMoveBtn, 0);
    m_gizmoBarGroup->addButton(m_gizmoRotateBtn, 1);
    m_gizmoBarGroup->addButton(m_gizmoScaleBtn, 2);

    connect(m_gizmoMoveBtn, &QToolButton::clicked, this, [this] { setGizmoMode(GizmoMode::Move); });
    connect(m_gizmoRotateBtn, &QToolButton::clicked, this, [this] { setGizmoMode(GizmoMode::Rotate); });
    connect(m_gizmoScaleBtn, &QToolButton::clicked, this, [this] { setGizmoMode(GizmoMode::Scale); });

    // 碰撞体轮廓开关（独立于互斥组，默认开启）
    m_colliderToggleBtn = addBarButton(tr("碰撞体"), tr("显示/隐藏碰撞体轮廓"));
    m_colliderToggleBtn->setChecked(true);
    connect(m_colliderToggleBtn, &QToolButton::clicked, this, [this] {
        m_showColliders = m_colliderToggleBtn->isChecked();
        update();
    });

    auto *layout = new QHBoxLayout(m_gizmoBar);
    layout->setContentsMargins(3, 3, 3, 3);
    layout->setSpacing(2);
    layout->addWidget(m_gizmoMoveBtn);
    layout->addWidget(m_gizmoRotateBtn);
    layout->addWidget(m_gizmoScaleBtn);
    layout->addSpacing(6);
    layout->addWidget(m_colliderToggleBtn);

    setGizmoMode(GizmoMode::Move);   // 初始选中（同步按钮态）
    m_gizmoBar->adjustSize();
    m_gizmoBar->move(QPoint(8, 8));
}

void Viewport::syncGizmoBar()
{
    if (!m_gizmoBarGroup) return;
    switch (m_gizmoMode) {
        case GizmoMode::Move:  m_gizmoBarGroup->button(0)->setChecked(true); break;
        case GizmoMode::Rotate: m_gizmoBarGroup->button(1)->setChecked(true); break;
        case GizmoMode::Scale: m_gizmoBarGroup->button(2)->setChecked(true); break;
    }
}

void Viewport::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_gizmoBar) m_gizmoBar->move(QPoint(8, 8));   // 固定左上角
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
    // 编辑器相机是约定名 scene_camera（场景视图基础设施，Gizmo 拾取/平移/缩放
    // 都基于它）；找不到才回退场景中第一个相机（老场景兼容）
    for (auto &go : m_editScene->getGameObjects())
        if (go->getName() == "scene_camera")
            if (auto *cam = go->getComponent<Shit::CameraComponent>())
                return cam;
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
        drawPhysicsDebug(painter);
        drawTilemapGrid(painter);
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
    // 播放中游戏逻辑可能销毁了选中的对象：不在场景中 → 视为未选中（防悬垂解引用）
    if (!m_editScene->containsGameObject(m_selected)) {
        m_selected = nullptr;
        return;
    }
    auto *camera = editorCamera();
    auto *transform = m_selected->getComponent<Shit::TransformComponent>();
    if (!camera || !transform) return;

    const Shit::Vector2 pos = transform->getPosition();
    const Shit::Vector2 sp = camera->worldToScreen(pos);
    const QPoint p = logicalToWidget(sp.x, sp.y);
    const int len = 30;

    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_gizmoMode == GizmoMode::Rotate) {
        // 旋转：圆环 + 中心方块 + 当前角度指针
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(255, 190, 90), 2));
        painter.drawEllipse(QPointF(p), 40, 40);
        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawRect(p.x() - 3, p.y() - 3, 6, 6);
        const float rad = transform->getRotation() * 3.14159265f / 180.0f;
        const QPointF tip(p.x() + 40.0f * std::cos(rad), p.y() + 40.0f * std::sin(rad));
        painter.setPen(QPen(QColor(255, 190, 90), 2));
        painter.drawLine(p, tip.toPoint());
        return;
    }

    if (m_gizmoMode == GizmoMode::Scale) {
        // 缩放：中心方块 + X(红)/Y(绿) 轴端方块（可分别拖拽）
        painter.setBrush(Qt::white);
        painter.setPen(Qt::NoPen);
        painter.drawRect(p.x() - 3, p.y() - 3, 6, 6);
        painter.setPen(QPen(QColor(255, 90, 90), 2));
        painter.drawLine(p, p + QPoint(len, 0));
        painter.setBrush(QColor(255, 90, 90));
        painter.setPen(Qt::NoPen);
        painter.drawRect(p.x() + len - 5, p.y() - 5, 10, 10);
        painter.setPen(QPen(QColor(90, 255, 90), 2));
        painter.drawLine(p, p + QPoint(0, len));
        painter.setBrush(QColor(90, 255, 90));
        painter.setPen(Qt::NoPen);
        painter.drawRect(p.x() - 5, p.y() + len - 5, 10, 10);
        return;
    }

    // Move：中心方块 + X(Y)轴手柄
    painter.setBrush(Qt::white);
    painter.setPen(Qt::NoPen);
    painter.drawRect(p.x() - 3, p.y() - 3, 6, 6);

    painter.setPen(QPen(QColor(255, 90, 90), 2));   // X 轴（红）
    painter.drawLine(p, p + QPoint(len, 0));
    painter.drawLine(p + QPoint(len, 0), p + QPoint(len - 6, -4));
    painter.drawLine(p + QPoint(len, 0), p + QPoint(len - 6, 4));

    painter.setPen(QPen(QColor(90, 255, 90), 2));   // Y 轴（绿）
    painter.drawLine(p, p + QPoint(0, len));
    painter.drawLine(p + QPoint(0, len), p + QPoint(-4, len - 6));
    painter.drawLine(p + QPoint(0, len), p + QPoint(4, len - 6));
}

void Viewport::drawTilemapGrid(QPainter &painter)
{
    // 仅选中含 Tilemap 的对象时绘制网格线（辅助刷图对齐）
    if (!m_selected || !m_editScene || m_frame.isNull() || m_drawRect.isEmpty())
        return;
    // 播放中游戏逻辑可能销毁了选中对象 → 视为未选中
    if (!m_editScene->containsGameObject(m_selected)) return;
    auto *camera = editorCamera();
    if (!camera) return;
    auto *tilemap = m_selected->getComponent<Shit::Tilemap>();
    if (!tilemap) return;
    auto *transform = m_selected->getComponent<Shit::TransformComponent>();
    if (!transform) return;

    const int cols = tilemap->getGridWidth();
    const int rows = tilemap->getGridHeight();
    if (cols <= 0 || rows <= 0) return;
    const Shit::Vector2 cell = tilemap->getTileWorldSize();
    const float cellW = (cell.x > 0.0f) ? cell.x : static_cast<float>(tilemap->getTileWidth());
    const float cellH = (cell.y > 0.0f) ? cell.y : static_cast<float>(tilemap->getTileHeight());

    const Shit::Vector2 origin = transform->getPosition();
    const float totalW = cols * cellW;
    const float totalH = rows * cellH;

    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(0, 200, 220, 90), 1, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    // 竖线
    for (int i = 0; i <= cols; ++i) {
        const float wx = origin.x + i * cellW;
        const Shit::Vector2 sp = camera->worldToScreen({ wx, origin.y });
        const QPointF p = logicalToWidget(sp.x, sp.y);
        const Shit::Vector2 spBot = camera->worldToScreen({ wx, origin.y + totalH });
        const QPointF pBot = logicalToWidget(spBot.x, spBot.y);
        painter.drawLine(p, pBot);
    }
    // 横线
    for (int j = 0; j <= rows; ++j) {
        const float wy = origin.y + j * cellH;
        const Shit::Vector2 sp = camera->worldToScreen({ origin.x, wy });
        const QPointF p = logicalToWidget(sp.x, sp.y);
        const Shit::Vector2 spRight = camera->worldToScreen({ origin.x + totalW, wy });
        const QPointF pRight = logicalToWidget(spRight.x, spRight.y);
        painter.drawLine(p, pRight);
    }
}

bool Viewport::paintTileAt(const QPoint &widgetPos)
{
    if (!m_selected || !m_editScene || m_frame.isNull() || m_drawRect.isEmpty())
        return false;
    if (!m_editScene->containsGameObject(m_selected)) return false;
    auto *camera = editorCamera();
    if (!camera) return false;
    auto *tilemap = m_selected->getComponent<Shit::Tilemap>();
    if (!tilemap) return false;
    auto *transform = m_selected->getComponent<Shit::TransformComponent>();
    if (!transform) return false;

    const int cols = tilemap->getGridWidth();
    const int rows = tilemap->getGridHeight();
    if (cols <= 0 || rows <= 0) return false;
    const Shit::Vector2 cell = tilemap->getTileWorldSize();
    const float cellW = (cell.x > 0.0f) ? cell.x : static_cast<float>(tilemap->getTileWidth());
    const float cellH = (cell.y > 0.0f) ? cell.y : static_cast<float>(tilemap->getTileHeight());

    // 控件坐标 → 逻辑像素 → 世界坐标 → 网格行列
    const QPointF logical = widgetToLogical(widgetPos);
    const Shit::Vector2 world = camera->screenToWorld({ static_cast<float>(logical.x()),
                                                        static_cast<float>(logical.y()) });
    const Shit::Vector2 origin = transform->getPosition();
    const int col = static_cast<int>(std::floor((world.x - origin.x) / cellW));
    const int row = static_cast<int>(std::floor((world.y - origin.y) / cellH));

    if (col < 0 || row < 0 || col >= cols || row >= rows) return false;

    // 未在瓦片面板选择画笔（-2）时禁止刷图，避免误刷；擦除由 m_paintErasing 决定
    if (!m_paintErasing && m_paintTileId == -2)
        return false;

    const int tileId = m_paintErasing ? -1 : m_paintTileId;
    if (tilemap->getTile(col, row) != tileId) {
        tilemap->setTile(col, row, tileId);
        update();
    }
    return true;
}

bool Viewport::colliderHandleGeom(QPointF &center, float &rotRad, float &pixelScale) const
{
    if (!m_selected || !m_editScene || m_frame.isNull() || m_drawRect.isEmpty())
        return false;
    // 播放中游戏逻辑可能销毁了选中的对象：不在场景中 → 视为未选中（防悬垂解引用）
    if (!m_editScene->containsGameObject(m_selected)) return false;
    auto *camera = editorCamera();
    auto *transform = m_selected->getComponent<Shit::TransformComponent>();
    if (!camera || !transform) return false;

    const Shit::Vector2 pos = transform->getPosition();
    const Shit::Vector2 sp = camera->worldToScreen(pos);
    center = logicalToWidget(sp.x, sp.y);
    rotRad = transform->getRotation() * 3.14159265f / 180.0f;
    // 控件缩放 × 相机 PPU(含 zoom)：世界像素→控件像素的完整换算
    const float basePs = static_cast<float>(m_drawRect.width()) / std::max(1, m_frame.width());
    pixelScale = basePs * camera->getPixelPerUnit();
    return true;
}

/// 碰撞体手柄：局部坐标 → 控件坐标（对象旋转 + 中心平移 + letterbox 像素比例）
static QPointF colliderLocalToScreen(const QPointF &center, float rotRad, float pixelScale,
                                     const QPointF &local)
{
    const float c = std::cos(rotRad), s = std::sin(rotRad);
    return QPointF(center.x() + (local.x() * c - local.y() * s) * pixelScale,
                   center.y() + (local.x() * s + local.y() * c) * pixelScale);
}

void Viewport::drawPhysicsDebug(QPainter &painter)
{
    if (!m_showColliders || !m_editScene || m_frame.isNull()) return;
    auto *camera = editorCamera();
    if (!camera) return;

    painter.setBrush(Qt::NoBrush);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 逻辑/世界像素 → 控件像素缩放（letterbox 等比，两轴同比例）
    const float px = std::max(1, m_frame.width());
    const float pixelScale = static_cast<float>(m_drawRect.width()) / px;
    // 相机每逻辑单位像素数（含 zoom 缩放）：worldToScreen 内部用它做位置映射，
    // 碰撞箱尺寸也必须乘它，否则 zoom≠1 时显示与实际碰撞体大小不一致
    const float ppu = camera->getPixelPerUnit();

    // 冲突体轮廓按刚体类型着色（对齐 Unity Gizmo 惯例）：
    // Dynamic 绿 / Kinematic 黄 / Static 蓝灰；未挂刚体暗灰（无物理效果，只是占位形状）
    for (auto &go : m_editScene->getGameObjects()) {
        auto *transform = go->getComponent<Shit::TransformComponent>();
        if (!transform) continue;
        if (!go->getComponent<Shit::BoxCollider2D>() && !go->getComponent<Shit::CircleCollider2D>()) continue;

        QColor color(150, 160, 175, 140);
        if (auto *body = go->getComponent<Shit::RigidBody2D>()) {
            switch (body->getBodyType()) {
                case Shit::RigidBody2D::Type::Dynamic:   color = QColor(90, 220, 130, 185); break;
                case Shit::RigidBody2D::Type::Kinematic: color = QColor(235, 200, 90, 185); break;
                case Shit::RigidBody2D::Type::Static:    color = QColor(110, 165, 210, 165); break;
            }
        }
        painter.setPen(QPen(color, 1));

        const Shit::Vector2 pos = transform->getPosition();
        const Shit::Vector2 sp = camera->worldToScreen(pos);
        const QPointF c = logicalToWidget(sp.x, sp.y);

if (auto *box = go->getComponent<Shit::BoxCollider2D>()) {
	            const Shit::Vector2 size = box->getSize();
	            painter.save();
	            painter.translate(c);
	            // 物理形状不随 Transform.scale 缩放（与引擎一致），旋转跟随对象
	            painter.rotate(transform->getRotation());
	            // 尺寸 = 世界像素 × PPU(含 zoom) × 控件缩放，与 worldToScreen 的映射一致
	            const float sx = size.x * ppu * pixelScale;
	            const float sy = size.y * ppu * pixelScale;
	            painter.drawRect(QRectF(-sx * 0.5f, -sy * 0.5f, sx, sy));
	            painter.restore();
	        }
	        if (auto *circle = go->getComponent<Shit::CircleCollider2D>()) {
	            const float r = circle->getRadius() * ppu * pixelScale;
	            painter.drawEllipse(c, r, r);
	        }
    }

    // —— P26：关节可视化（青色连接线 + 锚点圆点）——
    // 遍历场景中所有 Joint2D：连接本对象（bodyA）与引用刚体（bodyB），
    // 锚点处画青色圆点，线为青色虚线，便于编辑/调试关节约束。
    painter.setPen(QPen(QColor(60, 200, 230, 200), 2, Qt::DashLine));
    for (auto &go : m_editScene->getGameObjects()) {
        auto *joint = go->getComponent<Shit::Joint2D>();
        if (!joint) continue;
        auto *transformA = go->getComponent<Shit::TransformComponent>();
        if (!transformA) continue;

        // bodyA 世界锚点（未显式设置时默认 = bodyA 位置；可视化为约束点）
        const Shit::Vector2 anchor = joint->getAnchor();
        auto *bodyB = joint->getConnectedBody();
        Shit::Vector2 pA = transformA->getPosition();
        Shit::Vector2 pB = pA;   // 无目标时退化到同点
        if (bodyB && bodyB->getOwner()) {
            if (auto *tfB = bodyB->getOwner()->getComponent<Shit::TransformComponent>()) {
                pB = tfB->getPosition();
            }
        }
        if (anchor != Shit::Vector2(0, 0)) pA = anchor;

        const Shit::Vector2 spA = camera->worldToScreen(pA);
        const Shit::Vector2 spB = camera->worldToScreen(pB);
        const QPointF cA = logicalToWidget(spA.x, spA.y);
        const QPointF cB = logicalToWidget(spB.x, spB.y);

        painter.drawLine(cA, cB);
        // 锚点圆点
        painter.setBrush(QColor(60, 200, 230, 230));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(cA, 4, 4);
        painter.setPen(QPen(QColor(60, 200, 230, 200), 2, Qt::DashLine));
    }

    // —— P25b：选中碰撞体的编辑手柄（黄色高亮轮廓 + 白色尺寸/半径块）——
    QPointF selCenter;
    float selRot = 0.0f;
    float selPs = 1.0f;
    if (!colliderHandleGeom(selCenter, selRot, selPs)) return;
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::NoBrush);
    if (auto *box = m_selected->getComponent<Shit::BoxCollider2D>()) {
        const Shit::Vector2 half = box->getSize() * 0.5f;
        const QPointF corners[4] = {
            colliderLocalToScreen(selCenter, selRot, selPs, {-half.x, -half.y}),
            colliderLocalToScreen(selCenter, selRot, selPs, { half.x, -half.y}),
            colliderLocalToScreen(selCenter, selRot, selPs, { half.x,  half.y}),
            colliderLocalToScreen(selCenter, selRot, selPs, {-half.x,  half.y}),
        };
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(255, 215, 80), 2));
        QPolygonF poly;
        poly << corners[0] << corners[1] << corners[2] << corners[3];
        painter.drawPolygon(poly);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        for (const QPointF &corner : corners)
            painter.drawRect(QRectF(corner.x() - 5, corner.y() - 5, 10, 10));
    } else if (auto *circle = m_selected->getComponent<Shit::CircleCollider2D>()) {
        const float r = circle->getRadius() * selPs;
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(255, 215, 80), 2));
        painter.drawEllipse(selCenter, r, r);
        const QPointF tip = colliderLocalToScreen(selCenter, selRot, selPs,
                                                  {circle->getRadius(), 0});
        painter.setPen(QPen(QColor(255, 215, 80), 1, Qt::DashLine));
        painter.drawLine(selCenter, tip);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.drawRect(QRectF(tip.x() - 5, tip.y() - 5, 10, 10));
    }
}

void Viewport::mousePressEvent(QMouseEvent *event)
{
    const QPoint pos = event->pos();

    // P27：瓦片刷图。选中含 Tilemap 的对象时：左键+Shift 放置画笔瓦片、右键擦除。
    // 优先于 Gizmo/碰撞体手柄与拾取（编辑锁时禁用；仅场景视图可刷，运行视口无 m_editScene）
    if (m_editEnabled && m_selected && m_editScene && !m_frame.isNull() && m_drawRect.contains(pos)) {
        if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ShiftModifier)) {
            if (m_selected->getComponent<Shit::Tilemap>()) {
                m_paintErasing = false;
                if (paintTileAt(pos)) {
                    m_drag = DragMode::PaintTiles;
                    m_dragStartWidget = pos;
                    emit gizmoDragStarted();
                    QWidget::mousePressEvent(event);
                    return;
                }
            }
        } else if (event->button() == Qt::RightButton) {
            if (m_selected->getComponent<Shit::Tilemap>()) {
                m_paintErasing = true;
                if (paintTileAt(pos)) {
                    m_drag = DragMode::PaintTiles;
                    m_dragStartWidget = pos;
                    emit gizmoDragStarted();
                    QWidget::mousePressEvent(event);
                    return;
                }
            }
        }
    }

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

if (event->button() == Qt::LeftButton && m_editEnabled && m_selected && !m_frame.isNull()
        && m_drawRect.contains(pos)) {
        // P25b：碰撞体编辑手柄命中（优先于 Gizmo——手柄目标更小，需更精确检测；
        // 与「碰撞体」开关联动：轮廓隐藏时手柄一并失活；播放态编辑锁禁用）
        if (m_editEnabled && m_showColliders) {
            QPointF c; float rotRad = 0.0f; float ps = 1.0f;
            if (colliderHandleGeom(c, rotRad, ps)) {
                const QPointF m(pos.x(), pos.y());
                if (auto *box = m_selected->getComponent<Shit::BoxCollider2D>()) {
                    const Shit::Vector2 half = box->getSize() * 0.5f;
                    for (int i = 0; i < 4; ++i) {
                        const QPointF corner = colliderLocalToScreen(c, rotRad, ps,
                            {(i & 1) ? half.x : -half.x, (i & 2) ? half.y : -half.y});
                        if (QRectF(corner.x() - 7, corner.y() - 7, 14, 14).contains(m)) {
                            m_drag = DragMode::ColliderBox;
                            m_dragCorner = i;
                            m_dragStartSizeX = box->getSize().x;
                            m_dragStartSizeY = box->getSize().y;
                            m_dragPixelScale = ps;
                            m_dragStartWidget = pos;
                            emit gizmoDragStarted();
                            QWidget::mousePressEvent(event);
                            return;
                        }
                    }
                } else if (auto *circle = m_selected->getComponent<Shit::CircleCollider2D>()) {
                    const QPointF tip = colliderLocalToScreen(c, rotRad, ps,
                                                              {circle->getRadius(), 0});
                    if (QRectF(tip.x() - 7, tip.y() - 7, 14, 14).contains(m)) {
                        m_drag = DragMode::ColliderCircle;
                        m_dragStartWidget = pos;
                        m_dragPixelScale = ps;
                        emit gizmoDragStarted();
                        QWidget::mousePressEvent(event);
                        return;
                    }
                }
            }
        }

        // Gizmo 手柄命中检测（按模式分派）
        auto *camera = editorCamera();
        auto *transform = m_selected->getComponent<Shit::TransformComponent>();
        if (camera && transform) {
            const Shit::Vector2 sp = camera->worldToScreen(transform->getPosition());
            const QPointF p = logicalToWidget(sp.x, sp.y);
            const QPointF m(pos.x(), pos.y());
            const int len = 30;

            if (m_gizmoMode == GizmoMode::Rotate) {
                // 旋转：圆环内 20~60px 命中
                const QPointF d = m - p;
                const float dist = std::hypot(d.x(), d.y());
                if (dist >= 20.0f && dist <= 60.0f) {
                    m_drag = DragMode::Rotate;
                    m_dragStartWidget = pos;
                    m_dragStartRotation = transform->getRotation();
                    m_dragStartSnapped = std::atan2(d.y(), d.x()) * 180.0f / 3.14159265f;
                    emit gizmoDragStarted();
                    QWidget::mousePressEvent(event);
                    return;
                }
            } else if (m_gizmoMode == GizmoMode::Scale) {
                // 缩放：X/Y 端方块命中
                const QPointF px(p.x() + len, p.y());
                const QPointF py(p.x(), p.y() + len);
                if (QRectF(px.x() - 6.0, px.y() - 6.0, 12.0, 12.0).contains(m)) {
                    m_drag = DragMode::ScaleX;
                    m_dragStartWidget = pos;
                    m_dragStartScaleX = transform->getScale().x;
                    m_dragStartScaleY = transform->getScale().y;
                    emit gizmoDragStarted();
                    QWidget::mousePressEvent(event);
                    return;
                }
                if (QRectF(py.x() - 6.0, py.y() - 6.0, 12.0, 12.0).contains(m)) {
                    m_drag = DragMode::ScaleY;
                    m_dragStartWidget = pos;
                    m_dragStartScaleX = transform->getScale().x;
                    m_dragStartScaleY = transform->getScale().y;
                    emit gizmoDragStarted();
                    QWidget::mousePressEvent(event);
                    return;
                }
            } else {
                // Move：X/Y 轴手柄单轴拖；中心方块整体拖（P11 遗留：此分支此前缺失）
                const QPointF px(p.x() + len, p.y());
                const QPointF py(p.x(), p.y() + len);
                const float sx = transform->getPosition().x;
                const float sy = transform->getPosition().y;
                if (QRectF(px.x() - 6.0, px.y() - 6.0, 12.0, 12.0).contains(m)) {
                    m_drag = DragMode::GizmoX;
                    m_dragStartWidget = pos;
                    m_dragStartPosX = sx;
                    m_dragStartPosY = sy;
                    emit gizmoDragStarted();
                    QWidget::mousePressEvent(event);
                    return;
                }
                if (QRectF(py.x() - 6.0, py.y() - 6.0, 12.0, 12.0).contains(m)) {
                    m_drag = DragMode::GizmoY;
                    m_dragStartWidget = pos;
                    m_dragStartPosX = sx;
                    m_dragStartPosY = sy;
                    emit gizmoDragStarted();
                    QWidget::mousePressEvent(event);
                    return;
                }
                if (QRectF(p.x() - 6.0, p.y() - 6.0, 12.0, 12.0).contains(m)) {
                    m_drag = DragMode::Move;
                    m_dragStartWidget = pos;
                    m_dragStartPosX = sx;
                    m_dragStartPosY = sy;
                    emit gizmoDragStarted();
                    QWidget::mousePressEvent(event);
                    return;
                }
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
	            // Ctrl：10px 网格吸附
	            if (event->modifiers() & Qt::ControlModifier) {
	                transform->setPosition({ std::round(nx / 10.0f) * 10.0f, std::round(ny / 10.0f) * 10.0f });
	            } else {
	                transform->setPosition({ nx, ny });
	            }
	            // 同步 Box2D 刚体位置（防止 Static 刚体碰撞箱显示与实际错位）
	            if (auto *body = m_selected ? m_selected->getComponent<Shit::RigidBody2D>() : nullptr) {
	                body->setTransform(transform->getPosition(), transform->getRotation());
	            }
	            update();
	        }
} else if (m_drag == DragMode::Move) {
	        // 中心方块：整体拖动（X+Y），Ctrl 10px 网格吸附
	        if (auto *transform = m_selected ? m_selected->getComponent<Shit::TransformComponent>() : nullptr) {
	            const float nx = m_dragStartPosX + worldDx;
	            const float ny = m_dragStartPosY + worldDy;
	            if (event->modifiers() & Qt::ControlModifier) {
	                transform->setPosition({ std::round(nx / 10.0f) * 10.0f, std::round(ny / 10.0f) * 10.0f });
	            } else {
	                transform->setPosition({ nx, ny });
	            }
	            // 同步 Box2D 刚体位置（防止 Static 刚体碰撞箱显示与实际错位）
	            if (auto *body = m_selected ? m_selected->getComponent<Shit::RigidBody2D>() : nullptr) {
	                body->setTransform(transform->getPosition(), transform->getRotation());
	            }
	            update();
	        }
} else if (m_drag == DragMode::Rotate) {
	        if (auto *transform = m_selected ? m_selected->getComponent<Shit::TransformComponent>() : nullptr) {
	            // 以对象屏幕位置为中心计算当前角度
	            const Shit::Vector2 sp = camera->worldToScreen(transform->getPosition());
	            const QPoint c = logicalToWidget(sp.x, sp.y);
	            const float cur = std::atan2(static_cast<float>(event->pos().y() - c.y()),
	                                         static_cast<float>(event->pos().x() - c.x()))
	                              * 180.0f / 3.14159265f;
	            const float step = (event->modifiers() & Qt::ControlModifier) ? 5.0f : 15.0f; // 15° 量子化，Ctrl 5°
	            const float snapped = std::round(cur / step) * step;
	            float delta = snapped - m_dragStartSnapped;      // 相对起始角的增量（wrap ±180）
	            if (delta > 180.0f) delta -= 360.0f;
	            else if (delta < -180.0f) delta += 360.0f;
	            transform->setRotation(m_dragStartRotation + delta);
	            // 同步 Box2D 刚体旋转（防止 Static 刚体碰撞箱显示与实际错位）
	            if (auto *body = m_selected ? m_selected->getComponent<Shit::RigidBody2D>() : nullptr) {
	                body->setTransform(transform->getPosition(), transform->getRotation());
	            }
	            update();
	        }
    } else if (m_drag == DragMode::ScaleX || m_drag == DragMode::ScaleY) {
        if (auto *transform = m_selected ? m_selected->getComponent<Shit::TransformComponent>() : nullptr) {
            // 拖轴端方块：位移 → 线性缩放因子（每 50 世界单位 × 2 上下）
            const float base = (m_drag == DragMode::ScaleX) ? worldDx : worldDy;
            float factor = 1.0f + base / 50.0f;
            if (factor < 0.05f) factor = 0.05f;
            if (event->modifiers() & Qt::ControlModifier) factor = std::round(factor / 0.1f) * 0.1f;
            Shit::Vector2 s = transform->getScale();
            if (m_drag == DragMode::ScaleX) s.x = m_dragStartScaleX * factor;
            else s.y = m_dragStartScaleY * factor;
            transform->setScale(s);
            update();
        }
    } else if (m_drag == DragMode::ColliderBox) {
        // 拖角：屏幕位移 → 逆旋转到对象局部系 → 尺寸双边伸缩（角移动 = 半宽两倍）
        if (auto *box = m_selected ? m_selected->getComponent<Shit::BoxCollider2D>() : nullptr) {
            if (auto *transform = m_selected->getComponent<Shit::TransformComponent>()) {
                const float rotRad = transform->getRotation() * 3.14159265f / 180.0f;
                const float dxw = static_cast<float>(event->pos().x() - m_dragStartWidget.x());
                const float dyw = static_cast<float>(event->pos().y() - m_dragStartWidget.y());
                const float cosr = std::cos(rotRad), sinr = std::sin(rotRad);
                const float ldx = (dxw * cosr + dyw * sinr) / m_dragPixelScale;
                const float ldy = (-dxw * sinr + dyw * cosr) / m_dragPixelScale;
                float sx = m_dragStartSizeX + 2.0f * ldx * ((m_dragCorner & 1) ? 1.0f : -1.0f);
                float sy = m_dragStartSizeY + 2.0f * ldy * ((m_dragCorner & 2) ? 1.0f : -1.0f);
                if (event->modifiers() & Qt::ControlModifier) {
                    sx = std::round(sx / 4.0f) * 4.0f;
                    sy = std::round(sy / 4.0f) * 4.0f;
                }
                box->setSize({ std::max(2.0f, sx), std::max(2.0f, sy) });
                update();
            }
        }
    } else if (m_drag == DragMode::ColliderCircle) {
        // 半径 = 当前鼠标到圆心的控件距离 / 像素比例（直观地"拖到鼠标处"）
        if (auto *circle = m_selected ? m_selected->getComponent<Shit::CircleCollider2D>() : nullptr) {
            QPointF c; float rotRad = 0.0f; float ps = 1.0f;
            if (colliderHandleGeom(c, rotRad, ps)) {
                const QPointF m(event->pos());
                const float dx = static_cast<float>(m.x() - c.x());
                const float dy = static_cast<float>(m.y() - c.y());
                float r = std::hypot(dx, dy) / m_dragPixelScale;
                if (event->modifiers() & Qt::ControlModifier)
                    r = std::round(r / 4.0f) * 4.0f;
                circle->setRadius(std::max(2.0f, r));
                update();
            }
        }
    } else if (m_drag == DragMode::Pan) {
        if (auto *t = camera->getOwner()->getComponent<Shit::TransformComponent>()) {
            t->setPosition({ m_panStartCamX - worldDx, m_panStartCamY - worldDy });
            update();
        }
    } else if (m_drag == DragMode::PaintTiles) {
        // 瓦片连续刷：按住拖拽经过的格子逐个放置/擦除
        if (m_selected && m_selected->getComponent<Shit::Tilemap>()) {
            paintTileAt(event->pos());
        }
    }
    QWidget::mouseMoveEvent(event);
}

void Viewport::mouseReleaseEvent(QMouseEvent *event)
{
    // 只有 Gizmo/碰撞体拖拽（对象字段已写回）才算编辑；相机平移/缩放不入库，不置 dirty
    const bool wasGizmoDrag = (m_drag == DragMode::GizmoX || m_drag == DragMode::GizmoY
                            || m_drag == DragMode::Move
                            || m_drag == DragMode::Rotate || m_drag == DragMode::ScaleX
                            || m_drag == DragMode::ScaleY
                            || m_drag == DragMode::ColliderBox
                            || m_drag == DragMode::ColliderCircle
                            || m_drag == DragMode::PaintTiles);
    m_drag = DragMode::None;
    if (wasGizmoDrag)
        emit gizmoDragFinished();
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

void Viewport::dragEnterEvent(QDragEnterEvent *event)
{
    QString path;
    if (firstImageFile(event->mimeData(), path) || firstPrefabFile(event->mimeData(), path)) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void Viewport::dragMoveEvent(QDragMoveEvent *event)
{
    QString path;
    if (firstImageFile(event->mimeData(), path) || firstPrefabFile(event->mimeData(), path)) {
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void Viewport::dropEvent(QDropEvent *event)
{
    QString path;
    const bool isPrefab = firstPrefabFile(event->mimeData(), path);
    if (!isPrefab && !firstImageFile(event->mimeData(), path)) {
        event->ignore();
        return;
    }

    // 落点（控件坐标）→ 逻辑像素坐标（与拾取同变换）
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPoint pos = event->position().toPoint();
#else
    const QPoint pos = event->pos();
#endif
    const QPointF logical = widgetToLogical(pos);
    event->acceptProposedAction();
    if (isPrefab)
        emit prefabDropped(path, static_cast<float>(logical.x()), static_cast<float>(logical.y()));
    else
        emit assetDropped(path, static_cast<float>(logical.x()), static_cast<float>(logical.y()));
}
