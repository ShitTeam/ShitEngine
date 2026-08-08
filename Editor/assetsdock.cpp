#include "assetsdock.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>

namespace {

/// 是否为资源面板允许展示的文件后缀
bool isAllowedAsset(const QString &path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    static const QStringList kAllowed = { "png", "jpg", "jpeg", "bmp", "wav", "ttf", "otf", "scene" };
    return kAllowed.contains(ext);
}

/// 过滤代理：目录全显示；文件只显示素材类型（隐藏 . 开头的隐藏文件/点目录）
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
        if (fs->isDir(idx)) {
            const QString name = idx.data(Qt::DisplayRole).toString();
            return !name.startsWith('.');   // 隐藏目录也藏起来
        }
        return isAllowedAsset(fs->filePath(idx));
    }
};

} // namespace



AssetsDock::AssetsDock(QWidget *parent)
    : QWidget(parent)
{
    m_model = new QFileSystemModel(this);
    m_model->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);
    m_filter = new AssetFilter(this);
    m_filter->setSourceModel(m_model);

    // 顶部：当前项目目录（可编辑）+ 浏览按钮
    auto *dirRow = new QHBoxLayout;
    auto *dirLabel = new QLabel(tr("目录"), this);
    m_dirEdit = new QLineEdit(this);
    auto *browseBtn = new QToolButton(this);
    browseBtn->setText(tr("…"));
    browseBtn->setToolTip(tr("选择资源目录"));
    dirRow->addWidget(dirLabel);
    dirRow->addWidget(m_dirEdit, 1);
    dirRow->addWidget(browseBtn);

    m_view = new QTreeView(this);
    m_view->setModel(m_filter);
    m_view->setHeaderHidden(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    // 拖拽素材到场景视口
    m_view->setDragEnabled(true);
    m_view->setDragDropMode(QAbstractItemView::DragOnly);
    m_view->setDefaultDropAction(Qt::CopyAction);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addLayout(dirRow);
    layout->addWidget(m_view, 1);

    connect(browseBtn, &QToolButton::clicked, this, &AssetsDock::browse);
    connect(m_dirEdit, &QLineEdit::editingFinished, this, &AssetsDock::onDirEdited);

    // 双击 .scene → 请求打开；其它双击忽略
    connect(m_view, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        const QModelIndex src = m_filter->mapToSource(index);
        const QString path = m_model->filePath(src);
        if (QFileInfo(path).suffix().compare(QStringLiteral("scene"), Qt::CaseInsensitive) == 0)
            emit sceneOpenRequested(path);
    });

    // 默认目录：上次记忆 → 无则应用目录
    QSettings settings(QStringLiteral("ShitTeam"), QStringLiteral("ShitEngineEditor"));
    const QString last = settings.value("projectDir").toString();
    applyProjectDir(last.isEmpty() ? QCoreApplication::applicationDirPath() : last);
}

void AssetsDock::applyProjectDir(const QString &dir)
{
    const QString trimmed = dir.trimmed();
    if (trimmed.isEmpty() || !QDir(trimmed).exists()) return;

    m_projectDir = QDir(trimmed).absolutePath();
    m_dirEdit->setText(m_projectDir);
    m_model->setRootPath(m_projectDir);
    m_view->setRootIndex(m_filter->mapFromSource(m_model->index(m_projectDir)));

    QSettings settings(QStringLiteral("ShitTeam"), QStringLiteral("ShitEngineEditor"));
    settings.setValue("projectDir", m_projectDir);
}

void AssetsDock::browse()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("选择资源目录"), m_projectDir);
    if (!dir.isEmpty())
        applyProjectDir(dir);
}

void AssetsDock::onDirEdited()
{
    applyProjectDir(m_dirEdit->text());
}