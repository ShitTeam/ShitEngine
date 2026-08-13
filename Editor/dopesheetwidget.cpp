#include "dopesheetwidget.h"

#include <ShitEngine/Animation/AnimationClip.h>

#include <QColor>
#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QPolygonF>
#include <QPen>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace {
constexpr qreal kGap = 3.0;          ///< 块间距
constexpr qreal kPlayheadWidth = 2.0;
}

DopeSheetWidget::DopeSheetWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(static_cast<int>(kRowH + 8));
    setMouseTracking(true);
}

void DopeSheetWidget::setClip(Shit::AnimationClip *clip)
{
    m_clip = clip;
    m_selected = -1;
    m_playTime = -1.0f;
    m_dragging = m_resizing = false;
    refreshBlocks();
    update();
}

void DopeSheetWidget::setFramePixmap(int frameId, const QPixmap &pixmap)
{
    for (auto &p : m_pixmaps) {
        if (p.first == frameId) { p.second = pixmap; update(); return; }
    }
    m_pixmaps.emplace_back(frameId, pixmap);
    refreshBlocks();
    update();
}

void DopeSheetWidget::clearPixmaps()
{
    m_pixmaps.clear();
    refreshBlocks();
    update();
}

void DopeSheetWidget::setPlayTime(float seconds)
{
    m_playTime = seconds;
    update();
}

void DopeSheetWidget::selectFrameAt(int index)
{
    if (index >= 0 && index < static_cast<int>(m_blocks.size())) {
        m_selected = index;
    } else {
        m_selected = -1;
    }
    update();
}

void DopeSheetWidget::refreshBlocks()
{
    m_blocks.clear();
    if (!m_clip) { update(); return; }
    const int n = static_cast<int>(m_clip->frames.size());
    for (int i = 0; i < n; ++i) {
        Block b;
        b.index = i;
        b.frameId = m_clip->frames[static_cast<size_t>(i)];
        b.duration = m_clip->frameDurations.size() == m_clip->frames.size()
                         ? m_clip->frameDurations[static_cast<size_t>(i)]
                         : m_clip->duration;
        if (b.duration <= 0.0f) b.duration = 0.1f;
        for (auto &p : m_pixmaps)
            if (p.first == b.frameId) { b.pixmap = &p.second; break; }
        m_blocks.push_back(b);
    }
    relayout();
}

void DopeSheetWidget::relayout()
{
    const qreal height = kRowH;
    qreal x = kGap;
    for (auto &b : m_blocks) {
        // 块宽 = 时长映射到像素：min 20px，max 120px
        const qreal w = std::clamp(b.duration * 100.0, 20.0, 120.0);
        b.rect = QRectF(x, (height - kBlockH) / 2.0, w, kBlockH);
        x += w + kGap;
    }
    const qreal total = x;
    setMinimumWidth(static_cast<int>(std::max(400.0, total)));
    setMaximumWidth(16777215);
    updateGeometry();
}

int DopeSheetWidget::blockAt(const QPointF &pos) const
{
    for (int i = static_cast<int>(m_blocks.size()) - 1; i >= 0; --i)
        if (m_blocks[static_cast<size_t>(i)].rect.contains(pos))
            return i;
    return -1;
}

int DopeSheetWidget::blockAtHandle(const QPointF &pos) const
{
    for (int i = static_cast<int>(m_blocks.size()) - 1; i >= 0; --i) {
        const QRectF &r = m_blocks[static_cast<size_t>(i)].rect;
        const QRectF handle(r.right() - kHandleW, r.top(), kHandleW, r.height());
        if (handle.contains(pos)) return i;
    }
    return -1;
}

void DopeSheetWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(30, 34, 40));

    // 轨道背景
    const qreal top = (kRowH - kBlockH) / 2.0;
    p.fillRect(QRectF(0, top, width(), kBlockH), QColor(40, 45, 53));

    // 帧块
    for (int i = 0; i < static_cast<int>(m_blocks.size()); ++i) {
        const Block &b = m_blocks[static_cast<size_t>(i)];
        const bool sel = (i == m_selected);
        p.save();
        // 块底色
        QColor fill = sel ? QColor(70, 120, 200) : QColor(58, 66, 78);
        p.fillRect(b.rect, fill);
        // 缩略图
        if (b.pixmap && !b.pixmap->isNull()) {
            QRectF imgRect = b.rect.adjusted(2, 2, -(kHandleW + 2), -2);
            const qreal ar = static_cast<qreal>(b.pixmap->width()) / std::max(1, b.pixmap->height());
            QRectF target = imgRect;
            const qreal targetAr = imgRect.width() / std::max(1.0, imgRect.height());
            if (ar > targetAr) {
                const qreal h = imgRect.width() / ar;
                target = QRectF(imgRect.left(), imgRect.center().y() - h / 2.0, imgRect.width(), h);
            } else {
                const qreal w = imgRect.height() * ar;
                target = QRectF(imgRect.center().x() - w / 2.0, imgRect.top(), w, imgRect.height());
            }
            p.drawPixmap(target.toRect(), *b.pixmap);
        } else {
            p.setPen(QColor(200, 210, 220));
            p.setFont(QFont(QStringLiteral("Consolas"), 9));
            p.drawText(b.rect, Qt::AlignCenter, QString::number(b.frameId));
        }
        // 序号 + 时长
        p.setPen(QColor(230, 235, 240));
        p.setFont(QFont(QStringLiteral("Segoe UI"), 8));
        p.drawText(b.rect.left() + 3, b.rect.top() + 10, QString::number(i + 1));
        p.setPen(QColor(160, 170, 180));
        p.drawText(b.rect.right() - 26, b.rect.bottom() - 4, QString::number(b.duration, 'g', 2));
        // 拉伸手柄
        p.fillRect(QRectF(b.rect.right() - kHandleW, b.rect.top(), kHandleW, b.rect.height()),
                   QColor(120, 200, 120));
        p.restore();
    }

    // 空态提示
    if (m_blocks.empty()) {
        p.setPen(QColor(140, 150, 160));
        p.drawText(rect(), Qt::AlignCenter, tr("（空）从左侧精灵表点选帧加入时间轴"));
    }

    // 播放头
    if (m_playTime >= 0.0f && m_clip && !m_blocks.empty()) {
        // 由时间定位 x：累积每块时长
        qreal acc = 0.0f;
        qreal x = kGap;
        for (const auto &b : m_blocks) {
            const qreal w = b.rect.width();
            if (m_playTime >= acc && m_playTime <= acc + b.duration) {
                const qreal t = (m_playTime - acc) / std::max(1e-5f, b.duration);
                x += w * std::clamp(t, 0.0, 1.0);
                break;
            }
            acc += b.duration;
            x += w + kGap;
        }
        p.setPen(QPen(QColor(255, 80, 80), kPlayheadWidth));
        p.drawLine(QPointF(x, top - 4), QPointF(x, top + kBlockH + 4));
        // 顶部小三角
        p.setBrush(QColor(255, 80, 80));
        p.drawPolygon(QPolygonF() << QPointF(x - 4, top - 4) << QPointF(x + 4, top - 4) << QPointF(x, top + 2));
    }
}

void DopeSheetWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_clip) return;
    const QPointF pos = event->position();

    if (event->button() == Qt::LeftButton) {
        // 先测拉伸手柄
        const int h = blockAtHandle(pos);
        if (h >= 0) {
            m_resizing = true;
            m_dragIndex = h;
            m_dragStartPos = pos;
            m_dragStartDur = m_blocks[static_cast<size_t>(h)].duration;
            m_selected = h;
            emit selectedFrameChanged(h);
            update();
            return;
        }
        const int hit = blockAt(pos);
        if (hit >= 0) {
            m_dragging = true;
            m_dragIndex = hit;
            m_dragStartPos = pos;
            m_selected = hit;
            emit selectedFrameChanged(hit);
            update();
            return;
        }
        // 空白点击：取消选中
        m_selected = -1;
        emit selectedFrameChanged(-1);
        update();
    }
}

void DopeSheetWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_clip || m_dragIndex < 0) return;
    const QPointF pos = event->position();
    const qreal dx = pos.x() - m_dragStartPos.x();

    if (m_resizing) {
        // 拉伸时长：块宽 = duration*100，反向映射
        float newDur = m_dragStartDur + static_cast<float>(dx / 100.0);
        newDur = std::max(static_cast<float>(kMinDur), newDur);
        const int idx = m_dragIndex;
        // 同步到剪辑（仅当逐帧时长开启）
        if (m_clip->frameDurations.size() != m_clip->frames.size()) {
            m_clip->frameDurations.assign(m_clip->frames.size(), m_clip->duration);
        }
        m_clip->frameDurations[static_cast<size_t>(idx)] = newDur;
        refreshBlocks();
        update();
        return;
    }

    // 拖拽排序：预览跟随（仅更新选中块高亮，不实时改数据）
    m_selected = m_dragIndex;
    update();
}

void DopeSheetWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_clip || m_dragIndex < 0) return;

    if (m_resizing) {
        emitChanged();
        m_resizing = false;
        m_dragIndex = -1;
        return;
    }

    // 拖拽排序落点：根据鼠标 x 计算新位置
    const QPointF pos = event->position();
    const int from = m_dragIndex;
    // 从块"移动后"的中心 x：当前渲染位置（未随拖拽平移） + 鼠标位移
    const qreal fromCenter = m_blocks[static_cast<size_t>(from)].rect.center().x()
                             + (pos.x() - m_dragStartPos.x());
    // 目标索引 = 中心 x 落在 fromCenter 左侧的块个数（排除自身），即插入槽位
    int to = 0;
    for (int i = 0; i < static_cast<int>(m_blocks.size()); ++i) {
        if (i == from) continue;
        if (m_blocks[static_cast<size_t>(i)].rect.center().x() < fromCenter)
            ++to;
    }
    applyReorder(from, to);

    m_dragging = false;
    m_dragIndex = -1;
    emitChanged();
    update();
}

void DopeSheetWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_clip) return;
    const int hit = blockAt(event->position());
    if (hit >= 0) {
        const int frameId = m_blocks[static_cast<size_t>(hit)].frameId;
        emit frameActivated(frameId);
        return;
    }
}

void DopeSheetWidget::wheelEvent(QWheelEvent *event)
{
    event->accept();
}

void DopeSheetWidget::applyReorder(int from, int to)
{
    if (!m_clip || from < 0 || from >= static_cast<int>(m_clip->frames.size())) return;
    const int n = static_cast<int>(m_clip->frames.size());
    // 从原位置移除
    const int frameId = m_clip->frames[static_cast<size_t>(from)];
    m_clip->frames.erase(m_clip->frames.begin() + from);
    float dur = 0.0f;
    if (m_clip->frameDurations.size() == static_cast<size_t>(n)) {
        dur = m_clip->frameDurations[static_cast<size_t>(from)];
        m_clip->frameDurations.erase(m_clip->frameDurations.begin() + from);
    }
    // 计算插入位置：to 基于未移除布局数出；移除 from 后需校正偏移
    int ins = (to > from) ? (to - 1) : to;
    ins = std::clamp(ins, 0, n - 1);
    // 插入
    m_clip->frames.insert(m_clip->frames.begin() + ins, frameId);
    if (m_clip->frameDurations.size() == static_cast<size_t>(n - 1)) {
        m_clip->frameDurations.insert(m_clip->frameDurations.begin() + ins, dur);
    }
    m_selected = ins;
    refreshBlocks();
}

void DopeSheetWidget::emitChanged()
{
    emit clipChanged();
}
