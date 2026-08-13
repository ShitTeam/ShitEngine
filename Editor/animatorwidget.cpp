#include "animatorwidget.h"

#include <ShitEngine/Animation/Animator.h>
#include <ShitEngine/Component/SpriteRenderer.h>
#include <ShitEngine/GameObject/GameObject.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <initializer_list>
#include <set>

namespace {

/// 尝试把相对路径解析为可读的绝对路径（候选：cwd / exe 目录 / 常见资源根）
QString resolveAssetPath(const QString &path)
{
    if (path.isEmpty()) return QString();
    QFileInfo direct(path);
    if (direct.isAbsolute() || direct.exists())
        return direct.absoluteFilePath();
    const QString appDir = QCoreApplication::applicationDirPath();
    QString candidate = appDir + "/" + path;
    if (QFileInfo(candidate).isFile()) return candidate;
    for (const QString &root : { "resource", "assets", "Assets" }) {
        candidate = appDir + "/" + root + "/" + path;
        if (QFileInfo(candidate).isFile()) return candidate;
    }
    return path;
}

QString paramTypeName(Shit::AnimatorParamType t)
{
    switch (t) {
        case Shit::AnimatorParamType::Float:   return QObject::tr("Float");
        case Shit::AnimatorParamType::Bool:    return QObject::tr("Bool");
        case Shit::AnimatorParamType::Trigger: return QObject::tr("Trigger");
    }
    return QString();
}

QString condTypeName(Shit::AnimatorConditionType t)
{
    switch (t) {
        case Shit::AnimatorConditionType::FloatGt: return QObject::tr(">");
        case Shit::AnimatorConditionType::FloatLt: return QObject::tr("<");
        case Shit::AnimatorConditionType::FloatEq: return QObject::tr("==");
        case Shit::AnimatorConditionType::Bool:    return QObject::tr("== Bool");
        case Shit::AnimatorConditionType::Trigger: return QObject::tr("Trigger");
    }
    return QString();
}

} // namespace

AnimatorWidget::AnimatorWidget(Shit::Animator *animator, QWidget *parent)
    : QWidget(parent)
    , m_animator(animator)
{
    // ── 参数区 ──
    auto *paramTitle = new QLabel(tr("参数"), this);
    m_paramList = new QListWidget(this);
    m_paramList->setFixedHeight(90);
    m_paramTypeCombo = new QComboBox(this);
    m_paramTypeCombo->addItem(tr("Float"));
    m_paramTypeCombo->addItem(tr("Bool"));
    m_paramTypeCombo->addItem(tr("Trigger"));
    m_paramNameEdit = new QLineEdit(this);
    m_paramNameEdit->setPlaceholderText(tr("参数名"));
    m_paramValueEdit = new QLineEdit(this);
    m_paramValueEdit->setPlaceholderText(tr("值"));
    auto *addParamBtn = new QToolButton(this); addParamBtn->setText(tr("+")); addParamBtn->setToolTip(tr("新增参数"));
    auto *delParamBtn = new QToolButton(this); delParamBtn->setText(tr("−")); delParamBtn->setToolTip(tr("删除参数"));
    auto *paramBtnRow = new QHBoxLayout;
    paramBtnRow->addWidget(addParamBtn); paramBtnRow->addWidget(delParamBtn); paramBtnRow->addStretch();
    auto *paramEditRow = new QHBoxLayout;
    paramEditRow->addWidget(m_paramTypeCombo);
    paramEditRow->addWidget(m_paramNameEdit, 1);
    paramEditRow->addWidget(m_paramValueEdit, 1);

    // ── 状态区 ──
    auto *stateTitle = new QLabel(tr("状态"), this);
    m_stateList = new QListWidget(this);
    m_stateList->setFixedHeight(90);
    auto *addStateBtn = new QToolButton(this); addStateBtn->setText(tr("+")); addStateBtn->setToolTip(tr("新增状态"));
    auto *delStateBtn = new QToolButton(this); delStateBtn->setText(tr("−")); delStateBtn->setToolTip(tr("删除状态"));
    auto *stateBtnRow = new QHBoxLayout;
    stateBtnRow->addWidget(addStateBtn); stateBtnRow->addWidget(delStateBtn); stateBtnRow->addStretch();
    m_stateNameEdit = new QLineEdit(this);
    m_stateNameEdit->setPlaceholderText(tr("状态名"));
    m_stateEntryCheck = new QCheckBox(tr("入口状态"), this);

    // ── 选中状态的剪辑编辑 ──
    auto *clipTitle = new QLabel(tr("选中状态剪辑"), this);
    m_texEdit = new QLineEdit(this);
    m_texEdit->setPlaceholderText(tr("纹理路径（留空用对象 SpriteRenderer）"));
    m_rowsSpin = new QSpinBox(this); m_rowsSpin->setRange(1, 256);
    m_colsSpin = new QSpinBox(this); m_colsSpin->setRange(1, 256);
    m_fwSpin = new QDoubleSpinBox(this); m_fwSpin->setRange(1, 4096); m_fwSpin->setDecimals(1);
    m_fhSpin = new QDoubleSpinBox(this); m_fhSpin->setRange(1, 4096); m_fhSpin->setDecimals(1);
    m_durSpin = new QDoubleSpinBox(this); m_durSpin->setRange(0.001, 60.0); m_durSpin->setDecimals(3); m_durSpin->setSingleStep(0.01);
    m_loopCheck = new QCheckBox(tr("循环"), this);
    m_clearFramesBtn = new QToolButton(this); m_clearFramesBtn->setText(tr("清空帧"));
    m_framesHint = new QLabel(this); m_framesHint->setStyleSheet("color:#9aa7b4;");

    auto *clipGrid = new QGridLayout;
    int r = 0;
    clipGrid->addWidget(new QLabel(tr("纹理"), this), r, 0); clipGrid->addWidget(m_texEdit, r, 1, 1, 2); ++r;
    clipGrid->addWidget(new QLabel(tr("行/列"), this), r, 0); clipGrid->addWidget(m_rowsSpin, r, 1); clipGrid->addWidget(m_colsSpin, r, 2); ++r;
    clipGrid->addWidget(new QLabel(tr("帧宽/高"), this), r, 0); clipGrid->addWidget(m_fwSpin, r, 1); clipGrid->addWidget(m_fhSpin, r, 2); ++r;
    clipGrid->addWidget(new QLabel(tr("每帧秒"), this), r, 0); clipGrid->addWidget(m_durSpin, r, 1); clipGrid->addWidget(m_loopCheck, r, 2); ++r;
    clipGrid->addWidget(m_clearFramesBtn, r, 0, 1, 2); clipGrid->addWidget(m_framesHint, r, 2); ++r;

    m_gridHost = new QWidget(this);
    auto *frameScroll = new QScrollArea(this);
    frameScroll->setWidgetResizable(true);
    frameScroll->setWidget(m_gridHost);
    frameScroll->setFixedHeight(110);

    // ── 转换区 ──
    auto *transTitle = new QLabel(tr("转换"), this);
    m_transitionList = new QListWidget(this);
    m_transitionList->setFixedHeight(80);
    auto *addTransBtn = new QToolButton(this); addTransBtn->setText(tr("+")); addTransBtn->setToolTip(tr("新增转换"));
    auto *delTransBtn = new QToolButton(this); delTransBtn->setText(tr("−")); delTransBtn->setToolTip(tr("删除转换"));
    auto *transBtnRow = new QHBoxLayout;
    transBtnRow->addWidget(addTransBtn); transBtnRow->addWidget(delTransBtn); transBtnRow->addStretch();
    m_fromCombo = new QComboBox(this);
    m_toCombo = new QComboBox(this);
    auto *fromToRow = new QHBoxLayout;
    fromToRow->addWidget(new QLabel(tr("从"), this)); fromToRow->addWidget(m_fromCombo, 1);
    fromToRow->addWidget(new QLabel(tr("到"), this)); fromToRow->addWidget(m_toCombo, 1);
    m_condList = new QListWidget(this);
    m_condList->setFixedHeight(70);
    auto *addCondBtn = new QToolButton(this); addCondBtn->setText(tr("+ 条件"));
    auto *delCondBtn = new QToolButton(this); delCondBtn->setText(tr("− 条件"));
    auto *condBtnRow = new QHBoxLayout;
    condBtnRow->addWidget(addCondBtn); condBtnRow->addWidget(delCondBtn); condBtnRow->addStretch();

    // ── 汇总布局 ──
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    auto *content = new QWidget;
    auto *main = new QVBoxLayout(content);
    main->setContentsMargins(0, 0, 0, 0);
    main->setSpacing(6);
    main->addWidget(paramTitle); main->addLayout(paramEditRow); main->addLayout(paramBtnRow); main->addWidget(m_paramList);
    main->addWidget(stateTitle); main->addWidget(m_stateNameEdit); main->addWidget(m_stateEntryCheck); main->addLayout(stateBtnRow); main->addWidget(m_stateList);
    main->addWidget(clipTitle); main->addLayout(clipGrid); main->addWidget(frameScroll);
    main->addWidget(transTitle); main->addLayout(fromToRow); main->addLayout(transBtnRow); main->addWidget(m_transitionList);
    main->addWidget(new QLabel(tr("条件（全满足才切换）"), this)); main->addLayout(condBtnRow); main->addWidget(m_condList);
    main->addStretch();
    scroll->setWidget(content);
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(scroll);

    // ── 连接 ──
    connect(addParamBtn, &QToolButton::clicked, this, &AnimatorWidget::onAddParam);
    connect(delParamBtn, &QToolButton::clicked, this, &AnimatorWidget::onRemoveParam);
    connect(m_paramTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &AnimatorWidget::onParamTypeChanged);
    connect(m_paramNameEdit, &QLineEdit::editingFinished, this, &AnimatorWidget::onParamNameEdited);
    connect(m_paramValueEdit, &QLineEdit::editingFinished, this, &AnimatorWidget::onParamValueEdited);
    connect(m_paramList, &QListWidget::currentRowChanged, this, [this](int r) { m_paramSel = r; refreshParamWidgets(); });

    connect(addStateBtn, &QToolButton::clicked, this, &AnimatorWidget::onAddState);
    connect(delStateBtn, &QToolButton::clicked, this, &AnimatorWidget::onRemoveState);
    connect(m_stateList, &QListWidget::currentRowChanged, this, &AnimatorWidget::onStateSelectionChanged);
    connect(m_stateNameEdit, &QLineEdit::editingFinished, this, &AnimatorWidget::onStateNameEdited);
    connect(m_stateEntryCheck, &QCheckBox::toggled, this, &AnimatorWidget::onStateEntryToggled);

    connect(m_texEdit, &QLineEdit::editingFinished, this, &AnimatorWidget::onClipTexEdited);
    connect(m_rowsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &AnimatorWidget::onClipGridChanged);
    connect(m_colsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &AnimatorWidget::onClipGridChanged);
    connect(m_fwSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimatorWidget::onClipGridChanged);
    connect(m_fhSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimatorWidget::onClipGridChanged);
    connect(m_durSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimatorWidget::onClipDurationChanged);
    connect(m_loopCheck, &QCheckBox::toggled, this, &AnimatorWidget::onClipLoopToggled);
    connect(m_clearFramesBtn, &QToolButton::clicked, this, &AnimatorWidget::onClearFrames);

    connect(addTransBtn, &QToolButton::clicked, this, &AnimatorWidget::onAddTransition);
    connect(delTransBtn, &QToolButton::clicked, this, &AnimatorWidget::onRemoveTransition);
    connect(m_transitionList, &QListWidget::currentRowChanged, this, &AnimatorWidget::onTransitionSelectionChanged);
    connect(m_fromCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &AnimatorWidget::onTransitionFromChanged);
    connect(m_toCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &AnimatorWidget::onTransitionToChanged);
    connect(addCondBtn, &QToolButton::clicked, this, &AnimatorWidget::onAddCondition);
    connect(delCondBtn, &QToolButton::clicked, this, &AnimatorWidget::onRemoveCondition);

    refresh();
}

void AnimatorWidget::refresh()
{
    if (!m_animator) return;
    m_updating = true;
    rebuildParams();
    rebuildStates();
    rebuildTransitions();
    m_updating = false;
    rebuildFrameGrid();
    refreshClipWidgets();
    refreshTransitionWidgets();
    refreshParamWidgets();
    refreshStateWidgets();
}

// ═══════════════════════════════════════════════════════════
// 参数
// ═══════════════════════════════════════════════════════════

void AnimatorWidget::rebuildParams()
{
    m_paramList->blockSignals(true);
    m_paramList->clear();
    const int n = m_animator ? m_animator->paramCount() : 0;
    for (int i = 0; i < n; ++i) {
        const Shit::AnimatorParameter *p = m_animator->paramAt(i);
        QString label;
        if (p) {
            label = QString("%1  [%2]").arg(QString::fromStdString(p->name), paramTypeName(p->type));
        } else label = tr("?");
        m_paramList->addItem(label);
    }
    if (m_paramSel >= n) m_paramSel = -1;
    if (m_paramSel >= 0) m_paramList->setCurrentRow(m_paramSel);
    m_paramList->blockSignals(false);
}

void AnimatorWidget::refreshParamWidgets()
{
    const Shit::AnimatorParameter *p = m_animator && m_paramSel >= 0 ? m_animator->paramAt(m_paramSel) : nullptr;
    const bool valid = p != nullptr;
    m_paramTypeCombo->setEnabled(valid);
    m_paramNameEdit->setEnabled(valid);
    m_paramValueEdit->setEnabled(valid);
    if (!p) return;
    m_paramTypeCombo->setCurrentIndex(static_cast<int>(p->type));
    m_paramNameEdit->setText(QString::fromStdString(p->name));
    switch (p->type) {
        case Shit::AnimatorParamType::Float:
            m_paramValueEdit->setText(QString::number(p->floatValue));
            break;
        case Shit::AnimatorParamType::Bool:
        case Shit::AnimatorParamType::Trigger:
            m_paramValueEdit->setText(p->boolValue ? tr("true") : tr("false"));
            break;
    }
}

void AnimatorWidget::onAddParam()
{
    if (!m_animator) return;
    const int idx = m_animator->addParam("Param", Shit::AnimatorParamType::Float);
    if (idx < 0) return;
    m_paramSel = idx;
    emit changed();
    refresh();
}

void AnimatorWidget::onRemoveParam()
{
    if (!m_animator || m_paramSel < 0) return;
    if (m_animator->removeParam(m_paramSel)) {
        m_paramSel = -1;
        emit changed();
        refresh();
    }
}

void AnimatorWidget::onParamTypeChanged(int index)
{
    if (m_updating || !m_animator || m_paramSel < 0) return;
    Shit::AnimatorParameter p = *m_animator->paramAt(m_paramSel);
    p.type = static_cast<Shit::AnimatorParamType>(index);
    if (m_animator->setParam(m_paramSel, p)) { emit changed(); refreshParamWidgets(); rebuildParams(); }
}

void AnimatorWidget::onParamNameEdited()
{
    if (!m_animator || m_paramSel < 0) return;
    Shit::AnimatorParameter p = *m_animator->paramAt(m_paramSel);
    const std::string name = m_paramNameEdit->text().trimmed().toStdString();
    if (name.empty()) { refresh(); return; }
    p.name = name;
    if (m_animator->setParam(m_paramSel, p)) { emit changed(); rebuildParams(); }
}

void AnimatorWidget::onParamValueEdited()
{
    if (!m_animator || m_paramSel < 0) return;
    Shit::AnimatorParameter p = *m_animator->paramAt(m_paramSel);
    const QString v = m_paramValueEdit->text().trimmed();
    if (p.type == Shit::AnimatorParamType::Float) {
        bool ok = false;
        float f = v.toFloat(&ok);
        if (ok) p.floatValue = f;
    } else {
        p.boolValue = (v.compare("true", Qt::CaseInsensitive) == 0);
    }
    if (m_animator->setParam(m_paramSel, p)) emit changed();
}

// ═══════════════════════════════════════════════════════════
// 状态
// ═══════════════════════════════════════════════════════════

void AnimatorWidget::rebuildStates()
{
    m_stateList->blockSignals(true);
    m_stateList->clear();
    const int n = m_animator ? m_animator->stateCount() : 0;
    for (int i = 0; i < n; ++i) {
        const Shit::AnimatorState *s = m_animator->stateAt(i);
        QString label = s ? QString::fromStdString(s->name) : QString("State %1").arg(i);
        if (s && s->isEntry) label += tr("  [入口]");
        m_stateList->addItem(label);
    }
    if (m_stateSel >= n) m_stateSel = -1;
    if (m_stateSel >= 0) m_stateList->setCurrentRow(m_stateSel);
    m_stateList->blockSignals(false);
}

void AnimatorWidget::refreshStateWidgets()
{
    const Shit::AnimatorState *s = m_animator && m_stateSel >= 0 ? m_animator->stateAt(m_stateSel) : nullptr;
    const bool valid = s != nullptr;
    m_stateNameEdit->setEnabled(valid);
    m_stateEntryCheck->setEnabled(valid);
    if (s) {
        m_stateNameEdit->setText(QString::fromStdString(s->name));
        m_stateEntryCheck->setChecked(s->isEntry);
    }
}

void AnimatorWidget::onAddState()
{
    if (!m_animator) return;
    const int idx = m_animator->addState("State");
    if (idx < 0) return;
    m_stateSel = idx;
    emit changed();
    refresh();
}

void AnimatorWidget::onRemoveState()
{
    if (!m_animator || m_stateSel < 0) return;
    if (m_animator->removeState(m_stateSel)) {
        m_stateSel = -1;
        emit changed();
        refresh();
    }
}

void AnimatorWidget::onStateSelectionChanged()
{
    if (m_updating) return;
    const int row = m_stateList->currentRow();
    if (row < 0) return;
    m_stateSel = row;
    refreshStateWidgets();
    refreshClipWidgets();
    rebuildFrameGrid();
    rebuildTransitionCombos();
}

void AnimatorWidget::onStateNameEdited()
{
    if (!m_animator || m_stateSel < 0) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_stateSel);
    const std::string name = m_stateNameEdit->text().trimmed().toStdString();
    if (name.empty()) { refresh(); return; }
    s.name = name;
    if (m_animator->setState(m_stateSel, s)) { emit changed(); rebuildStates(); }
}

void AnimatorWidget::onStateEntryToggled(bool on)
{
    if (m_updating || !m_animator || m_stateSel < 0) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_stateSel);
    if (s.isEntry == on) return;
    s.isEntry = on;
    if (m_animator->setState(m_stateSel, s)) { emit changed(); rebuildStates(); }
}

// ═══════════════════════════════════════════════════════════
// 状态剪辑编辑
// ═══════════════════════════════════════════════════════════

void AnimatorWidget::refreshClipWidgets()
{
    const Shit::AnimatorState *s = m_animator && m_stateSel >= 0 ? m_animator->stateAt(m_stateSel) : nullptr;
    const bool valid = s != nullptr;
    for (QWidget *w : std::initializer_list<QWidget*>{
             m_texEdit, m_rowsSpin, m_colsSpin, m_fwSpin, m_fhSpin, m_durSpin })
        w->setEnabled(valid);
    m_loopCheck->setEnabled(valid);
    m_clearFramesBtn->setEnabled(valid);
    if (!s) { m_framesHint->setText(tr("选择状态后编辑剪辑")); return; }
    const Shit::AnimationClip &clip = s->clip;
    if (!m_texEdit->hasFocus()) m_texEdit->setText(QString::fromStdString(clip.texturePath));
    if (!m_rowsSpin->hasFocus()) m_rowsSpin->setValue(clip.rows);
    if (!m_colsSpin->hasFocus()) m_colsSpin->setValue(clip.cols);
    if (!m_fwSpin->hasFocus()) m_fwSpin->setValue(clip.frameWidth);
    if (!m_fhSpin->hasFocus()) m_fhSpin->setValue(clip.frameHeight);
    if (!m_durSpin->hasFocus()) m_durSpin->setValue(clip.duration);
    m_loopCheck->setChecked(clip.loop);
    m_framesHint->setText(tr("帧数 %1").arg(clip.frames.size()));
}

void AnimatorWidget::writeClipFromWidgets()
{
    if (!m_animator || m_stateSel < 0) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_stateSel);
    Shit::AnimationClip &clip = s.clip;
    clip.texturePath = m_texEdit->text().trimmed().toStdString();
    clip.rows = m_rowsSpin->value();
    clip.cols = m_colsSpin->value();
    clip.frameWidth = static_cast<float>(m_fwSpin->value());
    clip.frameHeight = static_cast<float>(m_fhSpin->value());
    clip.duration = static_cast<float>(m_durSpin->value());
    clip.loop = m_loopCheck->isChecked();
    if (m_animator->setState(m_stateSel, s))
        emit changed();
}

void AnimatorWidget::onClipTexEdited() { writeClipFromWidgets(); rebuildFrameGrid(); }
void AnimatorWidget::onClipGridChanged() { writeClipFromWidgets(); rebuildFrameGrid(); }
void AnimatorWidget::onClipDurationChanged(double) { writeClipFromWidgets(); }
void AnimatorWidget::onClipLoopToggled(bool) { writeClipFromWidgets(); }

void AnimatorWidget::onClearFrames()
{
    if (!m_animator || m_stateSel < 0) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_stateSel);
    s.clip.frames.clear();
    if (m_animator->setState(m_stateSel, s)) {
        emit changed();
        refreshFrameHighlights();
        m_framesHint->setText(tr("帧数 0"));
    }
}

void AnimatorWidget::onFrameClicked(int tileId)
{
    if (!m_animator || m_stateSel < 0) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_stateSel);
    auto it = std::find(s.clip.frames.begin(), s.clip.frames.end(), tileId);
    if (it != s.clip.frames.end()) s.clip.frames.erase(it);
    else s.clip.frames.push_back(tileId);
    if (m_animator->setState(m_stateSel, s)) {
        emit changed();
        refreshFrameHighlights();
        m_framesHint->setText(tr("帧数 %1").arg(s.clip.frames.size()));
    }
}

void AnimatorWidget::rebuildFrameGrid()
{
    const Shit::AnimatorState *s = m_animator && m_stateSel >= 0 ? m_animator->stateAt(m_stateSel) : nullptr;
    if (!s || s->clip.cols <= 0 || s->clip.rows <= 0 || s->clip.frameWidth <= 0 || s->clip.frameHeight <= 0) {
        clearFrameGrid();
        return;
    }
    const QString texPath = resolveAssetPath(QString::fromStdString(s->clip.texturePath));
    const QString sig = QString("%1|%2x%3|%4x%5")
                            .arg(texPath).arg(s->clip.rows).arg(s->clip.cols)
                            .arg(s->clip.frameWidth).arg(s->clip.frameHeight);
    if (sig == m_frameGridSig && !m_frameButtons.empty()) { refreshFrameHighlights(); return; }
    clearFrameGrid();

    QImage sheet(texPath);
    if (sheet.isNull()) { m_frameGridSig.clear(); return; }
    const int tileW = static_cast<int>(s->clip.frameWidth);
    const int tileH = static_cast<int>(s->clip.frameHeight);
    const int tilesPerRow = sheet.width() / tileW;
    const int tileCount = (sheet.width() / tileW) * (sheet.height() / tileH);
    if (tilesPerRow <= 0 || tileCount <= 0) { m_frameGridSig.clear(); return; }

    auto *grid = new QGridLayout(m_gridHost);
    grid->setContentsMargins(2, 2, 2, 2);
    grid->setSpacing(2);
    const int maxCols = 8;
    const int pw = qMax(20, qMin(tileW, 52));
    const int ph = qMax(20, qMin(tileH, 52));
    for (int id = 0; id < tileCount; ++id) {
        const int sx = (id % tilesPerRow) * tileW;
        const int sy = (id / tilesPerRow) * tileH;
        QImage img = sheet.copy(sx, sy, tileW, tileH).scaled(pw, ph, Qt::KeepAspectRatio, Qt::FastTransformation);
        auto *btn = new QToolButton(m_gridHost);
        btn->setIcon(QIcon(QPixmap::fromImage(img)));
        btn->setIconSize(img.size());
        btn->setToolTip(QString("帧 %1").arg(id));
        btn->setCheckable(true);
        grid->addWidget(btn, id / maxCols, id % maxCols);
        connect(btn, &QToolButton::clicked, this, [this, id] { onFrameClicked(id); });
        m_frameButtons.emplace_back(id, btn);
    }
    m_frameGridSig = sig;
    refreshFrameHighlights();
    m_gridHost->adjustSize();
}

void AnimatorWidget::clearFrameGrid()
{
    if (QLayout *old = m_gridHost->layout()) {
        while (auto *item = old->takeAt(0))
            if (QWidget *w = item->widget()) delete w;
        delete old;
    }
    m_frameButtons.clear();
}

void AnimatorWidget::refreshFrameHighlights()
{
    const Shit::AnimatorState *s = m_animator && m_stateSel >= 0 ? m_animator->stateAt(m_stateSel) : nullptr;
    std::set<int> used;
    if (s) used.insert(s->clip.frames.begin(), s->clip.frames.end());
    for (auto &[id, btn] : m_frameButtons)
        btn->setChecked(used.count(id) > 0);
}

// ═══════════════════════════════════════════════════════════
// 转换
// ═══════════════════════════════════════════════════════════

void AnimatorWidget::rebuildTransitions()
{
    m_transitionList->blockSignals(true);
    m_transitionList->clear();
    const int n = m_animator ? m_animator->transitionCount() : 0;
    for (int i = 0; i < n; ++i) {
        const Shit::AnimatorTransition *t = m_animator->transitionAt(i);
        if (!t) { m_transitionList->addItem(tr("?")); continue; }
        const QString from = (t->fromState == -1) ? tr("*") :
            (m_animator->stateAt(t->fromState) ? QString::fromStdString(m_animator->stateAt(t->fromState)->name) : QString::number(t->fromState));
        const QString to = m_animator->stateAt(t->toState) ? QString::fromStdString(m_animator->stateAt(t->toState)->name) : QString::number(t->toState);
        m_transitionList->addItem(QString("%1 → %2").arg(from, to));
    }
    if (m_transitionSel >= n) m_transitionSel = -1;
    if (m_transitionSel >= 0) m_transitionList->setCurrentRow(m_transitionSel);
    m_transitionList->blockSignals(false);
    rebuildTransitionCombos();
    rebuildConditionList();
}

void AnimatorWidget::rebuildTransitionCombos()
{
    m_fromCombo->blockSignals(true);
    m_toCombo->blockSignals(true);
    m_fromCombo->clear();
    m_toCombo->clear();
    m_fromCombo->addItem(tr("*（任意）"));  // 对应 -1
    const int n = m_animator ? m_animator->stateCount() : 0;
    for (int i = 0; i < n; ++i) {
        const Shit::AnimatorState *s = m_animator->stateAt(i);
        const QString name = s ? QString::fromStdString(s->name) : QString("State %1").arg(i);
        m_fromCombo->addItem(name);
        m_toCombo->addItem(name);
    }
    m_fromCombo->blockSignals(false);
    m_toCombo->blockSignals(false);
}

void AnimatorWidget::refreshTransitionWidgets()
{
    const Shit::AnimatorTransition *t = m_animator && m_transitionSel >= 0 ? m_animator->transitionAt(m_transitionSel) : nullptr;
    const bool valid = t != nullptr;
    m_fromCombo->setEnabled(valid);
    m_toCombo->setEnabled(valid);
    m_condList->setEnabled(valid);
    if (!t) return;
    m_fromCombo->blockSignals(true);
    m_toCombo->blockSignals(true);
    m_fromCombo->setCurrentIndex(t->fromState + 1);  // 0=任意
    m_toCombo->setCurrentIndex(t->toState + 1);
    m_fromCombo->blockSignals(false);
    m_toCombo->blockSignals(false);
    rebuildConditionList();
}

void AnimatorWidget::rebuildConditionList()
{
    m_condList->blockSignals(true);
    m_condList->clear();
    const Shit::AnimatorTransition *t = m_animator && m_transitionSel >= 0 ? m_animator->transitionAt(m_transitionSel) : nullptr;
    if (!t) { m_condList->blockSignals(false); return; }
    for (const auto &c : t->conditions)
        m_condList->addItem(QString("%1 %2 %3").arg(QString::fromStdString(c.parameter), condTypeName(c.type),
                              c.type == Shit::AnimatorConditionType::Bool ? (c.boolValue ? tr("true") : tr("false"))
                                : QString::number(c.threshold)));
    m_condList->blockSignals(false);
}

void AnimatorWidget::onAddTransition()
{
    if (!m_animator || m_animator->stateCount() == 0) return;
    const int idx = m_animator->addTransition(-1, 0);
    if (idx < 0) return;
    m_transitionSel = idx;
    emit changed();
    rebuildTransitions();
    m_transitionList->setCurrentRow(idx);
}

void AnimatorWidget::onRemoveTransition()
{
    if (!m_animator || m_transitionSel < 0) return;
    if (m_animator->removeTransition(m_transitionSel)) {
        m_transitionSel = -1;
        emit changed();
        rebuildTransitions();
    }
}

void AnimatorWidget::onTransitionSelectionChanged()
{
    if (m_updating) return;
    const int row = m_transitionList->currentRow();
    if (row < 0) return;
    m_transitionSel = row;
    refreshTransitionWidgets();
}

void AnimatorWidget::onTransitionFromChanged(int index)
{
    if (m_updating || !m_animator || m_transitionSel < 0) return;
    Shit::AnimatorTransition t = *m_animator->transitionAt(m_transitionSel);
    t.fromState = index - 1;  // 0=任意 → -1
    if (m_animator->setTransition(m_transitionSel, t)) { emit changed(); rebuildTransitions(); }
}

void AnimatorWidget::onTransitionToChanged(int index)
{
    if (m_updating || !m_animator || m_transitionSel < 0) return;
    Shit::AnimatorTransition t = *m_animator->transitionAt(m_transitionSel);
    t.toState = index - 1;
    if (m_animator->setTransition(m_transitionSel, t)) { emit changed(); rebuildTransitions(); }
}

void AnimatorWidget::onAddCondition()
{
    if (!m_animator || m_transitionSel < 0) return;
    // 取第一个参数作为条件引用（无参数则提示）
    if (m_animator->paramCount() == 0) return;
    Shit::AnimatorTransition t = *m_animator->transitionAt(m_transitionSel);
    Shit::AnimatorTransitionCondition c;
    c.parameter = m_animator->paramAt(0)->name;
    c.type = Shit::AnimatorConditionType::FloatGt;
    c.threshold = 0.5f;
    t.conditions.push_back(c);
    if (m_animator->setTransition(m_transitionSel, t)) { emit changed(); rebuildConditionList(); }
}

void AnimatorWidget::onRemoveCondition()
{
    if (!m_animator || m_transitionSel < 0 || m_condList->currentRow() < 0) return;
    Shit::AnimatorTransition t = *m_animator->transitionAt(m_transitionSel);
    const int idx = m_condList->currentRow();
    if (idx >= static_cast<int>(t.conditions.size())) return;
    t.conditions.erase(t.conditions.begin() + idx);
    if (m_animator->setTransition(m_transitionSel, t)) { emit changed(); rebuildConditionList(); }
}
