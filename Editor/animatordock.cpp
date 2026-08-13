#include "animatordock.h"
#include "animatorgraphview.h"

#include <ShitEngine/Animation/Animator.h>
#include <ShitEngine/GameObject/GameObject.h>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QMessageBox>
#include <QPixmap>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <set>

namespace {

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
        case Shit::AnimatorConditionType::Bool:    return QObject::tr("Bool");
        case Shit::AnimatorConditionType::Trigger: return QObject::tr("Trigger");
    }
    return QString();
}

} // namespace

AnimatorDock::AnimatorDock(QWidget *parent)
    : QWidget(parent)
{
    setAcceptDrops(true);   // 方案 A：支持 .anim 资产拖入剪辑资产行
    // ── 工具栏：添加/删除状态、添加转换 ──
    auto *addStateBtn = new QToolButton(this); addStateBtn->setText(tr("+ 状态"));
    auto *delStateBtn = new QToolButton(this); delStateBtn->setText(tr("− 状态"));
    auto *addTransBtn = new QToolButton(this); addTransBtn->setText(tr("+ 转换"));
    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(4, 2, 4, 2);
    toolbar->addWidget(new QLabel(tr("状态机"), this));
    toolbar->addStretch();
    toolbar->addWidget(addStateBtn);
    toolbar->addWidget(delStateBtn);
    toolbar->addWidget(addTransBtn);

    // ── 中央状态机图 ──
    m_graph = new AnimatorGraphView(this);

    // ── 右侧参数面板 ──
    m_paramList = new QListWidget(this);
    m_paramList->setFixedWidth(150);
    m_paramTypeCombo = new QComboBox(this);
    m_paramTypeCombo->addItem(tr("Float"));
    m_paramTypeCombo->addItem(tr("Bool"));
    m_paramTypeCombo->addItem(tr("Trigger"));
    m_paramNameEdit = new QLineEdit(this);
    m_paramNameEdit->setPlaceholderText(tr("参数名"));
    m_paramValueEdit = new QLineEdit(this);
    m_paramValueEdit->setPlaceholderText(tr("值"));
    auto *addParamBtn = new QToolButton(this); addParamBtn->setText(tr("+"));
    auto *delParamBtn = new QToolButton(this); delParamBtn->setText(tr("−"));
    auto *paramBtnRow = new QHBoxLayout;
    paramBtnRow->addWidget(addParamBtn); paramBtnRow->addWidget(delParamBtn); paramBtnRow->addStretch();
    auto *paramPanel = new QWidget(this);
    auto *paramLayout = new QVBoxLayout(paramPanel);
    paramLayout->setContentsMargins(0, 0, 0, 0);
    paramLayout->addWidget(new QLabel(tr("参数"), this));
    paramLayout->addLayout(paramBtnRow);
    paramLayout->addWidget(m_paramTypeCombo);
    paramLayout->addWidget(m_paramNameEdit);
    paramLayout->addWidget(m_paramValueEdit);
    paramLayout->addWidget(m_paramList, 1);

    // ── 中央区：图 + 参数 ──
    auto *centerSplit = new QSplitter(Qt::Horizontal, this);
    centerSplit->addWidget(m_graph);
    centerSplit->addWidget(paramPanel);
    centerSplit->setStretchFactor(0, 1);
    centerSplit->setStretchFactor(1, 0);

    // ── 底部属性面板（状态/转换/剪辑） ──
    // 状态属性
    m_stateNameEdit = new QLineEdit(this);
    m_stateEntryCheck = new QCheckBox(tr("入口状态"), this);
    m_texEdit = new QLineEdit(this);
    m_texEdit->setPlaceholderText(tr("纹理路径（留空用 SpriteRenderer）"));
    m_rowsSpin = new QSpinBox(this); m_rowsSpin->setRange(1, 256);
    m_colsSpin = new QSpinBox(this); m_colsSpin->setRange(1, 256);
    m_fwSpin = new QDoubleSpinBox(this); m_fwSpin->setRange(1, 4096); m_fwSpin->setDecimals(1);
    m_fhSpin = new QDoubleSpinBox(this); m_fhSpin->setRange(1, 4096); m_fhSpin->setDecimals(1);
    m_durSpin = new QDoubleSpinBox(this); m_durSpin->setRange(0.001, 60.0); m_durSpin->setDecimals(3); m_durSpin->setSingleStep(0.01);
    m_loopCheck = new QCheckBox(tr("循环"), this);
    m_clearFramesBtn = new QToolButton(this); m_clearFramesBtn->setText(tr("清空帧"));
    m_framesHint = new QLabel(this); m_framesHint->setStyleSheet("color:#9aa7b4;");
    m_gridHost = new QWidget(this);

    auto *stateGrid = new QGridLayout;
    int r = 0;
    stateGrid->addWidget(new QLabel(tr("名称"), this), r, 0); stateGrid->addWidget(m_stateNameEdit, r, 1); stateGrid->addWidget(m_stateEntryCheck, r, 2); ++r;
    // 方案 A：剪辑资产行（引用的 .anim 文件；空 = 内嵌剪辑）
    m_assetPathEdit = new QLineEdit(this);
    m_assetPathEdit->setPlaceholderText(tr(".anim 资产路径（空 = 内嵌剪辑）"));
    m_assetPathEdit->setAcceptDrops(false);
    m_assetBrowseBtn = new QToolButton(this); m_assetBrowseBtn->setText(tr("浏览…"));
    m_assetOpenBtn = new QToolButton(this); m_assetOpenBtn->setText(tr("打开"));
    m_assetClearBtn = new QToolButton(this); m_assetClearBtn->setText(tr("清除"));
    auto *assetBtnRow = new QHBoxLayout;
    assetBtnRow->setContentsMargins(0, 0, 0, 0);
    assetBtnRow->addWidget(m_assetPathEdit, 1);
    assetBtnRow->addWidget(m_assetBrowseBtn);
    assetBtnRow->addWidget(m_assetOpenBtn);
    assetBtnRow->addWidget(m_assetClearBtn);
    stateGrid->addWidget(new QLabel(tr("剪辑资产"), this), r, 0);
    auto *assetHost = new QWidget(this);
    auto *assetHostLayout = new QVBoxLayout(assetHost);
    assetHostLayout->setContentsMargins(0, 0, 0, 0);
    assetHostLayout->addLayout(assetBtnRow);
    m_assetHint = new QLabel(tr("内嵌剪辑（在下方点选帧编辑）"), this);
    m_assetHint->setStyleSheet("color:#9aa7b4;");
    assetHostLayout->addWidget(m_assetHint);
    stateGrid->addWidget(assetHost, r, 1, 1, 2); ++r;
    stateGrid->addWidget(new QLabel(tr("纹理"), this), r, 0); stateGrid->addWidget(m_texEdit, r, 1, 1, 2); ++r;
    stateGrid->addWidget(new QLabel(tr("行/列"), this), r, 0); stateGrid->addWidget(m_rowsSpin, r, 1); stateGrid->addWidget(m_colsSpin, r, 2); ++r;
    stateGrid->addWidget(new QLabel(tr("帧宽/高"), this), r, 0); stateGrid->addWidget(m_fwSpin, r, 1); stateGrid->addWidget(m_fhSpin, r, 2); ++r;
    stateGrid->addWidget(new QLabel(tr("每帧秒"), this), r, 0); stateGrid->addWidget(m_durSpin, r, 1); stateGrid->addWidget(m_loopCheck, r, 2); ++r;
    stateGrid->addWidget(m_clearFramesBtn, r, 0, 1, 2); stateGrid->addWidget(m_framesHint, r, 2); ++r;

    auto *frameScroll = new QScrollArea(this);
    frameScroll->setWidgetResizable(true);
    frameScroll->setWidget(m_gridHost);
    frameScroll->setFixedHeight(110);

    // 转换属性
    m_fromCombo = new QComboBox(this);
    m_toCombo = new QComboBox(this);
    m_condList = new QListWidget(this);
    m_condList->setFixedHeight(70);
    auto *addCondBtn = new QToolButton(this); addCondBtn->setText(tr("+ 条件"));
    auto *delCondBtn = new QToolButton(this); delCondBtn->setText(tr("− 条件"));
    auto *condBtnRow = new QHBoxLayout;
    condBtnRow->addWidget(addCondBtn); condBtnRow->addWidget(delCondBtn); condBtnRow->addStretch();
    auto *fromToRow = new QHBoxLayout;
    fromToRow->addWidget(new QLabel(tr("从"), this)); fromToRow->addWidget(m_fromCombo, 1);
    fromToRow->addWidget(new QLabel(tr("到"), this)); fromToRow->addWidget(m_toCombo, 1);

    auto *propPanel = new QWidget(this);
    auto *propLayout = new QVBoxLayout(propPanel);
    propLayout->setContentsMargins(0, 0, 0, 0);
    propLayout->addWidget(new QLabel(tr("选中状态"), this));
    propLayout->addLayout(stateGrid);
    propLayout->addWidget(frameScroll);
    propLayout->addWidget(new QLabel(tr("点击格添加/移除帧"), this));
    propLayout->addWidget(new QLabel(tr("选中转换"), this));
    propLayout->addLayout(fromToRow);
    propLayout->addLayout(condBtnRow);
    propLayout->addWidget(m_condList);

    // 总布局
    auto *split = new QSplitter(Qt::Vertical, this);
    split->addWidget(centerSplit);
    split->addWidget(propPanel);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 0);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addLayout(toolbar);
    outer->addWidget(split, 1);

    // 连接工具栏按钮
    connect(addStateBtn, &QToolButton::clicked, this, &AnimatorDock::onAddState);
    connect(delStateBtn, &QToolButton::clicked, this, &AnimatorDock::onRemoveState);
    connect(addTransBtn, &QToolButton::clicked, this, &AnimatorDock::onAddTransition);

    // 连接 graph 信号
    connect(m_graph, &AnimatorGraphView::stateSelected, this, [this](int index) {
        m_selState = index; m_selTransition = -1;
        refreshStateWidgets(); refreshClipWidgets(); rebuildFrameGrid(); refreshTransitionWidgets();
        refreshConditionWidgets();
    });
    connect(m_graph, &AnimatorGraphView::transitionSelected, this, [this](int index) {
        m_selTransition = index; m_selState = -1;
        refreshStateWidgets(); refreshTransitionWidgets(); refreshConditionWidgets();
    });
    connect(m_graph, &AnimatorGraphView::graphChanged, this, [this] {
        emit changed();
        refresh();
    });

    // 参数连接
    connect(addParamBtn, &QToolButton::clicked, this, &AnimatorDock::onAddParam);
    connect(delParamBtn, &QToolButton::clicked, this, &AnimatorDock::onRemoveParam);
    connect(m_paramTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &AnimatorDock::onParamTypeChanged);
    connect(m_paramNameEdit, &QLineEdit::editingFinished, this, &AnimatorDock::onParamNameEdited);
    connect(m_paramValueEdit, &QLineEdit::editingFinished, this, &AnimatorDock::onParamValueEdited);
    connect(m_paramList, &QListWidget::currentRowChanged, this, &AnimatorDock::onParamRowChanged);

    // 状态属性连接
    connect(m_stateNameEdit, &QLineEdit::editingFinished, this, &AnimatorDock::onStateNameEdited);
    connect(m_stateEntryCheck, &QCheckBox::toggled, this, &AnimatorDock::onStateEntryToggled);
    // 剪辑资产（方案 A）
    connect(m_assetPathEdit, &QLineEdit::editingFinished, this, &AnimatorDock::onAssetTextEdited);
    connect(m_assetBrowseBtn, &QToolButton::clicked, this, &AnimatorDock::onAssetBrowse);
    connect(m_assetOpenBtn, &QToolButton::clicked, this, &AnimatorDock::onAssetOpenInWindow);
    connect(m_assetClearBtn, &QToolButton::clicked, this, &AnimatorDock::onAssetClear);
    connect(m_texEdit, &QLineEdit::editingFinished, this, &AnimatorDock::onClipTexEdited);
    connect(m_rowsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &AnimatorDock::onClipGridChanged);
    connect(m_colsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &AnimatorDock::onClipGridChanged);
    connect(m_fwSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimatorDock::onClipGridChanged);
    connect(m_fhSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimatorDock::onClipGridChanged);
    connect(m_durSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimatorDock::onClipDurationChanged);
    connect(m_loopCheck, &QCheckBox::toggled, this, &AnimatorDock::onClipLoopToggled);
    connect(m_clearFramesBtn, &QToolButton::clicked, this, &AnimatorDock::onClearFrames);

    // 转换连接
    connect(m_fromCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &AnimatorDock::onFromChanged);
    connect(m_toCombo, qOverload<int>(&QComboBox::currentIndexChanged), this, &AnimatorDock::onToChanged);
    connect(addCondBtn, &QToolButton::clicked, this, &AnimatorDock::onAddCondition);
    connect(delCondBtn, &QToolButton::clicked, this, &AnimatorDock::onRemoveCondition);
}

void AnimatorDock::setGameObject(Shit::GameObject *object)
{
    Shit::Animator *animator = object ? object->getComponent<Shit::Animator>() : nullptr;
    bindAnimator(animator);
}

void AnimatorDock::bindAnimator(Shit::Animator *animator)
{
    m_animator = animator;
    m_selState = -1;
    m_selTransition = -1;
    m_paramSel = -1;
    m_graph->setAnimator(animator);
    refresh();
}

void AnimatorDock::setProjectRoot(const QString &root)
{
    m_projectRoot = root;
    refreshClipWidgets();
}

// ═══════════════════════════════════════════════════════════
// 相对路径辅助（基准 = 项目根；无项目/不在项目内则原样）
// ═══════════════════════════════════════════════════════════

QString AnimatorDock::toRelativePath(const QString &abs) const
{
    if (abs.isEmpty() || m_projectRoot.isEmpty()) return abs;
    QFileInfo fileInfo(abs);
    if (!fileInfo.isAbsolute()) return abs;
    const QString rel = QDir(m_projectRoot).relativeFilePath(abs);
    // 相对路径不允许以 ".." 开头（越出项目根则保留绝对路径）
    if (rel.startsWith(QStringLiteral("../")) || rel == QStringLiteral("..")) return abs;
    return rel;
}

QString AnimatorDock::toAbsolutePath(const QString &rel) const
{
    if (rel.isEmpty() || m_projectRoot.isEmpty()) return rel;
    QFileInfo info(rel);
    if (info.isAbsolute()) return rel;
    return m_projectRoot + "/" + rel;
}

bool AnimatorDock::loadClipFromFile(const QString &path, Shit::AnimationClip &out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray data = f.readAll();
    f.close();
    try {
        nlohmann::json j = nlohmann::json::parse(data.constData());
        return out.fromJson(j);
    } catch (const std::exception &) {
        return false;
    }
}

void AnimatorDock::setSelectedStateAsset(const QString &absPath, bool withClip)
{
    if (!m_animator || m_selState < 0) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_selState);
    s.assetPath = absPath.toStdString();
    if (withClip) {
        Shit::AnimationClip clip;
        if (!absPath.isEmpty() && loadClipFromFile(absPath, clip)) s.clip = clip;
    }
    if (m_animator->setState(m_selState, s)) {
        emit changed();
        refreshClipWidgets();
        rebuildFrameGrid();
    }
}

void AnimatorDock::refresh()
{
    // 仅当 Animator 数据代数变化时才重建图（避免每帧清除/重建 QGraphicsItem）
    const uint64_t gen = m_animator ? m_animator->getDataGeneration() : 0;
    const bool graphChanged = (gen != m_cachedGeneration);
    m_cachedGeneration = gen;

    m_updating = true;
    rebuildParams();
    m_updating = false;
    if (m_animator) {
        // 若选中状态/转换索引越界则复位
        if (m_selState >= m_animator->stateCount()) m_selState = -1;
        if (m_selTransition >= m_animator->transitionCount()) m_selTransition = -1;
    } else {
        m_selState = m_selTransition = -1;
    }
    refreshStateWidgets();
    refreshTransitionWidgets();
    refreshClipWidgets();
    refreshConditionWidgets();
    if (graphChanged) {
        m_graph->rebuildGraph();
    }
}

// ═══════════════════════════════════════════════════════════
// 参数
// ═══════════════════════════════════════════════════════════

void AnimatorDock::rebuildParams()
{
    m_paramList->blockSignals(true);
    m_paramList->clear();
    const int n = m_animator ? m_animator->paramCount() : 0;
    for (int i = 0; i < n; ++i) {
        const Shit::AnimatorParameter *p = m_animator->paramAt(i);
        m_paramList->addItem(p ? QString("%1 [%2]").arg(QString::fromStdString(p->name), paramTypeName(p->type))
                               : QString("?"));
    }
    if (m_paramSel >= n) m_paramSel = -1;
    if (m_paramSel >= 0) m_paramList->setCurrentRow(m_paramSel);
    m_paramList->blockSignals(false);
    refreshParamWidgets();
}

void AnimatorDock::refreshParamWidgets()
{
    const Shit::AnimatorParameter *p = m_animator && m_paramSel >= 0 ? m_animator->paramAt(m_paramSel) : nullptr;
    const bool valid = p != nullptr;
    m_paramTypeCombo->setEnabled(valid);
    m_paramNameEdit->setEnabled(valid);
    m_paramValueEdit->setEnabled(valid);
    if (!p) return;
    m_paramTypeCombo->setCurrentIndex(static_cast<int>(p->type));
    m_paramNameEdit->setText(QString::fromStdString(p->name));
    if (p->type == Shit::AnimatorParamType::Float)
        m_paramValueEdit->setText(QString::number(p->floatValue));
    else
        m_paramValueEdit->setText(p->boolValue ? tr("true") : tr("false"));
}

void AnimatorDock::onAddParam()
{
    if (!m_animator) return;
    const int idx = m_animator->addParam("Param", Shit::AnimatorParamType::Float);
    if (idx < 0) return;
    m_paramSel = idx;
    emit changed();
    rebuildParams();
}

void AnimatorDock::onRemoveParam()
{
    if (!m_animator || m_paramSel < 0) return;
    if (m_animator->removeParam(m_paramSel)) {
        m_paramSel = -1;
        emit changed();
        rebuildParams();
    }
}

void AnimatorDock::onParamTypeChanged(int index)
{
    if (m_updating || !m_animator || m_paramSel < 0) return;
    Shit::AnimatorParameter p = *m_animator->paramAt(m_paramSel);
    p.type = static_cast<Shit::AnimatorParamType>(index);
    if (m_animator->setParam(m_paramSel, p)) { emit changed(); rebuildParams(); }
}

void AnimatorDock::onParamNameEdited()
{
    if (!m_animator || m_paramSel < 0) return;
    Shit::AnimatorParameter p = *m_animator->paramAt(m_paramSel);
    const std::string name = m_paramNameEdit->text().trimmed().toStdString();
    if (name.empty()) { rebuildParams(); return; }
    p.name = name;
    if (m_animator->setParam(m_paramSel, p)) { emit changed(); rebuildParams(); }
}

void AnimatorDock::onParamValueEdited()
{
    if (!m_animator || m_paramSel < 0) return;
    Shit::AnimatorParameter p = *m_animator->paramAt(m_paramSel);
    if (p.type == Shit::AnimatorParamType::Float) {
        bool ok = false;
        const float f = m_paramValueEdit->text().trimmed().toFloat(&ok);
        if (ok) p.floatValue = f;
    } else {
        p.boolValue = (m_paramValueEdit->text().trimmed().compare("true", Qt::CaseInsensitive) == 0);
    }
    if (m_animator->setParam(m_paramSel, p)) emit changed();
}

void AnimatorDock::onParamRowChanged(int row)
{
    if (m_updating) return;
    if (row < 0) return;
    m_paramSel = row;
    refreshParamWidgets();
}

	// ═══════════════════════════════════════════════════════════
	// 工具栏按钮
	// ═══════════════════════════════════════════════════════════

	void AnimatorDock::onAddState()
	{
		if (!m_animator) return;
		const int idx = m_animator->addState("State");
		if (idx < 0) return;
		emit changed();
		m_selState = idx;
		m_selTransition = -1;
		m_graph->rebuildGraph();
		// 属性面板跟随新状态
		refreshStateWidgets(); refreshClipWidgets(); rebuildFrameGrid(); refreshTransitionWidgets();
		refreshConditionWidgets();
	}

	void AnimatorDock::onRemoveState()
	{
		if (!m_animator || m_selState < 0) return;
		if (m_animator->removeState(m_selState)) {
			m_selState = -1;
			emit changed();
			m_graph->rebuildGraph();
			refreshStateWidgets(); refreshClipWidgets(); rebuildFrameGrid(); refreshTransitionWidgets();
			refreshConditionWidgets();
		}
	}

	void AnimatorDock::onAddTransition()
	{
		if (!m_animator) return;
		// 默认从当前选中状态到第一个状态（或 0→1）
		const int from = (m_selState >= 0) ? m_selState : 0;
		const int to = (from + 1 < m_animator->stateCount()) ? from + 1 : 0;
		if (to >= m_animator->stateCount()) return;
		const int idx = m_animator->addTransition(from, to);
		if (idx < 0) return;
		emit changed();
		m_selTransition = idx;
		m_selState = -1;
		m_graph->rebuildGraph();
		refreshTransitionWidgets(); refreshConditionWidgets();
		refreshStateWidgets();
	}

	void AnimatorDock::onRemoveTransition()
	{
		if (!m_animator || m_selTransition < 0) return;
		if (m_animator->removeTransition(m_selTransition)) {
			m_selTransition = -1;
			emit changed();
			m_graph->rebuildGraph();
			refreshTransitionWidgets(); refreshConditionWidgets();
		}
	}

	// ═══════════════════════════════════════════════════════════
	// 状态属性
	// ═══════════════════════════════════════════════════════════

void AnimatorDock::refreshStateWidgets()
{
    const Shit::AnimatorState *s = m_animator && m_selState >= 0 ? m_animator->stateAt(m_selState) : nullptr;
    const bool valid = s != nullptr;
    m_stateNameEdit->setEnabled(valid);
    m_stateEntryCheck->setEnabled(valid);
    if (s) {
        m_stateNameEdit->setText(QString::fromStdString(s->name));
        m_stateEntryCheck->setChecked(s->isEntry);
    }
}

void AnimatorDock::refreshClipWidgets()
{
    const Shit::AnimatorState *s = m_animator && m_selState >= 0 ? m_animator->stateAt(m_selState) : nullptr;
    const bool valid = s != nullptr;
    const bool hasAsset = s && !s->assetPath.empty();
    // 剪辑资产行
    m_assetPathEdit->setEnabled(valid);
    m_assetBrowseBtn->setEnabled(valid);
    m_assetOpenBtn->setEnabled(valid && hasAsset);
    m_assetClearBtn->setEnabled(valid && hasAsset);
    if (!s) {
        m_assetHint->setText(tr("内嵌剪辑（在下方点选帧编辑）"));
    } else if (hasAsset) {
        m_assetHint->setText(tr("本状态由 .anim 资产驱动，下方内嵌剪辑禁用"));
    } else {
        m_assetHint->setText(tr("内嵌剪辑（在下方点选帧编辑）"));
    }
    if (!m_assetPathEdit->hasFocus())
        m_assetPathEdit->setText(hasAsset ? toRelativePath(QString::fromStdString(s->assetPath)) : QString());
    // 内嵌剪辑编辑区：有资产则禁用
    const bool inlineEnabled = valid && !hasAsset;
    for (QWidget *w : std::initializer_list<QWidget*>{
             m_texEdit, m_rowsSpin, m_colsSpin, m_fwSpin, m_fhSpin, m_durSpin })
        w->setEnabled(inlineEnabled);
    m_loopCheck->setEnabled(inlineEnabled);
    m_clearFramesBtn->setEnabled(inlineEnabled);
    if (!s) { m_framesHint->setText(tr("在图中选中状态后编辑剪辑")); return; }
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

void AnimatorDock::writeClipFromWidgets()
{
    if (!m_animator || m_selState < 0) return;
    const Shit::AnimatorState *cur = m_animator->stateAt(m_selState);
    if (cur && !cur->assetPath.empty()) return;  // 资产驱动：内嵌剪辑被锁定
    Shit::AnimatorState s = *m_animator->stateAt(m_selState);
    s.clip.texturePath = m_texEdit->text().trimmed().toStdString();
    s.clip.rows = m_rowsSpin->value();
    s.clip.cols = m_colsSpin->value();
    s.clip.frameWidth = static_cast<float>(m_fwSpin->value());
    s.clip.frameHeight = static_cast<float>(m_fhSpin->value());
    s.clip.duration = static_cast<float>(m_durSpin->value());
    s.clip.loop = m_loopCheck->isChecked();
    if (m_animator->setState(m_selState, s))
        emit changed();
}

void AnimatorDock::onStateNameEdited()
{
    if (!m_animator || m_selState < 0) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_selState);
    const std::string name = m_stateNameEdit->text().trimmed().toStdString();
    if (name.empty()) { refresh(); return; }
    s.name = name;
    if (m_animator->setState(m_selState, s)) { emit changed(); m_graph->rebuildGraph(); }
}

void AnimatorDock::onStateEntryToggled(bool on)
{
    if (m_updating || !m_animator || m_selState < 0) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_selState);
    if (s.isEntry == on) return;
    s.isEntry = on;
    if (m_animator->setState(m_selState, s)) { emit changed(); m_graph->rebuildGraph(); }
}

void AnimatorDock::onClipTexEdited() { writeClipFromWidgets(); rebuildFrameGrid(); }
void AnimatorDock::onClipGridChanged() { writeClipFromWidgets(); rebuildFrameGrid(); }
void AnimatorDock::onClipDurationChanged(double) { writeClipFromWidgets(); }
void AnimatorDock::onClipLoopToggled(bool) { writeClipFromWidgets(); }

// ═══════════════════════════════════════════════════════════
// 剪辑资产（方案 A：Animator 状态引用 .anim 资产）
// ═══════════════════════════════════════════════════════════

void AnimatorDock::onAssetTextEdited()
{
    if (!m_animator || m_selState < 0) return;
    const QString text = m_assetPathEdit->text().trimmed();
    QString abs = toAbsolutePath(text);
    if (abs.isEmpty()) {
        // 清空资产 → 回到内嵌剪辑（clip 保留）
        Shit::AnimatorState s = *m_animator->stateAt(m_selState);
        s.assetPath.clear();
        if (m_animator->setState(m_selState, s)) { emit changed(); refreshClipWidgets(); rebuildFrameGrid(); }
        return;
    }
    if (!QFileInfo::exists(abs)) { refreshClipWidgets(); return; }  // 路径无效，回退显示
    setSelectedStateAsset(abs, /*withClip=*/true);
}

void AnimatorDock::onAssetBrowse()
{
    if (!m_animator || m_selState < 0) return;
    QString initialDir = m_projectRoot.isEmpty() ? QString() : m_projectRoot + "/Assets";
    const QString path = QFileDialog::getOpenFileName(this, tr("选择动画剪辑资产"), initialDir,
                                                      tr("ShitEngine 动画 (*.anim)"));
    if (path.isEmpty()) return;
    setSelectedStateAsset(QFileInfo(path).absoluteFilePath(), /*withClip=*/true);
}

void AnimatorDock::onAssetOpenInWindow()
{
    if (!m_animator || m_selState < 0) return;
    const Shit::AnimatorState *s = m_animator->stateAt(m_selState);
    if (!s || s->assetPath.empty()) return;
    const QString abs = toAbsolutePath(QString::fromStdString(s->assetPath));
    if (!QFileInfo::exists(abs)) {
        QMessageBox::warning(this, tr("资产缺失"), tr("无法打开 .anim 资产：%1").arg(abs));
        return;
    }
    emit openAssetRequested(abs);
}

void AnimatorDock::onAssetClear()
{
    if (!m_animator || m_selState < 0) return;
    const Shit::AnimatorState *s = m_animator->stateAt(m_selState);
    if (!s || s->assetPath.empty()) return;
    Shit::AnimatorState ns = *s;
    ns.assetPath.clear();
    if (m_animator->setState(m_selState, ns)) { emit changed(); refreshClipWidgets(); rebuildFrameGrid(); }
}

void AnimatorDock::dragEnterEvent(QDragEnterEvent *event)
{
    // 仅当拖入内容含 .anim 文件 URL 且当前选中状态时接受
    if (m_animator && m_selState >= 0 && event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls()) {
            if (QFileInfo(url.toLocalFile()).suffix().compare(QStringLiteral("anim"), Qt::CaseInsensitive) == 0) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void AnimatorDock::dropEvent(QDropEvent *event)
{
    if (!m_animator || m_selState < 0 || !event->mimeData()->hasUrls()) {
        event->ignore();
        return;
    }
    for (const QUrl &url : event->mimeData()->urls()) {
        const QString p = url.toLocalFile();
        if (QFileInfo(p).suffix().compare(QStringLiteral("anim"), Qt::CaseInsensitive) == 0) {
            setSelectedStateAsset(QFileInfo(p).absoluteFilePath(), /*withClip=*/true);
            event->acceptProposedAction();
            return;
        }
    }
    event->ignore();
}

void AnimatorDock::onClearFrames()
{
    if (!m_animator || m_selState < 0) return;
    const Shit::AnimatorState *cur = m_animator->stateAt(m_selState);
    if (cur && !cur->assetPath.empty()) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_selState);
    s.clip.frames.clear();
    if (m_animator->setState(m_selState, s)) { emit changed(); refreshFrameHighlights(); m_framesHint->setText(tr("帧数 0")); }
}

void AnimatorDock::onFrameClicked(int tileId)
{
    if (!m_animator || m_selState < 0) return;
    const Shit::AnimatorState *cur = m_animator->stateAt(m_selState);
    if (cur && !cur->assetPath.empty()) return;
    Shit::AnimatorState s = *m_animator->stateAt(m_selState);
    auto it = std::find(s.clip.frames.begin(), s.clip.frames.end(), tileId);
    if (it != s.clip.frames.end()) s.clip.frames.erase(it);
    else s.clip.frames.push_back(tileId);
    if (m_animator->setState(m_selState, s)) { emit changed(); refreshFrameHighlights(); m_framesHint->setText(tr("帧数 %1").arg(s.clip.frames.size())); }
}

void AnimatorDock::rebuildFrameGrid()
{
    const Shit::AnimatorState *s = m_animator && m_selState >= 0 ? m_animator->stateAt(m_selState) : nullptr;
    // 资产驱动状态不提供内嵌点选帧
    if (s && !s->assetPath.empty()) { clearFrameGrid(); return; }
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

void AnimatorDock::clearFrameGrid()
{
    if (QLayout *old = m_gridHost->layout()) {
        while (auto *item = old->takeAt(0))
            if (QWidget *w = item->widget()) delete w;
        delete old;
    }
    m_frameButtons.clear();
}

void AnimatorDock::refreshFrameHighlights()
{
    const Shit::AnimatorState *s = m_animator && m_selState >= 0 ? m_animator->stateAt(m_selState) : nullptr;
    std::set<int> used;
    if (s) used.insert(s->clip.frames.begin(), s->clip.frames.end());
    for (auto &[id, btn] : m_frameButtons)
        btn->setChecked(used.count(id) > 0);
}

// ═══════════════════════════════════════════════════════════
// 转换属性
// ═══════════════════════════════════════════════════════════

void AnimatorDock::refreshTransitionWidgets()
{
    const Shit::AnimatorTransition *t = m_animator && m_selTransition >= 0 ? m_animator->transitionAt(m_selTransition) : nullptr;
    const bool valid = t != nullptr;
    m_fromCombo->setEnabled(valid);
    m_toCombo->setEnabled(valid);
    m_condList->setEnabled(valid);
    if (!valid) { m_fromCombo->clear(); m_toCombo->clear(); return; }
    // 重建下拉
    m_fromCombo->blockSignals(true);
    m_toCombo->blockSignals(true);
    m_fromCombo->clear(); m_toCombo->clear();
    m_fromCombo->addItem(tr("*（任意）"));
    const int n = m_animator->stateCount();
    for (int i = 0; i < n; ++i) {
        const Shit::AnimatorState *s = m_animator->stateAt(i);
        const QString name = s ? QString::fromStdString(s->name) : QString("State %1").arg(i);
        m_fromCombo->addItem(name);
        m_toCombo->addItem(name);
    }
    m_fromCombo->setCurrentIndex(t->fromState + 1);
    m_toCombo->setCurrentIndex(t->toState + 1);
    m_fromCombo->blockSignals(false);
    m_toCombo->blockSignals(false);
}

void AnimatorDock::refreshConditionWidgets()
{
    m_condList->blockSignals(true);
    m_condList->clear();
    const Shit::AnimatorTransition *t = m_animator && m_selTransition >= 0 ? m_animator->transitionAt(m_selTransition) : nullptr;
    if (t) {
        for (const auto &c : t->conditions)
            m_condList->addItem(QString("%1 %2 %3").arg(QString::fromStdString(c.parameter), condTypeName(c.type),
                c.type == Shit::AnimatorConditionType::Bool ? (c.boolValue ? tr("true") : tr("false"))
                    : QString::number(c.threshold)));
    }
    m_condList->blockSignals(false);
}

void AnimatorDock::onFromChanged(int index)
{
    if (m_updating || !m_animator || m_selTransition < 0) return;
    Shit::AnimatorTransition t = *m_animator->transitionAt(m_selTransition);
    t.fromState = index - 1;
    if (m_animator->setTransition(m_selTransition, t)) { emit changed(); m_graph->rebuildGraph(); }
}

void AnimatorDock::onToChanged(int index)
{
    if (m_updating || !m_animator || m_selTransition < 0) return;
    Shit::AnimatorTransition t = *m_animator->transitionAt(m_selTransition);
    t.toState = index - 1;
    if (m_animator->setTransition(m_selTransition, t)) { emit changed(); m_graph->rebuildGraph(); }
}

void AnimatorDock::onAddCondition()
{
    if (!m_animator || m_selTransition < 0 || m_animator->paramCount() == 0) return;
    Shit::AnimatorTransition t = *m_animator->transitionAt(m_selTransition);
    Shit::AnimatorTransitionCondition c;
    c.parameter = m_animator->paramAt(0)->name;
    c.type = Shit::AnimatorConditionType::FloatGt;
    c.threshold = 0.5f;
    t.conditions.push_back(c);
    if (m_animator->setTransition(m_selTransition, t)) { emit changed(); refreshConditionWidgets(); }
}

void AnimatorDock::onRemoveCondition()
{
    if (!m_animator || m_selTransition < 0 || m_condList->currentRow() < 0) return;
    Shit::AnimatorTransition t = *m_animator->transitionAt(m_selTransition);
    const int idx = m_condList->currentRow();
    if (idx >= static_cast<int>(t.conditions.size())) return;
    t.conditions.erase(t.conditions.begin() + idx);
    if (m_animator->setTransition(m_selTransition, t)) { emit changed(); refreshConditionWidgets(); }
}
