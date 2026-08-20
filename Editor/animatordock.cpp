#include "animatordock.h"
#include "animatorgraphview.h"

#include <ShitEngine/Animation/Animator.h>
#include <ShitEngine/GameObject/GameObject.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QMessageBox>
#include <QSplitter>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <nlohmann/json.hpp>

#include <algorithm>

namespace {

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
    setAcceptDrops(true);   // 支持 .anim 资产拖入剪辑资产行
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

    // ── 底部属性面板（状态/转换） ──
    // 状态属性：名称 / 入口 / .anim 剪辑资产（Unity 语义，必选）
    m_stateNameEdit = new QLineEdit(this);
    m_stateEntryCheck = new QCheckBox(tr("入口状态"), this);

    auto *stateGrid = new QGridLayout;
    int r = 0;
    stateGrid->addWidget(new QLabel(tr("名称"), this), r, 0); stateGrid->addWidget(m_stateNameEdit, r, 1); stateGrid->addWidget(m_stateEntryCheck, r, 2); ++r;
    // 剪辑资产行（引用的 .anim 文件——状态剪辑的唯一来源）
    m_assetPathEdit = new QLineEdit(this);
    m_assetPathEdit->setPlaceholderText(tr("必选：.anim 资产路径"));
    m_assetPathEdit->setAcceptDrops(false);
    m_assetBrowseBtn = new QToolButton(this); m_assetBrowseBtn->setText(tr("浏览…"));
    m_assetOpenBtn = new QToolButton(this); m_assetOpenBtn->setText(tr("打开"));
    auto *assetBtnRow = new QHBoxLayout;
    assetBtnRow->setContentsMargins(0, 0, 0, 0);
    assetBtnRow->addWidget(m_assetPathEdit, 1);
    assetBtnRow->addWidget(m_assetBrowseBtn);
    assetBtnRow->addWidget(m_assetOpenBtn);
    stateGrid->addWidget(new QLabel(tr("剪辑资产"), this), r, 0);
    auto *assetHost = new QWidget(this);
    auto *assetHostLayout = new QVBoxLayout(assetHost);
    assetHostLayout->setContentsMargins(0, 0, 0, 0);
    assetHostLayout->addLayout(assetBtnRow);
    m_assetHint = new QLabel(tr("请选择 .anim 剪辑资产"), this);
    m_assetHint->setStyleSheet("color:#9aa7b4;");
    assetHostLayout->addWidget(m_assetHint);
    stateGrid->addWidget(assetHost, r, 1, 1, 2); ++r;

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
        refreshStateWidgets(); refreshClipWidgets(); refreshTransitionWidgets();
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
    // 剪辑资产（Unity 语义：状态只引用 .anim 文件）
    connect(m_assetPathEdit, &QLineEdit::editingFinished, this, &AnimatorDock::onAssetTextEdited);
    connect(m_assetBrowseBtn, &QToolButton::clicked, this, &AnimatorDock::onAssetBrowse);
    connect(m_assetOpenBtn, &QToolButton::clicked, this, &AnimatorDock::onAssetOpenInWindow);

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
    if (withClip) {
        // 资产是状态剪辑的唯一来源；空路径 = 清除资产
        Shit::AnimationClip clip;
        if (absPath.isEmpty()) {
            s.clip = Shit::AnimationClip{};
        } else if (!loadClipFromFile(absPath, clip)) {
            // P33：读取/解析失败不再静默（此前保持旧状态无任何提示，用户以为已绑定）
            QMessageBox::warning(this, tr("动画资产"),
                tr("无法读取 .anim 资产：\n%1\n\n文件可能已损坏、缺失或格式不受支持。").arg(absPath));
            return;
        } else {
            s.clip = clip;
        }
    }
    s.assetPath = absPath.toStdString();
    if (m_animator->setState(m_selState, s)) {
        emit changed();
        refreshClipWidgets();
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
	refreshStateWidgets(); refreshClipWidgets(); refreshTransitionWidgets();
	refreshConditionWidgets();
}

void AnimatorDock::onRemoveState()
{
	if (!m_animator || m_selState < 0) return;
	if (m_animator->removeState(m_selState)) {
		m_selState = -1;
		emit changed();
		m_graph->rebuildGraph();
		refreshStateWidgets(); refreshClipWidgets(); refreshTransitionWidgets();
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
    const bool hasAsset = s && !s->assetPath.empty();
    m_assetPathEdit->setEnabled(s != nullptr);
    m_assetBrowseBtn->setEnabled(s != nullptr);
    m_assetOpenBtn->setEnabled(hasAsset);
    if (!s) {
        m_assetPathEdit->clear();
        m_assetHint->setText(tr("在图中选中状态"));
    } else if (hasAsset) {
        m_assetHint->setText(tr("本状态剪辑来源：.anim 资产（可在 Animation 窗口编辑）"));
        if (!m_assetPathEdit->hasFocus())
            m_assetPathEdit->setText(toRelativePath(QString::fromStdString(s->assetPath)));
    } else {
        m_assetHint->setText(tr("请选择 .anim 剪辑资产（必选）"));
        if (!m_assetPathEdit->hasFocus())
            m_assetPathEdit->clear();
    }
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

// ═══════════════════════════════════════════════════════════
// 剪辑资产（Unity 语义：状态只引用 .anim 文件）
// ═══════════════════════════════════════════════════════════

void AnimatorDock::onAssetTextEdited()
{
    if (!m_animator || m_selState < 0) return;
    const QString text = m_assetPathEdit->text().trimmed();
    if (text.isEmpty()) {
        // 资产必选：空路径不解除绑定（保持原资产，回退显示旧值）
        refreshClipWidgets();
        return;
    }
    const QString abs = toAbsolutePath(text);
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
