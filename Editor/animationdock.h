#ifndef ANIMATIONDOCK_H
#define ANIMATIONDOCK_H

#include <QWidget>

#include <ShitEngine/Animation/AnimationClip.h>

#include <vector>

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QToolButton;
class QLabel;
class QWidget;
class QImage;
class DopeSheetWidget;

/// Unity 风格 Animation 窗口（P29）：制作/编辑独立 `.anim` 帧动画资产。
///
/// - 顶部工具栏：新建 / 打开 / 保存 / 另存为 + 播放/暂停/停止 + 循环 + 当前文件名
/// - 左侧精灵表面板：纹理路径 + 网格参数（rows/cols/帧宽高），网格点选帧 → 追加进时间轴
/// - 中央 Dope Sheet 时间轴：拖块排序、拉右缘调逐帧时长、双击从时间轴移除、播放头预览
/// - 底部剪辑参数：名称、统一每帧时长
///
/// 编辑对象是内存中的 AnimationClip m_clip，保存时序列化为 .anim 文件。
class AnimationDock : public QWidget
{
    Q_OBJECT
public:
    explicit AnimationDock(QWidget *parent = nullptr);

    /// 打开指定 .anim 文件（成功返回 true；失败保留当前剪辑）
    bool openFile(const QString &path);
    /// 询问并保存当前剪辑（另存为对话框）；返回是否成功
    bool saveAs();
    /// 保存到当前路径（无路径则弹对话框）；返回是否成功
    bool save();
    /// 当前是否已打开/新建了剪辑（区别于空态）
    bool hasClip() const { return m_clipValid; }
    /// 当前文件路径
    QString filePath() const { return m_filePath; }
    /// 当前剪辑是否自上次保存后有改动
    bool isDirty() const { return m_dirty; }
    /// 标记已保存（mainwindow 保存后调用）
    void markSaved() { m_dirty = false; updateTitle(); }

signals:
    /// 剪辑数据被修改（mainwindow 接 dirty / 标题栏）
    void changed();
    /// 保存成功（mainwindow 接：同步依赖该 .anim 的 Animator 状态）
    void saved(const QString &path);

private slots:
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onPlayToggle();
    void onLoopToggled(bool on);
    void onNameEdited();
    void onUniformDurationChanged(double v);
    void onTextureEdited();
    void onGridParamChanged();
    void onFrameGridClicked(int tileId);
    void onTimelineChanged();
    void onFrameActivated(int frameId);

private:
    void updateTitle();
    void rebuildFrameGrid();
    void refreshFrameHighlights();
    void refreshClipParams();
    void loadClip(const Shit::AnimationClip &clip);
    void notifyClipChanged();
    void syncTimeline();
    /// 从精灵表纹理切帧生成缩略图灌入时间轴
    void refreshTimelinePixmaps();
    /// 播放推进（QTimer 驱动）
    void advancePlayback();
    /// 当前剪辑总时长（秒）
    float totalDuration() const;

    Shit::AnimationClip m_clip;
    bool m_clipValid = false;
    bool m_dirty = false;
    QString m_filePath;
    QImage m_sheetImage;              ///< 当前精灵表纹理缓存（用于切帧缩略图；isNull = 无）
    bool m_sheetValid = false;

    // 顶部
    QLabel *m_fileLabel = nullptr;
    QToolButton *m_playBtn = nullptr;
    QCheckBox *m_loopCheck = nullptr;

    // 精灵表面板
    QLineEdit *m_texEdit = nullptr;
    QSpinBox *m_rowsSpin = nullptr;
    QSpinBox *m_colsSpin = nullptr;
    QDoubleSpinBox *m_fwSpin = nullptr;
    QDoubleSpinBox *m_fhSpin = nullptr;
    QWidget *m_gridHost = nullptr;
    std::vector<std::pair<int, QWidget *>> m_gridButtons;  ///< frameId → 按钮（用于高亮）

    // 剪辑参数
    QLineEdit *m_nameEdit = nullptr;
    QDoubleSpinBox *m_uniformDurSpin = nullptr;

    // 时间轴
    DopeSheetWidget *m_timeline = nullptr;

    // 预览
    float m_playTime = 0.0f;
    bool m_playing = false;

    bool m_updating = false;
};

#endif // ANIMATIONDOCK_H
