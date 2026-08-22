#ifndef ASSETSDOCK_H
#define ASSETSDOCK_H

#include <QWidget>

class QListView;
class QModelIndex;
class QFileSystemModel;
class QSortFilterProxyModel;
class QTreeView;

/// Unity 风格资源窗口：双栏布局——左侧文件夹树 + 右侧文件图标网格。
/// - 根目录为整个项目根（Assets/ Scenes/ Scripts/ 等，生成目录 bin/build/.shitengine 隐藏）
/// - 网格大图标展示：图片显示缩略图，其余用 OS 图标；网格里目录与文件混排
/// - 树选中目录 → 网格切换；网格双击目录 → 进入并联动树高亮
/// - 双击 .scene/.prefab/.anim 触发既有信号；拖拽图片/预置到场景视口创建精灵
/// - 树/网格右键菜单——刷新 / 新建文件夹 / 重命名 / 删除 / 导入文件…
class AssetsDock : public QWidget
{
    Q_OBJECT
public:
    explicit AssetsDock(QWidget *parent = nullptr);

    QString projectDir() const { return m_projectDir; }

signals:
    /// 双击 .scene 文件请求打开
    void sceneOpenRequested(const QString &path);
    /// 双击 .prefab 文件请求实例化进当前场景
    void prefabOpenRequested(const QString &path);
    /// 双击 .anim 文件请求编辑动画剪辑（应用于选中对象的 Animator 状态）
    void animOpenRequested(const QString &path);
    /// 双击 .sprite 文件请求在精灵表视图中打开
    void spriteFileRequested(const QString &path);

public slots:
    /// 绑定根目录（打开项目时设为整个项目根；仍可经由树浏览其它目录）
    void applyProjectDir(const QString &dir);

private slots:
    /// 网格双击：目录→进入并联动树；.scene/.prefab/.anim→请求打开
    void onGridDoubleClicked(const QModelIndex &index);
    /// 右键菜单（树 / 网格）
    void showTreeContextMenu(const QPoint &pos);
    void showGridContextMenu(const QPoint &pos);

private:
    /// 把网格 root index 切到 dir，并同步树状态（加载时 / 网格进入目录时调用）
    void navigateTo(const QString &dir);

    // ── 资产文件操作 ──
    QString currentDir() const;          ///< 网格当前浏览目录（右键基准）
    void refreshAll();                   ///< 文件系统模型 + 两个过滤模型刷新
    void createFolder();                 ///< 当前目录下新建文件夹（内联命名）
    void renameItem(const QModelIndex &srcIdx);  ///< 重命名（目录或文件）
    void deleteItem(const QModelIndex &srcIdx);  ///< 删除（目录递归，先确认）
    void importFilesInto(const QString &destDir);  ///< 从文件管理器导入允许格式文件到指定目录

    QTreeView *m_treeView = nullptr;           ///< 左侧文件夹树（仅目录）
    QListView *m_gridView = nullptr;           ///< 右侧文件网格（目录+文件混排）
    QFileSystemModel *m_model = nullptr;       ///< 共享文件系统模型（缩略图版）
    QSortFilterProxyModel *m_dirFilter = nullptr;   ///< 树：仅目录
    QSortFilterProxyModel *m_assetFilter = nullptr; ///< 网格：目录 + 允许素材文件
    QString m_projectDir;
};

#endif // ASSETSDOCK_H