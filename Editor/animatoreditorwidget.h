#ifndef ANIMATOREDITORWIDGET_H
#define ANIMATOREDITORWIDGET_H

#include <QWidget>

#include <vector>

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;
class QToolButton;
class QLabel;

namespace Shit { class AnimationComponent; }

/// 动画剪辑编辑器（P28）：为 AnimationComponent 提供可视化的剪辑编辑 UI。
/// 支持：剪辑列表增删/切换、每帧时长/循环/默认播放、纹理网格参数、帧序列点选。
/// 所有修改经 AnimationComponent 的剪辑 API 写回并同步 m_clipsData 载体（.scene 可存）。
/// 修改时发 changed() 信号（供检查器接入撤销/dirty）。
class AnimatorEditorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AnimatorEditorWidget(Shit::AnimationComponent *comp, QWidget *parent = nullptr);

    /// 重新从组件读取剪辑状态并刷新 UI（检查器每帧回读调用）
    void refresh();

signals:
    /// 剪辑数据被修改（撤销 begin/commit 由检查器接线）
    void changed();

private slots:
    void onClipIndexChanged(int index);
    void onAddClip();
    void onRemoveClip();
    void onClipNameEdited();
    void onTextureEdited();
    void onGridParamChanged();
    void onDurationChanged(double v);
    void onLoopToggled(bool on);
    void onSetDefault();
    void onFrameClicked(int tileId);
    void onClearFrames();
    void onPreviewToggle();

private:
    Shit::AnimationComponent *m_comp;   ///< 目标组件（仅指针，销毁前检查器会清空并析构本控件）

    // 剪辑选择
    QComboBox *m_clipBox = nullptr;
    QToolButton *m_addBtn = nullptr;
    QToolButton *m_removeBtn = nullptr;

    // 选中剪辑参数
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_texEdit = nullptr;
    QSpinBox *m_rowsSpin = nullptr;
    QSpinBox *m_colsSpin = nullptr;
    QDoubleSpinBox *m_fwSpin = nullptr;
    QDoubleSpinBox *m_fhSpin = nullptr;
    QDoubleSpinBox *m_durSpin = nullptr;
    QCheckBox *m_loopCheck = nullptr;
    QToolButton *m_defaultBtn = nullptr;
    QToolButton *m_clearFramesBtn = nullptr;
    QLabel *m_framesHint = nullptr;

    // 帧预览网格容器
    QWidget *m_gridHost = nullptr;

    // 预览按钮（播放当前剪辑）
    QToolButton *m_previewBtn = nullptr;

    int m_selectedClip = -1;          ///< 当前选中剪辑索引
    std::vector<std::pair<int, QToolButton*>> m_frameButtons;  ///< 网格按钮（id → 按钮）
    QString m_frameGridSig;           ///< 当前帧网格签名（纹理+网格参数），变化时才重建

    bool m_updating = false;          ///< 防重入（填充控件时抑制 changed 信号）

    /// 重建剪辑下拉列表
    void rebuildClipList();
    /// 仅同步下拉中的剪辑名文本（不清空、不改变选中）
    void refreshClipNames();
    /// 重建帧预览网格（基于选中剪辑纹理）
    void rebuildFrameGrid();
    /// 清空帧预览网格
    void clearFrameGrid();
    /// 刷新选中剪辑的参数编辑区
    void refreshParams();
    /// 刷新帧按钮高亮（按选中剪辑 frames）
    void refreshFrameHighlights();
    /// 从选中剪辑读各参数并写入控件（blockSignals）
    void applyClipToWidgets();
    /// 从控件读回各参数写入选中的剪辑并同步载体；isCommit 决定是否发 changed
    void writeWidgetsToClip();
    /// 当前选中的剪辑参数是否有有效纹理（决定帧网格显示与否）
    void updatePreviewState();
};

#endif // ANIMATOREDITORWIDGET_H
