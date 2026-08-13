#ifndef ANIMATORDOCK_H
#define ANIMATORDOCK_H

#include <QWidget>

#include <vector>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QSpinBox;
class QToolButton;

class AnimatorGraphView;

namespace Shit { class Animator; class GameObject; }

/// Unity 风格 Animator 窗口（P28）：独立 Dock 面板。
/// 布局：中央状态机图（AnimatorGraphView）+ 右侧参数面板 + 底部属性面板。
/// - 图中右键拖拽建转换、拖节点移动、Delete 删除选中、滚轮缩放
/// - 参数面板：增删 float/bool/trigger 参数
/// - 属性面板：选中状态 → 名称/入口 + 剪辑编辑（纹理/网格/时长/循环/帧序列点选）；
///   选中转换 → from/to + 条件列表
/// 所有修改经 Animator API 写回并同步载体；setGameObject 绑定/解绑，每帧 refresh()。
class AnimatorDock : public QWidget
{
    Q_OBJECT
public:
    explicit AnimatorDock(QWidget *parent = nullptr);

    /// 绑定选中对象（取对象 Animator；nullptr = 解绑）
    void setGameObject(Shit::GameObject *object);
    /// 每帧刷新（由 mainwindow 驱动）
    void refresh();

signals:
    /// 状态机数据被修改（撤销 begin/commit 由 mainwindow 接线）
    void changed();

private slots:
    // 参数
    void onAddParam();
    void onRemoveParam();
    void onParamTypeChanged(int index);
    void onParamNameEdited();
    void onParamValueEdited();
    void onParamRowChanged(int row);
    // 状态（工具栏按钮 + 属性面板）
    void onAddState();
    void onRemoveState();
    void onStateNameEdited();
    void onStateEntryToggled(bool on);
    // 剪辑
    void onClipTexEdited();
    void onClipGridChanged();
    void onClipDurationChanged(double v);
    void onClipLoopToggled(bool on);
    void onFrameClicked(int tileId);
    void onClearFrames();
    // 转换（工具栏按钮 + 属性面板）
    void onAddTransition();
    void onRemoveTransition();
    void onFromChanged(int index);
    void onToChanged(int index);
    void onAddCondition();
    void onRemoveCondition();

private:
    void bindAnimator(Shit::Animator *animator);
    void rebuildParams();
    void refreshParamWidgets();
    void refreshStateWidgets();
    void refreshTransitionWidgets();
    void refreshClipWidgets();
    void rebuildFrameGrid();
    void clearFrameGrid();
    void refreshFrameHighlights();
    void writeClipFromWidgets();
    void rebuildConditionList();
    void refreshConditionWidgets();

    Shit::Animator *m_animator = nullptr;

    AnimatorGraphView *m_graph = nullptr;

    // 参数面板
    QListWidget *m_paramList = nullptr;
    QComboBox *m_paramTypeCombo = nullptr;
    QLineEdit *m_paramNameEdit = nullptr;
    QLineEdit *m_paramValueEdit = nullptr;
    int m_paramSel = -1;

    // 状态属性
    QLineEdit *m_stateNameEdit = nullptr;
    QCheckBox *m_stateEntryCheck = nullptr;
    QLineEdit *m_texEdit = nullptr;
    QSpinBox *m_rowsSpin = nullptr;
    QSpinBox *m_colsSpin = nullptr;
    QDoubleSpinBox *m_fwSpin = nullptr;
    QDoubleSpinBox *m_fhSpin = nullptr;
    QDoubleSpinBox *m_durSpin = nullptr;
    QCheckBox *m_loopCheck = nullptr;
    QToolButton *m_clearFramesBtn = nullptr;
    QLabel *m_framesHint = nullptr;
    QWidget *m_gridHost = nullptr;
    std::vector<std::pair<int, QToolButton*>> m_frameButtons;
    QString m_frameGridSig;

    // 转换属性
    QComboBox *m_fromCombo = nullptr;
    QComboBox *m_toCombo = nullptr;
    QListWidget *m_condList = nullptr;

    int m_selState = -1;
    int m_selTransition = -1;
    bool m_updating = false;
    uint64_t m_cachedGeneration = 0;  ///< 上次 refresh 时 Animator 的代数，用于判断是否需要重建图
};

#endif // ANIMATORDOCK_H
