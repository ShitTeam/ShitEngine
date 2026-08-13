#ifndef TILESETDOCK_H
#define TILESETDOCK_H

#include <QWidget>

#include <vector>

class QLabel;
class QScrollArea;
class QToolButton;

namespace Shit { class GameObject; }

/// 瓦片选择面板（P27 增强）：选中含 Tilemap 组件对象时，读取其瓦片集纹理，
/// 按 tileWidth × tileHeight 切成网格缩略图显示。点击一格选中该瓦片 id，
/// 通过 tileSelected(int) 通知外部（视口 setPaintTileId），再点一下取消选择。
/// 另提供「橡皮」按钮（tileSelected(-1)）与「清空」说明。无 Tilemap 选中时显示占位提示。
class TilesetDock : public QWidget
{
    Q_OBJECT
public:
    explicit TilesetDock(QWidget *parent = nullptr);

    /// 设置当前选中对象（nullptr = 未选中）；检测到 Tilemap 时重建瓦片网格预览
    void setGameObject(Shit::GameObject *object);

signals:
    /// 用户点选某瓦片（tileId ≥ 0 = 瓦片索引；-1 = 橡皮擦除）；同一格再点取消（发射 -2 表示无选择）
    void tileSelected(int tileId);

private slots:
    void onTileClicked(int tileId);
    void onEraseClicked();

private:
    /// 重建瓦片网格（读选中对象 Tilemap 的纹理/瓦片尺寸）；无 Tilemap 或纹理加载失败则显示占位
    void rebuildGrid();

    QLabel *m_hint = nullptr;          ///< 顶部状态提示（当前画笔 / 橡皮）
    QScrollArea *m_scroll = nullptr;
    QWidget *m_gridHost = nullptr;     ///< 瓦片网格容器（含 flow 布局）
    QToolButton *m_eraseBtn = nullptr; ///< 橡皮工具按钮

    Shit::GameObject *m_object = nullptr; ///< 当前选中对象（仅指针，播放中可能失效，使用前校验）
    int m_selectedTile = -1;           ///< 当前选中的瓦片 id（-1 = 未选中/橡皮）

    /// 当前网格里的瓦片按钮集合（id 对应按钮）
    std::vector<std::pair<int, QToolButton *>> m_tileButtons;
};

#endif // TILESETDOCK_H
