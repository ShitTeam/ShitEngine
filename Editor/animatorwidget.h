#ifndef ANIMATORWIDGET_H
#define ANIMATORWIDGET_H

#include <QWidget>

#include <vector>

class QComboBox;
class QCheckBox;
class QDoubleSpinBox;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QSpinBox;
class QToolButton;
class QLabel;
class QTableWidget;

namespace Shit { class Animator; }

/// 动画状态机编辑器（P28）：为 Animator 提供可视化的状态机编辑 UI。
/// 三个 section：参数表（float/bool/trigger）、状态列表（含入口）+ 选中状态剪辑编辑、
/// 转换编辑（from→to + 条件）。
/// 所有修改经 Animator 的 API 写回并同步 m_animatorData 载体（.scene 可存），
/// 修改时发 changed() 信号（检查器接撤销/dirty）。
class AnimatorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AnimatorWidget(Shit::Animator *animator, QWidget *parent = nullptr);

    /// 重新从组件读取状态并刷新 UI（检查器每帧回读）
    void refresh();

signals:
    void changed();

private slots:
    // 参数
    void onAddParam();
    void onRemoveParam();
    void onParamTypeChanged(int index);
    void onParamNameEdited();
    void onParamValueEdited();
    // 状态
    void onAddState();
    void onRemoveState();
    void onStateSelectionChanged();
    void onStateNameEdited();
    void onStateEntryToggled(bool on);
    // 状态剪辑参数
    void onClipTexEdited();
    void onClipGridChanged();
    void onClipDurationChanged(double v);
    void onClipLoopToggled(bool on);
    void onFrameClicked(int tileId);
    void onClearFrames();
    // 转换
    void onAddTransition();
    void onRemoveTransition();
    void onTransitionSelectionChanged();
    void onTransitionFromChanged(int index);
    void onTransitionToChanged(int index);
    void onAddCondition();
    void onRemoveCondition();

private:
    Shit::Animator *m_animator;

    // 参数区
    QListWidget *m_paramList = nullptr;
    QComboBox *m_paramTypeCombo = nullptr;
    QLineEdit *m_paramNameEdit = nullptr;
    QLineEdit *m_paramValueEdit = nullptr;

    // 状态区
    QListWidget *m_stateList = nullptr;
    QLineEdit *m_stateNameEdit = nullptr;
    QCheckBox *m_stateEntryCheck = nullptr;

    // 选中状态的剪辑编辑
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

    // 转换区
    QListWidget *m_transitionList = nullptr;
    QComboBox *m_fromCombo = nullptr;
    QComboBox *m_toCombo = nullptr;
    QListWidget *m_condList = nullptr;

    int m_paramSel = -1;
    int m_stateSel = -1;
    int m_transitionSel = -1;
    bool m_updating = false;

    void rebuildParams();
    void rebuildStates();
    void rebuildTransitions();
    void rebuildTransitionCombos();
    void rebuildConditionList();
    void rebuildFrameGrid();
    void clearFrameGrid();
    void refreshFrameHighlights();
    void refreshClipWidgets();
    void refreshStateWidgets();
    void refreshParamWidgets();
    void refreshTransitionWidgets();
    void applySelectedClipToWidgets();
    void writeClipFromWidgets();
};

#endif // ANIMATORWIDGET_H
