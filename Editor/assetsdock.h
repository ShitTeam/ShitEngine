#ifndef ASSETSDOCK_H
#define ASSETSDOCK_H

#include <QWidget>

class QLineEdit;
class QTreeView;
class QFileSystemModel;
class QSortFilterProxyModel;

/// 资源面板：浏览项目素材目录，过滤到引擎可用类型
/// （png/jpg/jpeg/bmp 图片、wav 音频、ttf/otf 字体、.scene 场景、.prefab 预置资产）。
/// 目录路径经 QSettings 持久化（"projectDir"）。
/// 行为：拖拽图片到场景视口创建精灵 / 双击 .scene 打开场景 / 双击 .prefab 实例化。
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
    /// 切换到指定目录（P14：打开项目时绑定项目 Assets/；仍可手动浏览其它目录）
    void applyProjectDir(const QString &dir);

private slots:
    void browse();
    void onDirEdited();

private:
    QLineEdit *m_dirEdit = nullptr;
    QTreeView *m_view = nullptr;
    QFileSystemModel *m_model = nullptr;
    QSortFilterProxyModel *m_filter = nullptr;
    QString m_projectDir;
};

#endif // ASSETSDOCK_H