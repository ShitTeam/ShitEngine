#include "animatoreditorwidget.h"

#include <ShitEngine/Component/AnimationComponent.h>
#include <ShitEngine/Component/SpriteRenderer.h>
#include <ShitEngine/GameObject/GameObject.h>

#include <QApplication>
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

} // namespace

AnimatorEditorWidget::AnimatorEditorWidget(Shit::AnimationComponent *comp, QWidget *parent)
    : QWidget(parent)
    , m_comp(comp)
{
    // ── 剪辑选择行 ──
    m_clipBox = new QComboBox(this);
    m_addBtn = new QToolButton(this);
    m_addBtn->setText(tr("+"));
    m_addBtn->setToolTip(tr("新增剪辑"));
    m_removeBtn = new QToolButton(this);
    m_removeBtn->setText(tr("−"));
    m_removeBtn->setToolTip(tr("删除选中剪辑"));

    auto *clipRow = new QHBoxLayout;
    clipRow->setContentsMargins(0, 0, 0, 0);
    clipRow->addWidget(m_clipBox, 1);
    clipRow->addWidget(m_addBtn);
    clipRow->addWidget(m_removeBtn);

    // ── 参数编辑区 ──
    m_nameEdit = new QLineEdit(this);
    m_texEdit = new QLineEdit(this);
    m_texEdit->setPlaceholderText(tr("纹理路径（留空用对象 SpriteRenderer）"));
    m_rowsSpin = new QSpinBox(this);
    m_rowsSpin->setRange(1, 256);
    m_colsSpin = new QSpinBox(this);
    m_colsSpin->setRange(1, 256);
    m_fwSpin = new QDoubleSpinBox(this);
    m_fwSpin->setRange(1, 4096);
    m_fwSpin->setDecimals(1);
    m_fhSpin = new QDoubleSpinBox(this);
    m_fhSpin->setRange(1, 4096);
    m_fhSpin->setDecimals(1);
    m_durSpin = new QDoubleSpinBox(this);
    m_durSpin->setRange(0.001, 60.0);
    m_durSpin->setDecimals(3);
    m_durSpin->setSingleStep(0.01);
    m_loopCheck = new QCheckBox(tr("循环"), this);
    m_defaultBtn = new QToolButton(this);
    m_defaultBtn->setText(tr("设为默认播放"));
    m_defaultBtn->setCheckable(true);
    m_clearFramesBtn = new QToolButton(this);
    m_clearFramesBtn->setText(tr("清空帧"));
    m_framesHint = new QLabel(this);
    m_framesHint->setWordWrap(true);
    m_framesHint->setStyleSheet("color:#9aa7b4;");

    auto *grid = new QGridLayout;
    grid->setContentsMargins(0, 0, 0, 0);
    int r = 0;
    grid->addWidget(new QLabel(tr("名称"), this), r, 0);
    grid->addWidget(m_nameEdit, r, 1, 1, 2);
    ++r;
    grid->addWidget(new QLabel(tr("纹理"), this), r, 0);
    grid->addWidget(m_texEdit, r, 1, 1, 2);
    ++r;
    grid->addWidget(new QLabel(tr("行/列"), this), r, 0);
    grid->addWidget(m_rowsSpin, r, 1);
    grid->addWidget(m_colsSpin, r, 2);
    ++r;
    grid->addWidget(new QLabel(tr("帧宽/高"), this), r, 0);
    grid->addWidget(m_fwSpin, r, 1);
    grid->addWidget(m_fhSpin, r, 2);
    ++r;
    grid->addWidget(new QLabel(tr("每帧秒"), this), r, 0);
    grid->addWidget(m_durSpin, r, 1);
    grid->addWidget(m_loopCheck, r, 2);
    ++r;
    grid->addWidget(m_defaultBtn, r, 0, 1, 3);
    ++r;
    grid->addWidget(m_framesHint, r, 0, 1, 3);

    // ── 帧预览区 ──
    m_gridHost = new QWidget(this);
    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setWidget(m_gridHost);
    scroll->setFixedHeight(120);

    // ── 底部：清空帧 + 预览 ──
    m_previewBtn = new QToolButton(this);
    m_previewBtn->setText(tr("▶ 播放"));
    m_previewBtn->setCheckable(true);
    auto *bottomRow = new QHBoxLayout;
    bottomRow->addWidget(m_clearFramesBtn);
    bottomRow->addWidget(m_previewBtn, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    layout->addLayout(clipRow);
    layout->addLayout(grid);
    layout->addWidget(new QLabel(tr("点击格添加/移除帧（已选高亮，序号=顺序）"), this));
    layout->addWidget(scroll);
    layout->addLayout(bottomRow);

    connect(m_clipBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &AnimatorEditorWidget::onClipIndexChanged);
    connect(m_addBtn, &QToolButton::clicked, this, &AnimatorEditorWidget::onAddClip);
    connect(m_removeBtn, &QToolButton::clicked, this, &AnimatorEditorWidget::onRemoveClip);
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &AnimatorEditorWidget::onClipNameEdited);
    connect(m_texEdit, &QLineEdit::editingFinished, this, &AnimatorEditorWidget::onTextureEdited);
    connect(m_rowsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &AnimatorEditorWidget::onGridParamChanged);
    connect(m_colsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &AnimatorEditorWidget::onGridParamChanged);
    connect(m_fwSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimatorEditorWidget::onGridParamChanged);
    connect(m_fhSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimatorEditorWidget::onGridParamChanged);
    connect(m_durSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimatorEditorWidget::onDurationChanged);
    connect(m_loopCheck, &QCheckBox::toggled, this, &AnimatorEditorWidget::onLoopToggled);
    connect(m_defaultBtn, &QToolButton::clicked, this, &AnimatorEditorWidget::onSetDefault);
    connect(m_clearFramesBtn, &QToolButton::clicked, this, &AnimatorEditorWidget::onClearFrames);
    connect(m_previewBtn, &QToolButton::clicked, this, &AnimatorEditorWidget::onPreviewToggle);

    refresh();
}

void AnimatorEditorWidget::refresh()
{
    if (!m_comp) return;
    m_updating = true;

    const int count = m_comp->clipCount();
    const bool listChanged = (m_clipBox->count() != count)
        || (count > 0 && (m_selectedClip < 0 || m_selectedClip >= count));
    if (listChanged)
        rebuildClipList();   // 剪辑增删 → 重建下拉（保留当前选中）
    else
        refreshClipNames();  // 数量未变 → 只同步下拉文本（如剪辑改名）

    refreshParams();
    rebuildFrameGrid();
    m_updating = false;
}

// ═══════════════════════════════════════════════════════════
// 内部构建
// ═══════════════════════════════════════════════════════════

void AnimatorEditorWidget::rebuildClipList()
{
    if (!m_comp) return;
    m_clipBox->blockSignals(true);
    m_clipBox->clear();
    const int count = m_comp->clipCount();
    for (int i = 0; i < count; ++i) {
        const Shit::AnimationClip *c = m_comp->clipAt(i);
        m_clipBox->addItem(c ? QString::fromStdString(c->name) : QString("Clip %1").arg(i));
    }
    if (count == 0) {
        m_clipBox->addItem(tr("（无剪辑）"));
        m_selectedClip = -1;
    } else {
        // 保留旧选中（若越界回落到最后一个合法索引）
        if (m_selectedClip >= count) m_selectedClip = count - 1;
        if (m_selectedClip < 0) m_selectedClip = 0;
        m_clipBox->setCurrentIndex(m_selectedClip);
    }
    m_clipBox->blockSignals(false);
}

/// 仅同步下拉里的剪辑名文本（不清空、不改变选中）
void AnimatorEditorWidget::refreshClipNames()
{
    if (!m_comp) return;
    m_clipBox->blockSignals(true);
    const int count = m_comp->clipCount();
    if (count > 0 && m_selectedClip >= 0 && m_selectedClip < count) {
        for (int i = 0; i < count; ++i) {
            const Shit::AnimationClip *c = m_comp->clipAt(i);
            if (c && m_clipBox->itemText(i) != QString::fromStdString(c->name))
                m_clipBox->setItemText(i, QString::fromStdString(c->name));
        }
    }
    m_clipBox->blockSignals(false);
}

void AnimatorEditorWidget::refreshParams()
{
    m_addBtn->setEnabled(m_comp != nullptr);
    m_removeBtn->setEnabled(m_comp && m_selectedClip >= 0);
    m_previewBtn->setEnabled(m_comp && m_selectedClip >= 0);
    applyClipToWidgets();
}

void AnimatorEditorWidget::applyClipToWidgets()
{
    const Shit::AnimationClip *c = m_comp && m_selectedClip >= 0 ? m_comp->clipAt(m_selectedClip) : nullptr;
    const bool valid = c != nullptr;
    const std::initializer_list<QWidget *> editWidgets = { m_nameEdit, m_texEdit, m_rowsSpin, m_colsSpin,
                                                           m_fwSpin, m_fhSpin, m_durSpin };
    for (QWidget *w : editWidgets)
        w->setEnabled(valid);
    m_loopCheck->setEnabled(valid);
    m_defaultBtn->setEnabled(valid);
    m_clearFramesBtn->setEnabled(valid);
    if (!valid) {
        m_framesHint->setText(tr("添加剪辑后编辑"));
        return;
    }
    // 控件若正被用户编辑（有焦点）则跳过回写，避免每帧回读打断输入
    if (!m_nameEdit->hasFocus()) m_nameEdit->setText(QString::fromStdString(c->name));
    if (!m_texEdit->hasFocus()) m_texEdit->setText(QString::fromStdString(c->texturePath));
    if (!m_rowsSpin->hasFocus()) m_rowsSpin->setValue(c->rows);
    if (!m_colsSpin->hasFocus()) m_colsSpin->setValue(c->cols);
    if (!m_fwSpin->hasFocus()) m_fwSpin->setValue(c->frameWidth);
    if (!m_fhSpin->hasFocus()) m_fhSpin->setValue(c->frameHeight);
    if (!m_durSpin->hasFocus()) m_durSpin->setValue(c->duration);
    m_loopCheck->setChecked(c->loop);
    m_defaultBtn->setChecked(c->isDefault);
    const int def = m_comp->defaultClipIndex();
    m_defaultBtn->setText(c->isDefault ? tr("默认播放") : tr("设为默认播放"));
    m_framesHint->setText(tr("帧数 %1").arg(c->frames.size()));
}

void AnimatorEditorWidget::rebuildFrameGrid()
{
    const Shit::AnimationClip *c = m_comp && m_selectedClip >= 0 ? m_comp->clipAt(m_selectedClip) : nullptr;

    // 无有效剪辑/参数 → 清空网格
    const bool haveValid = c && c->cols > 0 && c->rows > 0 && c->frameWidth > 0 && c->frameHeight > 0;
    if (!haveValid) {
        clearFrameGrid();
        return;
    }

    // 计算帧网格签名：纹理路径 + 网格参数。签名未变 → 只刷新高亮，避免每帧重建
    const QString texPath = resolveAssetPath(QString::fromStdString(c->texturePath));
    const QString sig = QString("%1|%2x%3|%4x%5")
                            .arg(texPath).arg(c->rows).arg(c->cols)
                            .arg(c->frameWidth).arg(c->frameHeight);
    if (m_frameGridSig == sig && !m_frameButtons.empty()) {
        refreshFrameHighlights();
        return;
    }

    clearFrameGrid();

    QImage sheet(texPath);
    if (sheet.isNull()) {
        m_frameGridSig.clear();
        return;
    }

    const int tileW = static_cast<int>(c->frameWidth);
    const int tileH = static_cast<int>(c->frameHeight);
    const int tilesPerRow = sheet.width() / tileW;
    const int tileCount = (sheet.width() / tileW) * (sheet.height() / tileH);
    if (tilesPerRow <= 0 || tileCount <= 0) {
        m_frameGridSig.clear();
        return;
    }

    auto *grid = new QGridLayout(m_gridHost);
    grid->setContentsMargins(2, 2, 2, 2);
    grid->setSpacing(2);
    const int maxCols = 10;
    const int previewW = qMax(20, qMin(tileW, 56));
    const int previewH = qMax(20, qMin(tileH, 56));

    for (int id = 0; id < tileCount; ++id) {
        const int sx = (id % tilesPerRow) * tileW;
        const int sy = (id / tilesPerRow) * tileH;
        QImage tileImg = sheet.copy(sx, sy, tileW, tileH)
                             .scaled(previewW, previewH, Qt::KeepAspectRatio, Qt::FastTransformation);
        auto *btn = new QToolButton(m_gridHost);
        btn->setIcon(QIcon(QPixmap::fromImage(tileImg)));
        btn->setIconSize(tileImg.size());
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

void AnimatorEditorWidget::clearFrameGrid()
{
    if (QLayout *old = m_gridHost->layout()) {
        while (auto *item = old->takeAt(0))
            if (QWidget *w = item->widget()) delete w;
        delete old;
    }
    m_frameButtons.clear();
}

void AnimatorEditorWidget::refreshFrameHighlights()
{
    const Shit::AnimationClip *c = m_comp && m_selectedClip >= 0 ? m_comp->clipAt(m_selectedClip) : nullptr;
    std::set<int> used;
    if (c) used.insert(c->frames.begin(), c->frames.end());
    for (auto &[id, btn] : m_frameButtons)
        btn->setChecked(used.count(id) > 0);
}

// ═══════════════════════════════════════════════════════════
// 槽
// ═══════════════════════════════════════════════════════════

void AnimatorEditorWidget::onClipIndexChanged(int index)
{
    if (m_updating) return;
    if (index < 0) return;
    m_selectedClip = index;
    refreshParams();
    rebuildFrameGrid();
}

void AnimatorEditorWidget::onAddClip()
{
    if (!m_comp) return;
    const int idx = m_comp->addClip(QString("Clip").toStdString());
    if (idx < 0) return;
    m_selectedClip = idx;
    emit changed();
    refresh();
}

void AnimatorEditorWidget::onRemoveClip()
{
    if (!m_comp || m_selectedClip < 0) return;
    if (m_comp->removeClip(m_selectedClip)) {
        emit changed();
        refresh();
    }
}

void AnimatorEditorWidget::onClipNameEdited()
{
    if (!m_comp || m_selectedClip < 0) return;
    Shit::AnimationClip clip = *m_comp->clipAt(m_selectedClip);
    clip.name = m_nameEdit->text().trimmed().toStdString();
    if (clip.name.empty()) { refresh(); return; }
    if (m_comp->setClip(m_selectedClip, clip)) {
        emit changed();
        refresh();
    }
}

void AnimatorEditorWidget::onTextureEdited()
{
    writeWidgetsToClip();
    rebuildFrameGrid();
}

void AnimatorEditorWidget::onGridParamChanged()
{
    writeWidgetsToClip();
    rebuildFrameGrid();
}

void AnimatorEditorWidget::onDurationChanged(double)
{
    writeWidgetsToClip();
}

void AnimatorEditorWidget::onLoopToggled(bool)
{
    writeWidgetsToClip();
}

void AnimatorEditorWidget::onSetDefault()
{
    if (!m_comp || m_selectedClip < 0) return;
    if (m_comp->setDefaultClip(m_selectedClip)) {
        emit changed();
        refresh();
    }
}

void AnimatorEditorWidget::onFrameClicked(int tileId)
{
    if (!m_comp || m_selectedClip < 0) return;
    Shit::AnimationClip clip = *m_comp->clipAt(m_selectedClip);
    auto it = std::find(clip.frames.begin(), clip.frames.end(), tileId);
    if (it != clip.frames.end())
        clip.frames.erase(it);          // 已在序列中 → 移除
    else
        clip.frames.push_back(tileId);  // 追加到末尾（连续帧按点击顺序）
    if (m_comp->setClip(m_selectedClip, clip)) {
        emit changed();
        refreshFrameHighlights();
        m_framesHint->setText(tr("帧数 %1").arg(clip.frames.size()));
    }
}

void AnimatorEditorWidget::onClearFrames()
{
    if (!m_comp || m_selectedClip < 0) return;
    Shit::AnimationClip clip = *m_comp->clipAt(m_selectedClip);
    clip.frames.clear();
    if (m_comp->setClip(m_selectedClip, clip)) {
        emit changed();
        refreshFrameHighlights();
        m_framesHint->setText(tr("帧数 0"));
    }
}

void AnimatorEditorWidget::onPreviewToggle()
{
    if (!m_comp || m_selectedClip < 0) return;
    if (m_previewBtn->isChecked()) {
        const Shit::AnimationClip *c = m_comp->clipAt(m_selectedClip);
        if (c) m_comp->play(c->name);
        m_previewBtn->setText(tr("■ 停止"));
    } else {
        m_comp->stop();
        m_previewBtn->setText(tr("▶ 播放"));
    }
}

// ═══════════════════════════════════════════════════════════

void AnimatorEditorWidget::writeWidgetsToClip()
{
    if (!m_comp || m_selectedClip < 0) return;
    const Shit::AnimationClip *cur = m_comp->clipAt(m_selectedClip);
    if (!cur) return;
    Shit::AnimationClip clip = *cur;
    clip.texturePath = m_texEdit->text().trimmed().toStdString();
    clip.rows = m_rowsSpin->value();
    clip.cols = m_colsSpin->value();
    clip.frameWidth = static_cast<float>(m_fwSpin->value());
    clip.frameHeight = static_cast<float>(m_fhSpin->value());
    clip.duration = static_cast<float>(m_durSpin->value());
    clip.loop = m_loopCheck->isChecked();
    if (m_comp->setClip(m_selectedClip, clip)) {
        emit changed();
    }
}
