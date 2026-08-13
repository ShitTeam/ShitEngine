#ifndef DOPESHEETWIDGET_H
#define DOPESHEETWIDGET_H

#include <QWidget>

#include <vector>

class QPixmap;
class QPaintEvent;
class QMouseEvent;
class QWheelEvent;

namespace Shit { class AnimationClip; }

/// 帧动画 Dope Sheet 时间轴（P29）：Unity 风格的水平帧轨道。
///
/// 直接编辑一份 AnimationClip：
/// - 每个帧 = 一个块，块宽 ∝ 该帧时长（秒），块内显示精灵表缩略图 + 序号
/// - 交互：
///   * 拖拽块水平移动 → 调整帧顺序（释放时按落点重新排序）
///   * 拖拽块右缘 → 拉伸/压缩该帧时长（逐帧独立时长 frameDurations）
///   * 双击块 → 删除
///   * 点击块 → 选中（selectedFrameChanged）
///   * 播放时红色播放头随 m_playTime 移动
/// - 播放头时间由外部 setPlayTime() 驱动（AnimationDock 预览）
/// - 数据改动经 clipChanged() 通知外部（写回/保存/刷新）
class DopeSheetWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DopeSheetWidget(QWidget *parent = nullptr);

    /// 绑定/解绑要编辑的剪辑（解绑传 nullptr；剪辑归外部所有，本控件不删除）
    void setClip(Shit::AnimationClip *clip);
    /// 设置帧缩略图（外部从精灵表纹理生成；索引 = 全局帧 id，未生成时为 null）
    void setFramePixmap(int frameId, const QPixmap &pixmap);
    void clearPixmaps();
    /// 设置/清除播放头时间（秒）；<0 表示不显示播放头
    void setPlayTime(float seconds);
    /// 选择某一帧块（按帧索引定位，用于程序选中）
    void selectFrameAt(int index);
    /// 当前选中帧块索引；-1 = 无
    int selectedFrame() const { return m_selected; }

signals:
    /// 帧序列/时长被修改（排序、拉伸、删除、追加都由外部走 addFrames 触发此信号）
    void clipChanged();
    /// 选中帧块改变
    void selectedFrameChanged(int index);
    /// 双击某个帧块（参数=该帧全局帧 id；供外部从网格联动选中）
    void frameActivated(int frameId);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    struct Block {
        int index = 0;      ///< 在帧序列中的位置
        int frameId = -1;   ///< 精灵表全局帧 id
        float duration = 0.1f;
        QRectF rect;        ///< 布局后的块矩形（缓存用于命中）
        QPixmap *pixmap = nullptr;
    };

    void relayout();
    int blockAt(const QPointF &pos) const;   ///< 命中的块索引；-1 无
    int blockAtHandle(const QPointF &pos) const; ///< 命中右缘拉伸手柄的块；-1 无
    void applyReorder(int from, int to);     ///< 把 from 位置的帧移动到 to 槽位（同步 frames/frameDurations）
    void refreshBlocks();
    void emitChanged();

    Shit::AnimationClip *m_clip = nullptr;
    std::vector<Block> m_blocks;
    // 缩略图缓存（外部灌入）
    std::vector<std::pair<int, QPixmap>> m_pixmaps;   ///< frameId → pixmap

    // 布局参数
    static constexpr qreal kBlockH = 56.0;
    static constexpr qreal kRowH = kBlockH + 6.0;     ///< 含行距
    static constexpr qreal kHandleW = 6.0;
    static constexpr qreal kMinDur = 0.02;

    int m_selected = -1;
    float m_playTime = -1.0f;

    // 拖拽状态
    bool m_dragging = false;
    bool m_resizing = false;
    int m_dragIndex = -1;
    QPointF m_dragStartPos;
    float m_dragStartDur = 0.0f;
};

#endif // DOPESHEETWIDGET_H
