#include "assetsdock.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHash>
#include <QImage>
#include <QItemSelectionModel>
#include <QListView>
#include <QPixmap>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTreeView>
#include <QVBoxLayout>

namespace {

/// 引擎可用素材的文件后缀白名单
const QStringList &allowedSuffixes()
{
    static const QStringList kAllowed = { "png", "jpg", "jpeg", "bmp", "wav", "ttf", "otf", "scene", "prefab", "anim" };
    return kAllowed;
}

bool isAllowedAsset(const QString &path)
{
    return allowedSuffixes().contains(QFileInfo(path).suffix().toLower());
}

bool isImageAsset(const QString &path)
{
    static const QStringList kImages = { "png", "jpg", "jpeg", "bmp" };
    return kImages.contains(QFileInfo(path).suffix().toLower());
}

/// 生成目录（Unity 里不可见的基础设施目录）——不在资源窗口中显示
bool isInfraDir(const QString &name)
{
    static const QStringList kInfra = { ".shitengine", "bin", "build", ".git", ".vs", "CMakeFiles" };
    return kInfra.contains(name) || name.startsWith('.');
}

/// 目录是否要展示：排除隐藏点目录与基础设施目录
bool dirAcceptable(const QString &name)
{
    return !isInfraDir(name);
}

/// 文件系统模型：图片文件返回缩略图（按 56×56 缩放并缓存），其余回落 OS 图标
class ThumbFileSystemModel : public QFileSystemModel
{
public:
    explicit ThumbFileSystemModel(QObject *parent = nullptr) : QFileSystemModel(parent) {}

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (role == Qt::DecorationRole && !isDir(index)) {
            const QString path = filePath(index);
            if (isImageAsset(path))
                return thumbFor(path);
        }
        return QFileSystemModel::data(index, role);
    }

private:
    QIcon thumbFor(const QString &path) const
    {
        auto it = m_thumbs.find(path);
        if (it != m_thumbs.end())
            return it.value();
        const int size = 56;   // 网格图标尺寸（与 QListView iconSize 一致）
        QPixmap pm;
        QImage img(path);
        if (!img.isNull()) pm = QPixmap::fromImage(img.scaled(size, size, Qt::KeepAspectRatio, Qt::FastTransformation));
        QIcon icon = pm.isNull() ? QFileSystemModel::iconProvider()->icon(QFileInfo(path)) : QIcon(pm);
        m_thumbs.insert(path, icon);
        return icon;
    }

    mutable QHash<QString, QIcon> m_thumbs;
};

/// 树过滤：仅显示目录（隐藏点目录与基础设施目录）
class DirFilter : public QSortFilterProxyModel
{
public:
    explicit DirFilter(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}

protected:
    bool filterAcceptsRow(int row, const QModelIndex &srcParent) const override
    {
        const QModelIndex idx = sourceModel()->index(row, 0, srcParent);
        if (!idx.isValid()) return false;
        const auto *fs = qobject_cast<QFileSystemModel *>(sourceModel());
        if (!fs || !fs->isDir(idx)) return false;
        return dirAcceptable(idx.data(Qt::DisplayRole).toString());
    }
};

/// 网格过滤：目录（隐藏点目录与基础设施）+ 允许后缀文件
class AssetFilter : public QSortFilterProxyModel
{
public:
    explicit AssetFilter(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}

protected:
    bool filterAcceptsRow(int row, const QModelIndex &srcParent) const override
    {
        const QModelIndex idx = sourceModel()->index(row, 0, srcParent);
        if (!idx.isValid()) return false;
        const auto *fs = qobject_cast<QFileSystemModel *>(sourceModel());
        if (!fs) return false;
        if (fs->isDir(idx))
            return dirAcceptable(idx.data(Qt::DisplayRole).toString());
        return isAllowedAsset(fs->filePath(idx));
    }
};

} // namespace

AssetsDock::AssetsDock(QWidget *parent)
    : QWidget(parent)
{
    // 共享文件系统模型（右侧网格展示图片缩略图）
    m_model = new ThumbFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);

    m_dirFilter = new DirFilter(this);
    m_dirFilter->setSourceModel(m_model);
    m_assetFilter = new AssetFilter(this);
    m_assetFilter->setSourceModel(m_model);

    // ── 左侧文件夹树（仅目录）──
    m_treeView = new QTreeView(this);
    m_treeView->setModel(m_dirFilter);
    m_treeView->setHeaderHidden(true);
    m_treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_treeView->setDragEnabled(true);
    m_treeView->setDragDropMode(QAbstractItemView::DragOnly);
    m_treeView->setDefaultDropAction(Qt::CopyAction);

    // ── 右侧文件网格（目录 + 素材文件混排，大图标）──
    m_gridView = new QListView(this);
    m_gridView->setModel(m_assetFilter);
    m_gridView->setViewMode(QListView::IconMode);
    m_gridView->setIconSize(QSize(56, 56));
    m_gridView->setGridSize(QSize(88, 92));
    m_gridView->setSpacing(4);
    m_gridView->setResizeMode(QListView::Adjust);
    m_gridView->setUniformItemSizes(true);
    m_gridView->setWordWrap(true);
    m_gridView->setSelectionMode(QAbstractItemView::SingleSelection);
    // 拖拽图片 / 预置到场景视口（QFileSystemModel 原生输出 text/uri-list）
    m_gridView->setDragEnabled(true);
    m_gridView->setDragDropMode(QAbstractItemView::DragOnly);
    m_gridView->setDefaultDropAction(Qt::CopyAction);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_treeView);
    splitter->addWidget(m_gridView);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({ 200, 500 });

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(splitter, 1);

    // 树选中目录 → 网格切换
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, [this](const QModelIndex &current, const QModelIndex &) {
                if (!current.isValid()) return;
                const QModelIndex src = m_dirFilter->mapToSource(current);
                if (m_model->isDir(src))
                    navigateTo(m_model->filePath(src));
            });

    // 网格双击：目录 → 进入并联动树；.scene/.prefab/.anim → 请求打开
    connect(m_gridView, &QListView::doubleClicked, this, &AssetsDock::onGridDoubleClicked);

    // 默认根目录：上次记忆 → 无则应用目录
    QSettings settings(QStringLiteral("ShitTeam"), QStringLiteral("ShitEngineEditor"));
    const QString last = settings.value("projectDir").toString();
    applyProjectDir(last.isEmpty() ? QCoreApplication::applicationDirPath() : last);
}

void AssetsDock::applyProjectDir(const QString &dir)
{
    const QString trimmed = dir.trimmed();
    if (trimmed.isEmpty() || !QDir(trimmed).exists()) return;

    m_projectDir = QDir(trimmed).absolutePath();
    m_model->setRootPath(m_projectDir);
    m_treeView->setRootIndex(m_dirFilter->mapFromSource(m_model->index(m_projectDir)));
    navigateTo(m_projectDir);

    QSettings settings(QStringLiteral("ShitTeam"), QStringLiteral("ShitEngineEditor"));
    settings.setValue("projectDir", m_projectDir);
}

void AssetsDock::navigateTo(const QString &dir)
{
    if (!QDir(dir).exists()) return;
    const QModelIndex srcIdx = m_model->index(dir);
    if (!srcIdx.isValid()) return;

    // 网格切到该目录
    m_gridView->setRootIndex(m_assetFilter->mapFromSource(srcIdx));
    m_gridView->scrollToTop();

    // 同步树：选中 + 展开到该目录（若树当前不是同一节点）
    const QModelIndex treeIdx = m_dirFilter->mapFromSource(srcIdx);
    if (m_treeView->selectionModel()->currentIndex() != treeIdx) {
        m_treeView->setCurrentIndex(treeIdx);
        m_treeView->scrollTo(treeIdx);
    }
    const QModelIndex parent = treeIdx.parent();
    if (parent.isValid()) m_treeView->expand(parent);
    m_treeView->expand(treeIdx);
}

void AssetsDock::onGridDoubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    const QModelIndex src = m_assetFilter->mapToSource(index);
    const QString path = m_model->filePath(src);
    if (m_model->isDir(src)) {
        navigateTo(path);
        return;
    }
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == QStringLiteral("scene"))
        emit sceneOpenRequested(path);
    else if (ext == QStringLiteral("prefab"))
        emit prefabOpenRequested(path);
    else if (ext == QStringLiteral("anim"))
        emit animOpenRequested(path);
}