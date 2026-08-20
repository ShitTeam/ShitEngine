#include "assetsdock.h"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHash>
#include <QImage>
#include <QInputDialog>
#include <QItemSelectionModel>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
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

    // P35：右键菜单（树 / 网格）——刷新 / 新建文件夹 / 重命名 / 删除 / 导入
    m_treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_gridView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_treeView, &QTreeView::customContextMenuRequested,
            this, &AssetsDock::showTreeContextMenu);
    connect(m_gridView, &QListView::customContextMenuRequested,
            this, &AssetsDock::showGridContextMenu);

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

// ═══════════════════════════════════════════════════════════════
// P35: 资产文件操作（右键菜单）
// ═══════════════════════════════════════════════════════════════

QString AssetsDock::currentDir() const
{
    const QModelIndex srcRoot = m_assetFilter->mapToSource(m_gridView->rootIndex());
    return srcRoot.isValid() ? m_model->filePath(srcRoot) : m_model->rootPath();
}

void AssetsDock::refreshAll()
{
    // QFileSystemModel 自动监视目录变更，这里强制刷新过滤（新建/删除后立即反映）
    m_model->setRootPath(m_model->rootPath());
    m_dirFilter->invalidate();
    m_assetFilter->invalidate();
}

void AssetsDock::showTreeContextMenu(const QPoint &pos)
{
    // 树右键：以树当前选中目录（或网格当前目录）为基准
    const QModelIndex idx = m_treeView->indexAt(pos);
    QString base = m_projectDir;
    if (idx.isValid()) {
        const QModelIndex src = m_dirFilter->mapToSource(idx);
        if (m_model->isDir(src)) base = m_model->filePath(src);
    } else {
        base = currentDir();
    }

    auto *menu = new QMenu(this);
    menu->addAction(tr("刷新"), this, [this] { refreshAll(); });
    menu->addAction(tr("新建文件夹…"), this, [this, base] {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("新建文件夹"),
            tr("文件夹名称："), QLineEdit::Normal, QString(), &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        const QString dir = base + "/" + name.trimmed();
        if (QDir().mkpath(dir)) {
            refreshAll();
            navigateTo(dir);
        } else {
            QMessageBox::warning(this, tr("新建文件夹"), tr("创建失败：\n%1").arg(dir));
        }
    });
    menu->addAction(tr("导入文件…"), this, [this, base] { importFilesInto(base); });
    menu->exec(m_treeView->viewport()->mapToGlobal(pos));
    delete menu;
}

void AssetsDock::showGridContextMenu(const QPoint &pos)
{
    const QModelIndex idx = m_gridView->indexAt(pos);
    const QModelIndex src = idx.isValid() ? m_assetFilter->mapToSource(idx) : QModelIndex();
    const bool isDir = src.isValid() && m_model->isDir(src);
    const bool haveItem = src.isValid();

    auto *menu = new QMenu(this);
    menu->addAction(tr("刷新"), this, [this] { refreshAll(); });
    menu->addAction(tr("新建文件夹…"), this, [this] {
        bool ok = false;
        const QString name = QInputDialog::getText(this, tr("新建文件夹"),
            tr("文件夹名称："), QLineEdit::Normal, QString(), &ok);
        if (!ok || name.trimmed().isEmpty()) return;
        const QString dir = currentDir() + "/" + name.trimmed();
        if (QDir().mkpath(dir)) {
            refreshAll();
            navigateTo(dir);
        } else {
            QMessageBox::warning(this, tr("新建文件夹"), tr("创建失败：\n%1").arg(dir));
        }
    });
    menu->addAction(tr("导入文件…"), this, [this] { importFilesInto(currentDir()); });

    if (haveItem && !isDir) {
        // 资产文件：重命名 / 删除（目录在树里操作；删除从网格也能做）
        menu->addSeparator();
        menu->addAction(tr("重命名…"), this, [this, src] { renameItem(src); });
        menu->addAction(tr("删除"), this, [this, src] { deleteItem(src); });
    }

    menu->exec(m_gridView->viewport()->mapToGlobal(pos));
    delete menu;
}

void AssetsDock::renameItem(const QModelIndex &srcIdx)
{
    const QString path = m_model->filePath(srcIdx);
    bool ok = false;
    const QString newName = QInputDialog::getText(this, tr("重命名"),
        tr("新名称："), QLineEdit::Normal, QFileInfo(path).fileName(), &ok);
    if (!ok || newName.trimmed().isEmpty() || newName.trimmed() == QFileInfo(path).fileName())
        return;
    // QFileSystemModel::setData 执行真实文件系统重命名
    if (!m_model->setData(srcIdx, newName.trimmed()))
        QMessageBox::warning(this, tr("重命名"),
            tr("重命名失败：\n%1").arg(path));
    refreshAll();
}

void AssetsDock::deleteItem(const QModelIndex &srcIdx)
{
    const QString path = m_model->filePath(srcIdx);
    const bool isDir = m_model->isDir(srcIdx);
    if (QMessageBox::question(this, tr("删除"),
            isDir ? tr("删除文件夹及其全部内容？\n%1").arg(path)
                  : tr("删除文件？\n%1").arg(path),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    if (!m_model->remove(srcIdx))
        QMessageBox::warning(this, tr("删除"),
            tr("删除失败（文件可能被占用）：\n%1").arg(path));
    refreshAll();
}

void AssetsDock::importFilesInto(const QString &destDir)
{
    const QStringList files = QFileDialog::getOpenFileNames(this, tr("导入资源"),
        m_projectDir, tr("素材文件 (*.png *.jpg *.jpeg *.bmp *.wav *.ttf *.otf *.scene *.prefab *.anim)"));
    if (files.isEmpty()) return;

    int copied = 0;
    for (const QString &f : files) {
        const QString target = destDir + "/" + QFileInfo(f).fileName();
        if (QFileInfo::exists(target)) {
            QMessageBox::warning(this, tr("导入资源"),
                tr("文件已存在，跳过：\n%1").arg(target));
            continue;
        }
        if (QFile::copy(f, target))
            ++copied;
    }
    refreshAll();
    if (copied > 0)
        navigateTo(destDir);   // 导入后定位到目标目录，新文件立即可见
}