#ifndef SPRITESHEETDOCK_H
#define SPRITESHEETDOCK_H

#include <QWidget>

#include <QPixmap>
#include <QPoint>
#include <QVector>

#include "spriteeditordialog.h"

class QScrollArea;
class QMouseEvent;

/// P38：精灵表视图 Dock — 显示 .sprite 文件的切帧缩略图网格，点选+拖拽到 Animation 窗口
class SpriteSheetDock : public QWidget
{
    Q_OBJECT
public:
    explicit SpriteSheetDock(QWidget *parent = nullptr);

    /// 打开 .sprite 元数据文件，加载纹理并重建缩略图网格
    void openSpriteFile(const QString &path);

    /// 设置项目根目录（.sprite 内纹理相对路径的基准）
    void setProjectRoot(const QString &root);

signals:
    /// 请求打开精灵表（资源面板双击 .sprite 触发）
    void openRequested(const QString &path);

private slots:
    void onFrameClicked(int frameIndex);

private:
    void rebuildGrid();
    void updateFrameSelection();
    QPixmap thumbFor(int frameIndex) const;
    QString resolveTexturePath(const QString &rel) const;

    // 拖拽支持
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    QScrollArea *m_scroll = nullptr;
    QWidget *m_gridHost = nullptr;
    QLabel *m_hint = nullptr;

    // 当前精灵表数据
    SpriteSheetParams m_params;
    QString m_textureRelPath;
    QString m_spriteFilePath;
    QImage m_texture;
    QVector<QPixmap> m_thumbs;

    // 选择状态
    int m_selectedFrame = -1;

    // 拖拽
    QPoint m_pressPos;
    bool m_dragging = false;

    QString m_projectRoot;
};

#endif // SPRITESHEETDOCK_H
