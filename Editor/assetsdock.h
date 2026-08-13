#ifndef ASSETSDOCK_H
#define ASSETSDOCK_H

#include <QWidget>

class QListView;
class QModelIndex;
class QFileSystemModel;
class QSortFilterProxyModel;
class QTreeView;

/// Unity 风格资源窗口（P30）：双栏布局——左侧文件夹树 + 右侧文件图标网格。
/// - 根目录为整个项目根（Assets/ Scenes/ Scripts/ 等，生成目录 bin/build/.shitengine 隐藏）
/// - 网格大图标展示：图片显示缩略图，其余用 OS 图标；网格里目录与文件混排
/// - 树选中目录 → 网格切换；网格双击目录 → 进入并联动树高亮
/// - 双击 .scene/.prefab/.anim 触发既有信号；拖拽图片/预置到场景视口创建精灵
class AssetsDock : public QWidget
{
    Q_OBJECT
public:
    explicit AssetsDock(QWidget *parent = nullptr);

    QString projectDir() const { return m_projectDir; }

signals:
    /// 双击 .scene 文件请求打开
    void sceneOpenRequested(const QString &path);
    /// 双击 .prefab 文件请求实例化进当前场景（P25c）
    void prefabOpenRequested(const QString &path);
    /// 双击 .anim 文件请求编辑动画剪辑（P28：应用于选中对象的 Animator 状态）
    void animOpenRequested(const QString &path);

public slots:
    /// 绑定根目录（P14：打开项目时设为整个项目根；仍可经由树浏览其它目录）
    void applyProjectDir(const QString &dir);

private slots:
    /// 网格双击：目录→进入并联动树；.scene/.prefab/.anim→请求打开
    void onGridDoubleClicked(const QModelIndex &index);

private:
    /// 把网格 root index 切到 dir，并同步树状态（加载时 / 网格进入目录时调用）
    void navigateTo(const QString &dir);

    QTreeView *m_treeView = nullptr;           ///< 左侧文件夹树（仅目录）
    QListView *m_gridView = nullptr;           ///< 右侧文件网格（目录+文件混排）
    QFileSystemModel *m_model = nullptr;       ///< 共享文件系统模型（缩略图版）
    QSortFilterProxyModel *m_dirFilter = nullptr;   ///< 树：仅目录
    QSortFilterProxyModel *m_assetFilter = nullptr; ///< 网格：目录 + 允许素材文件
    QString m_projectDir;
};

#endif // ASSETSDOCK_H