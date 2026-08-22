#include "tilesetdock.h"

#include "assetpaths.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QPixmap>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

#include <ShitEngine/Component/Tilemap.h>
#include <ShitEngine/GameObject/GameObject.h>

namespace {

/// 把相对路径解析为可读的绝对路径（委托统一路径服务：项目根 → exe 目录
/// → 常见资源根——此前仅认 exe 目录，现随全局服务获得项目根感知）
QString resolveTexturePath(const QString &path)
{
    return AssetPaths::toAbsolute(path);
}

} // namespace

TilesetDock::TilesetDock(QWidget *parent)
    : QWidget(parent)
    , m_selectedTile(-1)
{
    // 顶部：状态提示 + 橡皮按钮
    m_hint = new QLabel(tr("未选中瓦片地图"), this);
    m_hint->setWordWrap(true);

    m_eraseBtn = new QToolButton(this);
    m_eraseBtn->setText(tr("橡皮"));
    m_eraseBtn->setToolTip(tr("擦除模式：选择后刷图即擦除（等同视口右键擦除）"));
    m_eraseBtn->setCheckable(true);
    m_eraseBtn->setChecked(false);

    auto *topRow = new QHBoxLayout;
    topRow->addWidget(m_hint, 1);
    topRow->addWidget(m_eraseBtn);

    // 中间：瓦片网格滚动区
    m_gridHost = new QWidget(this);
    m_scroll = new QScrollArea(this);
    m_scroll->setWidgetResizable(true);
    m_scroll->setWidget(m_gridHost);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(topRow);
    layout->addWidget(m_scroll, 1);

    connect(m_eraseBtn, &QToolButton::clicked, this, &TilesetDock::onEraseClicked);
}

void TilesetDock::setGameObject(Shit::GameObject *object)
{
    m_object = object;
    m_selectedTile = -1;
    m_eraseBtn->setChecked(false);
    rebuildGrid();
}

void TilesetDock::rebuildGrid()
{
    // 清理旧网格
    delete m_gridHost->layout();
    m_tileButtons.clear();

    // 未选中对象或没有 Tilemap → 占位提示
    Shit::Tilemap *tilemap = m_object ? m_object->getComponent<Shit::Tilemap>() : nullptr;
    if (!tilemap) {
        m_hint->setText(tr("未选中瓦片地图\n请在场景树/视口选中含 Tilemap 组件的对象"));
        m_hint->setVisible(true);
        m_eraseBtn->setEnabled(false);
        return;
    }

    const int tileW = tilemap->getTileWidth();
    const int tileH = tilemap->getTileHeight();
    const QString texPath = resolveTexturePath(QString::fromStdString(tilemap->getTexturePath()));
    QImage sheet(texPath);
    if (sheet.isNull() || tileW <= 0 || tileH <= 0) {
        m_hint->setText(tr("瓦片地图纹理加载失败\n%1").arg(texPath));
        m_hint->setVisible(true);
        m_eraseBtn->setEnabled(false);
        return;
    }

    const int tilesPerRow = sheet.width() / tileW;
    const int tileCount = (sheet.width() / tileW) * (sheet.height() / tileH);
    if (tilesPerRow <= 0 || tileCount <= 0) {
        m_hint->setText(tr("纹理尺寸 %1×%2 小于瓦片 %3×%4")
                            .arg(sheet.width()).arg(sheet.height()).arg(tileW).arg(tileH));
        m_hint->setVisible(true);
        m_eraseBtn->setEnabled(false);
        return;
    }

    m_hint->setText(tr("点击选择瓦片（id %1），再点同一格取消").arg(0));
    m_hint->setVisible(true);
    m_eraseBtn->setEnabled(true);

    auto *grid = new QGridLayout(m_gridHost);
    grid->setContentsMargins(4, 4, 4, 4);
    grid->setSpacing(2);

    const int maxCols = 8;
    const int previewW = qMax(24, qMin(tileW, 64));   // 缩略图不超过 64px，不小于 24px
    const int previewH = qMax(24, qMin(tileH, 64));

    for (int id = 0; id < tileCount; ++id) {
        const int sx = (id % tilesPerRow) * tileW;
        const int sy = (id / tilesPerRow) * tileH;
        QImage tileImg = sheet.copy(sx, sy, tileW, tileH)
                             .scaled(previewW, previewH, Qt::KeepAspectRatio, Qt::FastTransformation);

        auto *btn = new QToolButton(m_gridHost);
        btn->setIcon(QIcon(QPixmap::fromImage(tileImg)));
        btn->setIconSize(tileImg.size());
        btn->setToolTip(QString("瓦片 %1").arg(id));
        btn->setAutoRaise(false);
        btn->setCheckable(true);

        const int gridCol = id % maxCols;
        const int gridRow = id / maxCols;
        grid->addWidget(btn, gridRow, gridCol);

        connect(btn, &QToolButton::clicked, this, [this, id] { onTileClicked(id); });

        m_tileButtons.emplace_back(id, btn);
    }

    // 网格宿主尺寸适应内容
    m_gridHost->adjustSize();
}

void TilesetDock::onTileClicked(int tileId)
{
    // 同格再点 → 取消选择（重置为无画笔，避免误刷）
    if (m_selectedTile == tileId) {
        m_selectedTile = -1;
        m_eraseBtn->setChecked(false);
        for (auto &[id, btn] : m_tileButtons)
            btn->setChecked(false);
        emit tileSelected(-2); // -2 = 无选择（视口回退到不刷）
        return;
    }

    m_selectedTile = tileId;
    m_eraseBtn->setChecked(false);
    for (auto &[id, btn] : m_tileButtons)
        btn->setChecked(id == tileId);

    m_hint->setText(tr("画笔：瓦片 %1（左键+Shift 刷图）").arg(tileId));
    emit tileSelected(tileId);
}

void TilesetDock::onEraseClicked()
{
    m_selectedTile = -1;
    for (auto &[id, btn] : m_tileButtons)
        btn->setChecked(false);

    if (m_eraseBtn->isChecked()) {
        m_hint->setText(tr("橡皮：右键拖拽擦除（或左键+Shift 刷 -1）"));
        emit tileSelected(-1);
    } else {
        m_hint->setText(tr("点击选择瓦片，再点同一格取消"));
        emit tileSelected(-2); // 关闭橡皮 → 无选择
    }
}
