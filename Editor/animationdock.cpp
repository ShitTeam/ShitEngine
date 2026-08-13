#include "animationdock.h"
#include "dopesheetwidget.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
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

} // namespace

AnimationDock::AnimationDock(QWidget *parent)
    : QWidget(parent)
{
    // ── 顶部工具栏 ──
    auto *newBtn = new QToolButton(this); newBtn->setText(tr("新建"));
    auto *openBtn = new QToolButton(this); openBtn->setText(tr("打开…"));
    auto *saveBtn = new QToolButton(this); saveBtn->setText(tr("保存"));
    auto *saveAsBtn = new QToolButton(this); saveAsBtn->setText(tr("另存为…"));
    m_playBtn = new QToolButton(this); m_playBtn->setText(tr("▶ 播放"));
    m_playBtn->setCheckable(true);
    m_loopCheck = new QCheckBox(tr("循环"), this);
    m_loopCheck->setChecked(true);
    m_fileLabel = new QLabel(tr("（未打开）"), this);
    m_fileLabel->setStyleSheet("color:#9aa7b4;");
    m_fileLabel->setMinimumWidth(120);

    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(4, 2, 4, 2);
    toolbar->addWidget(newBtn);
    toolbar->addWidget(openBtn);
    toolbar->addWidget(saveBtn);
    toolbar->addWidget(saveAsBtn);
    toolbar->addSpacing(8);
    toolbar->addWidget(m_playBtn);
    toolbar->addWidget(m_loopCheck);
    toolbar->addStretch();
    toolbar->addWidget(m_fileLabel, 1);

    // ── 左侧精灵表面板 ──
    m_texEdit = new QLineEdit(this);
    m_texEdit->setPlaceholderText(tr("纹理路径"));
    m_rowsSpin = new QSpinBox(this); m_rowsSpin->setRange(1, 256);
    m_colsSpin = new QSpinBox(this); m_colsSpin->setRange(1, 256);
    m_fwSpin = new QDoubleSpinBox(this); m_fwSpin->setRange(1, 4096); m_fwSpin->setDecimals(1);
    m_fhSpin = new QDoubleSpinBox(this); m_fhSpin->setRange(1, 4096); m_fhSpin->setDecimals(1);
    m_gridHost = new QWidget(this);

    auto *sheetPanel = new QWidget(this);
    sheetPanel->setFixedWidth(240);
    auto *sheetLayout = new QVBoxLayout(sheetPanel);
    sheetLayout->setContentsMargins(0, 0, 0, 0);
    sheetLayout->addWidget(new QLabel(tr("精灵表"), this));
    sheetLayout->addWidget(m_texEdit);
    auto *gridParamRow = new QHBoxLayout;
    gridParamRow->addWidget(m_rowsSpin); gridParamRow->addWidget(m_colsSpin);
    sheetLayout->addLayout(gridParamRow);
    auto *gridParamRow2 = new QHBoxLayout;
    gridParamRow2->addWidget(m_fwSpin); gridParamRow2->addWidget(m_fhSpin);
    sheetLayout->addLayout(gridParamRow2);
    auto *frameScroll = new QScrollArea(this);
    frameScroll->setWidgetResizable(true);
    frameScroll->setWidget(m_gridHost);
    sheetLayout->addWidget(frameScroll, 1);
    sheetLayout->addWidget(new QLabel(tr("点选帧加入时间轴"), this));

    // ── 右侧：时间轴 + 剪辑参数 ──
    m_timeline = new DopeSheetWidget(this);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("剪辑名"));
    m_uniformDurSpin = new QDoubleSpinBox(this);
    m_uniformDurSpin->setRange(0.001, 60.0);
    m_uniformDurSpin->setDecimals(3);
    m_uniformDurSpin->setSingleStep(0.01);
    m_uniformDurSpin->setValue(0.1);

    auto *paramRow = new QHBoxLayout;
    paramRow->addWidget(new QLabel(tr("名称"), this));
    paramRow->addWidget(m_nameEdit, 1);
    paramRow->addWidget(new QLabel(tr("每帧秒"), this));
    paramRow->addWidget(m_uniformDurSpin);

    auto *rightPanel = new QWidget(this);
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->addWidget(m_timeline, 1);
    rightLayout->addLayout(paramRow);
    rightLayout->addWidget(new QLabel(tr("拖块排序 · 拉右缘调时长 · 双击移除 · 底部为统一每帧时长（拉伸后逐帧生效）"), this));

    // ── 主布局：左精灵表 + 右时间轴 ──
    auto *main = new QHBoxLayout;
    main->setContentsMargins(0, 0, 0, 0);
    main->addWidget(sheetPanel);
    main->addWidget(rightPanel, 1);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addLayout(toolbar);
    outer->addLayout(main, 1);

    // ── 连接 ──
    connect(newBtn, &QToolButton::clicked, this, &AnimationDock::onNew);
    connect(openBtn, &QToolButton::clicked, this, &AnimationDock::onOpen);
    connect(saveBtn, &QToolButton::clicked, this, &AnimationDock::onSave);
    connect(saveAsBtn, &QToolButton::clicked, this, &AnimationDock::onSaveAs);
    connect(m_playBtn, &QToolButton::clicked, this, &AnimationDock::onPlayToggle);
    connect(m_loopCheck, &QCheckBox::toggled, this, &AnimationDock::onLoopToggled);
    connect(m_nameEdit, &QLineEdit::editingFinished, this, &AnimationDock::onNameEdited);
    connect(m_uniformDurSpin, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &AnimationDock::onUniformDurationChanged);
    connect(m_texEdit, &QLineEdit::editingFinished, this, &AnimationDock::onTextureEdited);
    connect(m_rowsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &AnimationDock::onGridParamChanged);
    connect(m_colsSpin, qOverload<int>(&QSpinBox::valueChanged), this, &AnimationDock::onGridParamChanged);
    connect(m_fwSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimationDock::onGridParamChanged);
    connect(m_fhSpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnimationDock::onGridParamChanged);

    connect(m_timeline, &DopeSheetWidget::clipChanged, this, &AnimationDock::onTimelineChanged);
    connect(m_timeline, &DopeSheetWidget::selectedFrameChanged, this,
            [this](int index) { if (index >= 0) refreshFrameHighlights(); });
    connect(m_timeline, &DopeSheetWidget::frameActivated, this, &AnimationDock::onFrameActivated);

    // 预览播放定时器
    auto *timer = new QTimer(this);
    timer->setInterval(16);
    connect(timer, &QTimer::timeout, this, &AnimationDock::advancePlayback);
    timer->start();

    updateTitle();
}

// ═══════════════════════════════════════════════════════════
// 文件操作
// ═══════════════════════════════════════════════════════════

void AnimationDock::onNew()
{
    if (m_dirty) {
        const auto r = QMessageBox::question(this, tr("未保存"),
            tr("当前剪辑有未保存的改动，丢弃并新建？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (r != QMessageBox::Yes) return;
    }
    m_clip = Shit::AnimationClip();
    m_clip.name = "Clip";
    m_clip.rows = 1; m_clip.cols = 1;
    m_clip.frameWidth = 32; m_clip.frameHeight = 32;
    m_clip.duration = 0.1f;
    m_clip.loop = true;
    m_clipValid = true;
    m_dirty = false;
    m_filePath.clear();
    m_playTime = 0.0f;
    m_playing = false;
    m_playBtn->setChecked(false);
    m_playBtn->setText(tr("▶ 播放"));
    loadClip(m_clip);
    updateTitle();
    rebuildFrameGrid();
}

bool AnimationDock::openFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray data = f.readAll();
    f.close();
    Shit::AnimationClip clip;
    try {
        nlohmann::json j = nlohmann::json::parse(data.constData());
        if (!clip.fromJson(j)) return false;
    } catch (const std::exception &) {
        return false;
    }
    if (m_dirty) {
        const auto r = QMessageBox::question(this, tr("未保存"),
            tr("当前剪辑有未保存的改动，丢弃并打开？"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (r != QMessageBox::Yes) return false;
    }
    m_clip = clip;
    m_clipValid = true;
    m_dirty = false;
    m_filePath = path;
    m_playTime = 0.0f;
    m_playing = false;
    m_playBtn->setChecked(false);
    m_playBtn->setText(tr("▶ 播放"));
    loadClip(m_clip);
    updateTitle();
    rebuildFrameGrid();
    return true;
}

void AnimationDock::onOpen()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("打开动画剪辑"), QString(),
                                                      tr("ShitEngine 动画 (*.anim)"));
    if (path.isEmpty()) return;
    if (!openFile(path)) {
        QMessageBox::warning(this, tr("打开失败"), tr("无法读取该 .anim 文件"));
    }
}

bool AnimationDock::save()
{
    if (m_filePath.isEmpty()) return saveAs();
    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("保存失败"), tr("无法写入文件：%1").arg(m_filePath));
        return false;
    }
    f.write(QByteArray::fromStdString(m_clip.toJson().dump(4)));
    f.close();
    m_dirty = false;
    updateTitle();
    emit saved(m_filePath);   // 方案 A：通知依赖该资产的 Animator 状态同步
    return true;
}

bool AnimationDock::saveAs()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("另存为动画剪辑"), m_filePath,
                                                      tr("ShitEngine 动画 (*.anim)"));
    if (path.isEmpty()) return false;
    m_filePath = path;
    return save();
}

void AnimationDock::onSave() { save(); }
void AnimationDock::onSaveAs() { saveAs(); }

// ═══════════════════════════════════════════════════════════
// 预览播放
// ═══════════════════════════════════════════════════════════

void AnimationDock::onPlayToggle()
{
    if (!m_clipValid) { m_playBtn->setChecked(false); return; }
    if (m_clip.frames.empty()) { m_playBtn->setChecked(false); return; }
    m_playing = m_playBtn->isChecked();
    if (m_playing) {
        if (m_playTime <= 0.0f || m_playTime >= totalDuration()) m_playTime = 0.0f;
        m_playBtn->setText(tr("■ 停止"));
    } else {
        m_playBtn->setText(tr("▶ 播放"));
    }
    m_timeline->setPlayTime(m_playing ? m_playTime : -1.0f);
}

void AnimationDock::onLoopToggled(bool on)
{
    if (!m_clipValid || m_updating) return;
    m_clip.loop = on;
    notifyClipChanged();
}

void AnimationDock::advancePlayback()
{
    if (!m_playing || !m_clipValid || m_clip.frames.empty()) {
        m_timeline->setPlayTime(-1.0f);
        return;
    }
    const float total = totalDuration();
    m_playTime += 0.016f;
    if (m_playTime >= total) {
        if (m_clip.loop) m_playTime = std::fmod(m_playTime, total);
        else { m_playTime = total; m_playing = false; m_playBtn->setChecked(false); m_playBtn->setText(tr("▶ 播放")); }
    }
    m_timeline->setPlayTime(m_playTime);
}

float AnimationDock::totalDuration() const
{
    if (m_clip.frames.empty()) return 0.0f;
    if (m_clip.frameDurations.size() == m_clip.frames.size()) {
        float t = 0.0f;
        for (float d : m_clip.frameDurations) t += d;
        return t;
    }
    return static_cast<float>(m_clip.frames.size()) * m_clip.duration;
}

// ═══════════════════════════════════════════════════════════
// 剪辑参数
// ═══════════════════════════════════════════════════════════

void AnimationDock::onNameEdited()
{
    if (!m_clipValid) return;
    const std::string name = m_nameEdit->text().trimmed().toStdString();
    if (name.empty()) { refreshClipParams(); return; }
    m_clip.name = name;
    notifyClipChanged();
}

void AnimationDock::onUniformDurationChanged(double v)
{
    if (!m_clipValid || m_updating) return;
    // 统一时长：若当前无逐帧时长，直接改 duration；有则保留逐帧（仅更新基础值）
    m_clip.duration = static_cast<float>(v);
    notifyClipChanged();
    refreshTimelinePixmaps();
}

// ═══════════════════════════════════════════════════════════
// 精灵表面板
// ═══════════════════════════════════════════════════════════

void AnimationDock::onTextureEdited()
{
    if (!m_clipValid) return;
    m_clip.texturePath = m_texEdit->text().trimmed().toStdString();
    notifyClipChanged();
    rebuildFrameGrid();
}

void AnimationDock::onGridParamChanged()
{
    if (!m_clipValid || m_updating) return;
    m_clip.rows = m_rowsSpin->value();
    m_clip.cols = m_colsSpin->value();
    m_clip.frameWidth = static_cast<float>(m_fwSpin->value());
    m_clip.frameHeight = static_cast<float>(m_fhSpin->value());
    notifyClipChanged();
    rebuildFrameGrid();
    refreshTimelinePixmaps();
}

void AnimationDock::onFrameGridClicked(int tileId)
{
    if (!m_clipValid) return;
    m_clip.frames.push_back(tileId);
    // 逐帧时长默认用统一值（若已开启逐帧，追加统一值）
    if (m_clip.frameDurations.size() == m_clip.frames.size() - 1)
        m_clip.frameDurations.push_back(m_clip.duration);
    else if (m_clip.frameDurations.empty())
        m_clip.frameDurations.clear();  // 保持空=统一时长
    notifyClipChanged();   // 内部已 syncTimeline + refreshTimelinePixmaps + refreshFrameHighlights
}

void AnimationDock::onTimelineChanged()
{
    notifyClipChanged();
    refreshTimelinePixmaps();
}

void AnimationDock::onFrameActivated(int frameId)
{
    // 双击时间轴某帧：可选联动（当前无额外动作，忽略）
    Q_UNUSED(frameId)
}

// ═══════════════════════════════════════════════════════════
// 内部
// ═══════════════════════════════════════════════════════════

void AnimationDock::loadClip(const Shit::AnimationClip &clip)
{
    m_updating = true;
    m_nameEdit->setText(QString::fromStdString(clip.name));
    m_uniformDurSpin->setValue(clip.duration);
    m_texEdit->setText(QString::fromStdString(clip.texturePath));
    m_rowsSpin->setValue(clip.rows);
    m_colsSpin->setValue(clip.cols);
    m_fwSpin->setValue(clip.frameWidth);
    m_fhSpin->setValue(clip.frameHeight);
    m_loopCheck->setChecked(clip.loop);
    m_updating = false;
    m_timeline->setClip(&m_clip);
    syncTimeline();
    refreshTimelinePixmaps();
}

void AnimationDock::syncTimeline()
{
    m_timeline->setClip(&m_clip);
    m_timeline->selectFrameAt(-1);
}

void AnimationDock::notifyClipChanged()
{
    m_dirty = true;
    updateTitle();
    emit changed();
    syncTimeline();
    refreshTimelinePixmaps();
    refreshFrameHighlights();
}

void AnimationDock::refreshClipParams()
{
    if (!m_clipValid) return;
    m_updating = true;
    m_nameEdit->setText(QString::fromStdString(m_clip.name));
    m_uniformDurSpin->setValue(m_clip.duration);
    m_updating = false;
}

void AnimationDock::updateTitle()
{
    if (m_filePath.isEmpty()) {
        m_fileLabel->setText(m_clipValid ? tr("未保存剪辑 %1").arg(m_dirty ? QStringLiteral("•") : QString())
                                         : tr("（未打开）"));
    } else {
        m_fileLabel->setText(QStringLiteral("%1%2")
                                 .arg(QFileInfo(m_filePath).fileName(),
                                      m_dirty ? QStringLiteral(" •") : QString()));
    }
}

void AnimationDock::rebuildFrameGrid()
{
    if (m_gridHost->layout()) {
        while (auto *item = m_gridHost->layout()->takeAt(0))
            if (QWidget *w = item->widget()) delete w;
        delete m_gridHost->layout();
    }
    m_gridButtons.clear();

    if (!m_clipValid) return;
    const int rows = m_clip.rows, cols = m_clip.cols;
    if (rows <= 0 || cols <= 0 || m_clip.frameWidth <= 0 || m_clip.frameHeight <= 0) return;

    const QString texPath = resolveAssetPath(QString::fromStdString(m_clip.texturePath));
    m_sheetImage = QImage(texPath);
    m_sheetValid = !m_sheetImage.isNull();
    if (!m_sheetValid) return;
    const QImage &sheet = m_sheetImage;

    const int tileW = static_cast<int>(m_clip.frameWidth);
    const int tileH = static_cast<int>(m_clip.frameHeight);
    const int tilesPerRow = sheet.width() / tileW;
    const int tileCount = (sheet.width() / tileW) * (sheet.height() / tileH);
    if (tilesPerRow <= 0 || tileCount <= 0) return;

    auto *grid = new QGridLayout(m_gridHost);
    grid->setContentsMargins(2, 2, 2, 2);
    grid->setSpacing(2);
    const int maxCols = 3;
    const int pw = 60, ph = 60;
    for (int id = 0; id < tileCount; ++id) {
        const int sx = (id % tilesPerRow) * tileW;
        const int sy = (id / tilesPerRow) * tileH;
        QImage img = sheet.copy(sx, sy, tileW, tileH)
                         .scaled(pw, ph, Qt::KeepAspectRatio, Qt::FastTransformation);
        auto *btn = new QToolButton(m_gridHost);
        btn->setIcon(QIcon(QPixmap::fromImage(img)));
        btn->setIconSize(img.size());
        btn->setToolTip(QStringLiteral("帧 %1").arg(id));
        grid->addWidget(btn, id / maxCols, id % maxCols);
        connect(btn, &QToolButton::clicked, this, [this, id] { onFrameGridClicked(id); });
        m_gridButtons.emplace_back(id, btn);
    }
    m_gridHost->adjustSize();
    refreshTimelinePixmaps();
}

void AnimationDock::refreshFrameHighlights()
{
    std::set<int> used;
    for (int f : m_clip.frames) used.insert(f);
    for (auto &[id, btn] : m_gridButtons) {
        // 高亮已加入时间轴的帧（用边框样式）
        btn->setProperty("inTimeline", used.count(id) > 0);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
    }
}

void AnimationDock::refreshTimelinePixmaps()
{
    // 从精灵表切出当前序列各帧的缩略图灌给时间轴
    if (!m_sheetValid || m_sheetImage.isNull()) { m_timeline->clearPixmaps(); return; }
    const int tileW = static_cast<int>(m_clip.frameWidth);
    const int tileH = static_cast<int>(m_clip.frameHeight);
    if (tileW <= 0 || tileH <= 0) return;
    const int tilesPerRow = m_sheetImage.width() / tileW;
    if (tilesPerRow <= 0) return;
    for (int frameId : m_clip.frames) {
        if (frameId < 0) continue;
        const int sx = (frameId % tilesPerRow) * tileW;
        const int sy = (frameId / tilesPerRow) * tileH;
        if (sx + tileW > m_sheetImage.width() || sy + tileH > m_sheetImage.height()) continue;
        QImage img = m_sheetImage.copy(sx, sy, tileW, tileH)
                         .scaled(56, 56, Qt::KeepAspectRatio, Qt::FastTransformation);
        m_timeline->setFramePixmap(frameId, QPixmap::fromImage(img));
    }
}
