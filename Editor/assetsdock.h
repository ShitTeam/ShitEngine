#ifndef ASSETSDOCK_H
#define ASSETSDOCK_H

#include <QWidget>

class QLineEdit;
class QTreeView;
class QFileSystemModel;
class QSortFilterProxyModel;

/// 资源面板：浏览项目素材目录，过滤到引擎可用类型
/// （png/jpg/jpeg/bmp 图片、wav 音频、ttf/otf 字体、.scene 场景）。
/// 目录路径经 QSettings 持久化（"projectDir"）。
/// 行为：拖拽图片到场景视口创建精灵 / 双击 .scene 打开场景。
class AssetsDock : public QWidget
{
    Q_OBJECT
public:
    explicit AssetsDock(QWidget *parent = nullptr);

    QString projectDir() const { return m_projectDir; }

signals:
    /// 双击 .scene 文件请求打开
    void sceneOpenRequested(const QString &path);

private slots:
    void browse();
    void onDirEdited();

private:
    void applyProjectDir(const QString &dir);

    QLineEdit *m_dirEdit = nullptr;
    QTreeView *m_view = nullptr;
    QFileSystemModel *m_model = nullptr;
    QSortFilterProxyModel *m_filter = nullptr;
    QString m_projectDir;
};

#endif // ASSETSDOCK_H