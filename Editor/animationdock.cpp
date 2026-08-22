#include "animationdock.h"
#include "assetpaths.h"
#include "dopesheetwidget.h"
#include "spritesheetdock.h"   // kSpriteFrameMime

#include <QCheckBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>

namespace {

/// 解析资产路径（委托统一路径服务：项目根 → exe 目录 → 常见资源根；全局项目根
/// 由 MainWindow 注入。参数 projectRoot 仅为兼容旧调用签名保留，不再参与解析）
QString resolveAssetPath(const QString &path, const QString &projectRoot)
{
    Q_UNUSED(projectRoot);
    return AssetPaths::toAbsolute(path);
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

    // ── 时间轴 + 剪辑参数 ──
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

    // ── 主布局 ──
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addLayout(toolbar);
    outer->addWidget(m_timeline, 1);
    outer->addLayout(paramRow);
    outer->addWidget(new QLabel(tr("拖块排序 · 拉右缘调时长 · 双击移除 · 底部为统一每帧时长（拉伸后逐帧生效）"), this));

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

    connect(m_timeline, &DopeSheetWidget::clipChanged, this, &AnimationDock::onTimelineChanged);
    connect(m_timeline, &DopeSheetWidget::frameRemoved, this, &AnimationDock::onFrameRemoved);

    // 预览播放定时器
    auto *timer = new QTimer(this);
    timer->setInterval(16);
    connect(timer, &QTimer::timeout, this, &AnimationDock::advancePlayback);
    timer->start();

    updateTitle();

    // 接受精灵表 Dock 的帧拖入
    setAcceptDrops(true);
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
    m_clip.duration = static_cast<float>(v);
    notifyClipChanged();
}

void AnimationDock::onTimelineChanged()
{
    notifyClipChanged();
}

void AnimationDock::onFrameRemoved(int blockIndex)
{
    // 双击时间轴块 → 移除该位置的帧（blockIndex = 时间轴块序号，与 frames 一一对应）
    if (!m_clipValid) return;
    if (blockIndex < 0 || blockIndex >= static_cast<int>(m_clip.frames.size())) return;
    m_clip.frames.erase(m_clip.frames.begin() + blockIndex);
    // 逐帧时长数组与帧序列同步删除（长度一致时逐帧生效）
    if (m_clip.frameDurations.size() > static_cast<size_t>(blockIndex) &&
        m_clip.frameDurations.size() == m_clip.frames.size() + 1) {
        m_clip.frameDurations.erase(m_clip.frameDurations.begin() + blockIndex);
    }
    notifyClipChanged();
}

// ═══════════════════════════════════════════════════════════
// 精灵帧拖入（来自精灵表 Dock）
// ═══════════════════════════════════════════════════════════

void AnimationDock::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(kSpriteFrameMime))
        event->acceptProposedAction();
}

void AnimationDock::dropEvent(QDropEvent *event)
{
    const QByteArray data = event->mimeData()->data(kSpriteFrameMime);
    if (data.isEmpty()) return;

    nlohmann::json payload;
    try { payload = nlohmann::json::parse(data.constData()); } catch (...) { return; }

    const QString texturePath = QString::fromStdString(payload.value("texture", ""));
    const int rows = payload.value("rows", 1);
    const int cols = payload.value("cols", 1);
    const float fw = payload.value("frameWidth", 0.0f);
    const float fh = payload.value("frameHeight", 0.0f);
    const float margin = payload.value("margin", 0.0f);
    const float spacing = payload.value("spacing", 0.0f);
    const int frameIndex = payload.value("frameIndex", 0);

    addSpriteFrames(texturePath, rows, cols, fw, fh, margin, spacing, { frameIndex });
    event->acceptProposedAction();
}

void AnimationDock::addSpriteFrames(const QString &texturePath, int rows, int cols,
                                    float frameWidth, float frameHeight,
                                    float margin, float spacing,
                                    const std::vector<int> &frameIds)
{
    if (frameIds.empty()) return;

    // 无剪辑打开时自动新建（网格参数取自拖入载荷）
    if (!m_clipValid) {
        m_clip = Shit::AnimationClip();
        m_clip.texturePath = texturePath.toStdString();
        m_clip.rows = rows;
        m_clip.cols = cols;
        m_clip.frameWidth = frameWidth;
        m_clip.frameHeight = frameHeight;
        m_clip.margin = margin;
        m_clip.spacing = spacing;
        m_clip.duration = 0.1f;
        m_clip.loop = true;
        m_clipValid = true;
        m_filePath.clear();
        loadClip(m_clip);
        updateTitle();
    } else if (m_clip.texturePath != texturePath.toStdString()) {
        // 换纹理：以新载荷的网格参数重置（同一剪辑只对应一张精灵表）
        m_clip.texturePath = texturePath.toStdString();
        m_clip.rows = rows;
        m_clip.cols = cols;
        m_clip.frameWidth = frameWidth;
        m_clip.frameHeight = frameHeight;
        m_clip.margin = margin;
        m_clip.spacing = spacing;
    }

    // 追加帧到序列
    for (int fid : frameIds)
        m_clip.frames.push_back(fid);

    reloadSheetImage();
    notifyClipChanged();
}

// ═══════════════════════════════════════════════════════════
// 内部
// ═══════════════════════════════════════════════════════════

void AnimationDock::loadClip(const Shit::AnimationClip &clip)
{
    m_updating = true;
    m_nameEdit->setText(QString::fromStdString(clip.name));
    m_uniformDurSpin->setValue(clip.duration);
    m_loopCheck->setChecked(clip.loop);
    m_updating = false;
    m_timeline->setClip(&m_clip);
    reloadSheetImage();
    syncTimeline();
}

void AnimationDock::reloadSheetImage()
{
    m_sheetImage = QImage();
    if (!m_clipValid) return;
    const QString texPath = resolveAssetPath(QString::fromStdString(m_clip.texturePath), m_projectRoot);
    if (texPath.isEmpty()) return;
    m_sheetImage = QImage(texPath);
    refreshTimelinePixmaps();
}

void AnimationDock::refreshTimelinePixmaps()
{
    // 从精灵表切出当前序列各帧的缩略图灌给时间轴
    if (m_sheetImage.isNull()) { m_timeline->clearPixmaps(); return; }
    const int tileW = static_cast<int>(m_clip.frameWidth);
    const int tileH = static_cast<int>(m_clip.frameHeight);
    const int margin = static_cast<int>(m_clip.margin);
    const int spacing = static_cast<int>(m_clip.spacing);
    if (tileW <= 0 || tileH <= 0) return;
    // 列数以元数据为准（与精灵表缩略图、引擎 SpriteSheet::getFrameRect 同源），
    // 缺失时按纹理宽度反推兜底
    const int tilesPerRow = (m_clip.cols > 0) ? m_clip.cols
                            : qMax(1, (m_sheetImage.width() - margin + spacing) / (tileW + spacing));
    if (tilesPerRow <= 0) return;
    for (int frameId : m_clip.frames) {
        if (frameId < 0) continue;
        const int col = frameId % tilesPerRow;
        const int row = frameId / tilesPerRow;
        const int sx = margin + col * (tileW + spacing);
        const int sy = margin + row * (tileH + spacing);
        if (sx + tileW > m_sheetImage.width() || sy + tileH > m_sheetImage.height()) continue;
        QImage img = m_sheetImage.copy(sx, sy, tileW, tileH)
                         .scaled(56, 56, Qt::KeepAspectRatio, Qt::FastTransformation);
        m_timeline->setFramePixmap(frameId, QPixmap::fromImage(img));
    }
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