#include "pathfieldwidget.h"

#include "assetpaths.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMimeData>
#include <QToolButton>
#include <QUrl>

namespace {

/// 字段名（小写、去 m_ 前缀）是否含任一关键字
bool nameContainsAny(QString name, const char* const* keywords, int count)
{
    if (name.startsWith("m_")) name = name.mid(2);
    for (int i = 0; i < count; ++i)
        if (name.contains(QLatin1String(keywords[i]))) return true;
    return false;
}

} // namespace

PathFieldSpec pathSpecForFieldName(const std::string& fieldName)
{
    QString name = QString::fromStdString(fieldName).toLower();
    if (name.startsWith("m_")) name = name.mid(2);

    PathFieldSpec spec;
    static const char* kImages[] = { "texture", "sprite", "sheet", "tileset", "icon" };
    static const char* kAudio[]  = { "audio", "sound", "music" };
    static const char* kFont[]   = { "font" };
    static const char* kAnim[]   = { "anim", "clip", "state" };
    static const char* kScene[]  = { "scene" };
    static const char* kPrefab[] = { "prefab" };

    if (nameContainsAny(name, kImages, 5)) {
        spec.isPath = true;
        spec.dialogTitle = QObject::tr("选择图片");
        spec.fileFilter = QObject::tr("图片 (*.png *.jpg *.jpeg *.bmp *.webp *.gif)");
        spec.suffixes = { "png", "jpg", "jpeg", "bmp", "webp", "gif" };
    } else if (nameContainsAny(name, kAudio, 3)) {
        spec.isPath = true;
        spec.dialogTitle = QObject::tr("选择音频");
        spec.fileFilter = QObject::tr("音频 (*.wav *.ogg *.mp3 *.flac)");
        spec.suffixes = { "wav", "ogg", "mp3", "flac" };
    } else if (nameContainsAny(name, kFont, 1)) {
        spec.isPath = true;
        spec.dialogTitle = QObject::tr("选择字体");
        spec.fileFilter = QObject::tr("字体 (*.ttf *.otf *.ttc)");
        spec.suffixes = { "ttf", "otf", "ttc" };
    } else if (nameContainsAny(name, kAnim, 3)) {
        spec.isPath = true;
        spec.dialogTitle = QObject::tr("选择动画剪辑");
        spec.fileFilter = QObject::tr("动画剪辑 (*.anim)");
        spec.suffixes = { "anim" };
    } else if (nameContainsAny(name, kScene, 1)) {
        spec.isPath = true;
        spec.dialogTitle = QObject::tr("选择场景");
        spec.fileFilter = QObject::tr("场景 (*.scene)");
        spec.suffixes = { "scene" };
    } else if (nameContainsAny(name, kPrefab, 1)) {
        spec.isPath = true;
        spec.dialogTitle = QObject::tr("选择预置体");
        spec.fileFilter = QObject::tr("预置体 (*.prefab)");
        spec.suffixes = { "prefab" };
    }
    return spec;
}

PathFieldWidget::PathFieldWidget(const PathFieldSpec& spec, QWidget* parent)
    : QWidget(parent)
    , m_spec(spec)
{
    setAcceptDrops(true);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // QLineEdit 自身会吞拖放（往文本框插路径），关掉让本控件统一处理
    m_edit = new QLineEdit(this);
    m_edit->setAcceptDrops(false);
    m_edit->setPlaceholderText(tr("拖入文件 / 浏览 / 手输路径"));
    layout->addWidget(m_edit, 1);

    m_clearBtn = new QToolButton(this);
    m_clearBtn->setText(tr("✕"));
    m_clearBtn->setToolTip(tr("清空路径"));
    m_clearBtn->setCursor(Qt::PointingHandCursor);
    m_clearBtn->setStyleSheet(
        "QToolButton { border: none; color: #7a8a9a; padding: 2px 4px; }"
        "QToolButton:hover { color: #ff6b6b; }");
    layout->addWidget(m_clearBtn);

    m_browseBtn = new QToolButton(this);
    m_browseBtn->setText(tr("…"));
    m_browseBtn->setToolTip(tr("浏览选择文件"));
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_browseBtn);

    connect(m_browseBtn, &QToolButton::clicked, this, &PathFieldWidget::browse);
    connect(m_clearBtn, &QToolButton::clicked, this, [this] { commitAbsolute(QString()); });
    // 手输：editingFinished（回车/失焦）才提交，避免逐字符写引擎字段
    connect(m_edit, &QLineEdit::editingFinished, this, [this] {
        const QString text = m_edit->text().trimmed();
        if (text.isEmpty()) {
            if (!m_stored.isEmpty()) commitAbsolute(QString());
            return;
        }
        const QString abs = AssetPaths::toAbsolute(text);
        if (AssetPaths::toRelative(abs) == m_stored) {
            setPath(m_stored);   // 等值换写法（如手输了绝对路径）只回显规范化
            return;
        }
        commitAbsolute(abs);
    });
}

void PathFieldWidget::setPath(const QString& storedPath)
{
    m_stored = storedPath;
    m_edit->setText(AssetPaths::toRelative(storedPath));
}

void PathFieldWidget::commitAbsolute(const QString& absolutePath)
{
    const QString stored = absolutePath.isEmpty() ? QString() : AssetPaths::toRelative(absolutePath);
    if (stored == m_stored && !absolutePath.isEmpty()) {
        setPath(m_stored);
        return;
    }
    m_stored = stored;
    m_edit->setText(stored);
    emit pathCommitted(stored);
}

void PathFieldWidget::browse()
{
    const QString dir = AssetPaths::projectRoot().isEmpty()
        ? QString() : AssetPaths::projectRoot();
    const QString selected = QFileDialog::getOpenFileName(
        this, m_spec.dialogTitle, dir, m_spec.fileFilter);
    if (selected.isEmpty()) return;
    commitAbsolute(selected);
}

bool PathFieldWidget::suffixAccepted(const QString& suffix) const
{
    // 未指定后缀集 = 不限（通用路径字段）
    return m_spec.suffixes.isEmpty() || m_spec.suffixes.contains(suffix.toLower());
}

void PathFieldWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->hasUrls()) return;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (!url.isLocalFile()) continue;
        const QString suffix = QFileInfo(url.toLocalFile()).suffix();
        if (suffixAccepted(suffix)) {
            event->acceptProposedAction();
            return;
        }
    }
}

void PathFieldWidget::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasUrls()) return;
    for (const QUrl& url : event->mimeData()->urls()) {
        if (!url.isLocalFile()) continue;
        const QString suffix = QFileInfo(url.toLocalFile()).suffix();
        if (!suffixAccepted(suffix)) continue;
        commitAbsolute(QFileInfo(url.toLocalFile()).absoluteFilePath());
        event->acceptProposedAction();
        return;   // 单字段单文件：首个匹配即填
    }
}
