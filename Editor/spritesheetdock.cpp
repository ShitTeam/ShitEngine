#include "spritesheetdock.h"

#include "assetpaths.h"

#include <nlohmann/json.hpp>

#include <QApplication>
#include <QDrag>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QToolButton>

#include <fstream>

SpriteSheetDock::SpriteSheetDock(QWidget *parent)
    : QWidget(parent)
    , m_scroll(new QScrollArea(this))
    , m_gridHost(new QWidget)
    , m_hint(new QLabel(tr("在资源面板双击 .sprite 文件打开精灵表"), this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_hint->setAlignment(Qt::AlignCenter);
    m_hint->setWordWrap(true);
    m_hint->setStyleSheet("color: #7a8a9a; padding: 20px;");
    layout->addWidget(m_hint);

    m_gridHost->setContentsMargins(4, 4, 4, 4);
    m_scroll->setWidget(m_gridHost);
    m_scroll->setWidgetResizable(true);
    layout->addWidget(m_scroll);
    m_scroll->hide();
}

void SpriteSheetDock::setProjectRoot(const QString &root)
{
    m_projectRoot = root;
}

QString SpriteSheetDock::resolveTexturePath(const QString &rel) const
{
    // 委托统一路径服务（项目根由全局注入，与各 Dock 一致）
    return AssetPaths::toAbsolute(rel);
}

void SpriteSheetDock::openSpriteFile(const QString &path)
{
    if (path.isEmpty()) return;

    std::ifstream f(path.toStdString());
    if (!f.is_open()) return;

    nlohmann::json j;
    try { f >> j; } catch (...) { return; }

    m_spriteFilePath = path;
    m_textureRelPath = QString::fromStdString(j.value("texture", ""));
    m_params.rows = j.value("rows", 1);
    m_params.cols = j.value("cols", 1);
    m_params.frameWidth = j.value("frameWidth", 0.0f);
    m_params.frameHeight = j.value("frameHeight", 0.0f);
    m_params.margin = j.value("margin", 0.0f);
    m_params.spacing = j.value("spacing", 0.0f);

    // 若帧宽高为 0，按图片尺寸/行列自动计算
    const QString texPath = resolveTexturePath(m_textureRelPath);
    m_texture = QImage(texPath);
    if (m_texture.isNull()) {
        m_hint->setText(tr("无法加载纹理：\n%1").arg(texPath));
        m_hint->show();
        m_gridHost->hide();
        return;
    }
    if (m_params.frameWidth <= 0)
        m_params.frameWidth = static_cast<float>(m_texture.width()) / m_params.cols;
    if (m_params.frameHeight <= 0)
        m_params.frameHeight = static_cast<float>(m_texture.height()) / m_params.rows;

    m_selectedFrame = -1;
    rebuildGrid();

    m_hint->hide();
    m_scroll->show();
}

void SpriteSheetDock::rebuildGrid()
{
    // 清空旧网格
    delete m_gridHost;
    m_gridHost = new QWidget;
    m_gridHost->setContentsMargins(4, 4, 4, 4);

    const int frameCount = m_params.rows * m_params.cols;
    m_thumbs.clear();
    m_thumbs.resize(frameCount);

    auto *grid = new QGridLayout(m_gridHost);
    grid->setSpacing(4);
    grid->setContentsMargins(0, 0, 0, 0);

    const int thumbSize = 56;

    for (int i = 0; i < frameCount; ++i) {
        m_thumbs[i] = thumbFor(i);

        auto *btn = new QToolButton(m_gridHost);
        btn->setIconSize(QSize(thumbSize - 4, thumbSize - 4));
        btn->setIcon(QIcon(m_thumbs[i]));
        btn->setToolTip(tr("帧 %1").arg(i));
        btn->setFixedSize(thumbSize, thumbSize);
        btn->setCheckable(true);

        const int row = i / m_params.cols;
        const int col = i % m_params.cols;
        grid->addWidget(btn, row, col);

        connect(btn, &QToolButton::clicked, this, [this, i] { onFrameClicked(i); });
    }

    m_scroll->setWidget(m_gridHost);
}

QPixmap SpriteSheetDock::thumbFor(int frameIndex) const
{
    if (m_texture.isNull() || frameIndex < 0 || frameIndex >= m_params.rows * m_params.cols)
        return {};

    const int col = frameIndex % m_params.cols;
    const int row = frameIndex / m_params.cols;
    const int x = static_cast<int>(m_params.margin + col * (m_params.frameWidth + m_params.spacing));
    const int y = static_cast<int>(m_params.margin + row * (m_params.frameHeight + m_params.spacing));
    const int w = static_cast<int>(m_params.frameWidth);
    const int h = static_cast<int>(m_params.frameHeight);

    QRect srcRect(x, y, w, h);
    if (!srcRect.intersects(m_texture.rect())) return {};
    return QPixmap::fromImage(m_texture.copy(srcRect)).scaled(52, 52, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void SpriteSheetDock::onFrameClicked(int frameIndex)
{
    m_selectedFrame = (m_selectedFrame == frameIndex) ? -1 : frameIndex;
    updateFrameSelection();
}

void SpriteSheetDock::updateFrameSelection()
{
    auto *grid = qobject_cast<QGridLayout *>(m_gridHost->layout());
    if (!grid) return;
    const int frameCount = m_params.rows * m_params.cols;
    for (int i = 0; i < frameCount; ++i) {
        auto *item = grid->itemAtPosition(i / m_params.cols, i % m_params.cols);
        if (auto *btn = item ? qobject_cast<QToolButton *>(item->widget()) : nullptr)
            btn->setChecked(i == m_selectedFrame);
    }
}

// ── 拖拽支持：选中帧后鼠标拖动 → 自定义 MIME 供 Animation 窗口接收 ──

void SpriteSheetDock::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) { QWidget::mousePressEvent(event); return; }
    m_pressPos = event->pos();
    m_dragging = false;
}

void SpriteSheetDock::mouseMoveEvent(QMouseEvent *event)
{
    if (m_selectedFrame < 0) { QWidget::mouseMoveEvent(event); return; }
    if (!m_dragging && (event->pos() - m_pressPos).manhattanLength() >= QApplication::startDragDistance()) {
        m_dragging = true;

        QMimeData *mime = new QMimeData;
        // 构造 JSON 载荷：精灵表参数 + 帧索引
        nlohmann::json payload;
        payload["texture"] = m_textureRelPath.toStdString();
        payload["rows"] = m_params.rows;
        payload["cols"] = m_params.cols;
        payload["frameWidth"] = m_params.frameWidth;
        payload["frameHeight"] = m_params.frameHeight;
        payload["margin"] = m_params.margin;
        payload["spacing"] = m_params.spacing;
        payload["frameIndex"] = m_selectedFrame;
        mime->setData(kSpriteFrameMime, QByteArray::fromStdString(payload.dump()));

        auto *drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->setPixmap(m_thumbs.value(m_selectedFrame, QPixmap()));
        drag->exec(Qt::CopyAction);
        m_dragging = false;
    }
}
