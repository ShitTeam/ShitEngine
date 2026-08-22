#ifndef ANIMATIONDOCK_H
#define ANIMATIONDOCK_H

#include <QWidget>
#include <QImage>

#include <ShitEngine/Animation/AnimationClip.h>

#include <vector>

class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QToolButton;
class QLabel;
class QWidget;
class DopeSheetWidget;

/// Unity 风格 Animation 窗口：制作/编辑独立 `.anim` 帧动画资产。
///
/// - 顶部工具栏：新建 / 打开 / 保存 / 另存为 + 播放/暂停/停止 + 循环 + 当前文件名
/// - 中央 Dope Sheet 时间轴：拖块排序、拉右缘调逐帧时长、双击从时间轴移除、播放头预览
/// - 底部剪辑参数：名称、统一每帧时长
///
/// 编辑对象是内存中的 AnimationClip m_clip，保存时序列化为 .anim 文件。
/// 加帧路径：从精灵表 Dock（SpriteSheetDock）拖帧到本窗口时间轴（自动新建/追加剪辑）。
class AnimationDock : public QWidget
{
    Q_OBJECT
public:
    explicit AnimationDock(QWidget *parent = nullptr);

    /// 项目根（解析剪辑纹理的相对路径；空 = 无项目，按绝对路径处理）
    void setProjectRoot(const QString &root) { m_projectRoot = root; }

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

    /// 从精灵表拖入帧——追加到当前剪辑的帧序列（若无剪辑则自动新建）
    /// 参数由外部解析自 application/x-sprite-frame MIME 数据
    void addSpriteFrames(const QString &texturePath, int rows, int cols,
                         float frameWidth, float frameHeight,
                         float margin, float spacing, const std::vector<int> &frameIds);

signals:
    /// 剪辑数据被修改（mainwindow 接 dirty / 标题栏）
    void changed();
    /// 保存成功（mainwindow 接：同步依赖该 .anim 的 Animator 状态）
    void saved(const QString &path);

protected:
    // 接受精灵表 Dock 拖入帧（kSpriteFrameMime）
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onPlayToggle();
    void onLoopToggled(bool on);
    void onNameEdited();
    void onUniformDurationChanged(double v);
    void onTimelineChanged();
    void onFrameRemoved(int blockIndex);   ///< 双击时间轴块 → 移除该帧

private:
    void updateTitle();
    void refreshClipParams();
    void loadClip(const Shit::AnimationClip &clip);
    void notifyClipChanged();
    void syncTimeline();
    /// 按 m_clip.texturePath 重载精灵表纹理缓存（用于时间轴缩略图）
    void reloadSheetImage();
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
    QString m_projectRoot;
    QImage m_sheetImage;              ///< 当前精灵表纹理缓存（用于切帧缩略图；isNull = 无）

    // 顶部
    QLabel *m_fileLabel = nullptr;
    QToolButton *m_playBtn = nullptr;
    QCheckBox *m_loopCheck = nullptr;

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