#include "projectwizard.h"

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QToolButton>
#include <QVBoxLayout>

namespace {
constexpr int kMaxRecentProjects = 5;

/// 全局编辑器设置（与 mainwindow 的组织/应用名一致）
QSettings globalSettings()
{
    return QSettings(QStringLiteral("ShitTeam"), QStringLiteral("ShitEngineEditor"));
}
} // namespace

ProjectWizard::ProjectWizard(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("新建项目"));
    setMinimumWidth(480);

    auto *layout = new QVBoxLayout(this);

    auto *form = new QFormLayout;
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("如 MyGame"));
    m_rootEdit = new QLineEdit(this);
    m_rootEdit->setPlaceholderText(tr("项目将创建在 <位置>/<项目名>/"));
    auto *rootBtn = new QToolButton(this);
    rootBtn->setText(tr("…"));
    auto *rootRow = new QHBoxLayout;
    rootRow->addWidget(m_rootEdit, 1);
    rootRow->addWidget(rootBtn);

    m_sdkEdit = new QLineEdit(this);
    m_sdkEdit->setPlaceholderText(tr("留空表示稍后在项目设置中配置（构建脚本前必须设置）"));
    auto *sdkBtn = new QToolButton(this);
    sdkBtn->setText(tr("…"));
    auto *sdkRow = new QHBoxLayout;
    sdkRow->addWidget(m_sdkEdit, 1);
    sdkRow->addWidget(sdkBtn);

    m_scriptsCheck = new QCheckBox(tr("生成 C++ 脚本工程（插件 DLL，含反射扫描）"), this);
    m_scriptsCheck->setChecked(true);

    form->addRow(tr("项目名"), m_nameEdit);
    form->addRow(tr("位置"), rootRow);
    form->addRow(tr("引擎 SDK"), sdkRow);
    layout->addLayout(form);
    layout->addWidget(m_scriptsCheck);

    auto *hint = new QLabel(tr("初始化场景由编辑器生成（含 game_camera）。脚本工程编译产物输出到 bin/，随后即可在场景中挂载自定义行为。"), this);
    hint->setWordWrap(true);
    hint->setStyleSheet(QStringLiteral("color: gray;"));
    layout->addWidget(hint);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    m_okButton = new QPushButton(tr("创建"), this);
    m_okButton->setDefault(true);
    auto *cancel = new QPushButton(tr("取消"), this);
    buttons->addWidget(m_okButton);
    buttons->addWidget(cancel);
    layout->addLayout(buttons);

    connect(rootBtn, &QToolButton::clicked, this, &ProjectWizard::browseRoot);
    connect(sdkBtn, &QToolButton::clicked, this, &ProjectWizard::browseSdk);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &ProjectWizard::onTextChanged);
    connect(m_rootEdit, &QLineEdit::textChanged, this, &ProjectWizard::onTextChanged);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

    // 预填上次使用的 SDK 路径与最近位置（全局记忆）
    const QSettings s = globalSettings();
    m_sdkEdit->setText(s.value("lastSdkDir").toString());
    m_rootEdit->setText(s.value("lastProjectParentDir").toString());
    m_nameEdit->setText(tr("MyGame"));

    validate();
}

QString ProjectWizard::projectName() const
{
    return m_nameEdit->text().trimmed();
}

QString ProjectWizard::rootDir() const
{
    const QString name = projectName();
    if (name.isEmpty()) return QString();
    return QDir::cleanPath(QDir(m_rootEdit->text().trimmed()).absoluteFilePath(name));
}

QString ProjectWizard::sdkDir() const
{
    return m_sdkEdit->text().trimmed();
}

bool ProjectWizard::withScripts() const
{
    return m_scriptsCheck->isChecked();
}

QString ProjectWizard::pickProjectRoot(QWidget *parent)
{
    const QSettings s = globalSettings();
    const QString initial = s.value("lastProjectDir").toString();
    const QString dir = QFileDialog::getExistingDirectory(
        parent, QObject::tr("打开项目目录"), initial);
    if (dir.isEmpty()) return QString();

    if (!QFileInfo::exists(dir + "/config.json")) {
        QMessageBox::warning(parent, QObject::tr("打开项目"),
            QObject::tr("所选目录不是 ShitEngine 项目（缺少 config.json）：\n%1").arg(dir));
        return QString();
    }
    return dir;
}

void ProjectWizard::browseRoot()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("选择项目位置"),
        m_rootEdit->text().isEmpty() ? QDir::homePath() : m_rootEdit->text());
    if (!dir.isEmpty())
        m_rootEdit->setText(dir);
}

void ProjectWizard::browseSdk()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("选择引擎 SDK 目录"),
        m_sdkEdit->text().isEmpty() ? QDir::homePath() : m_sdkEdit->text());
    if (!dir.isEmpty())
        m_sdkEdit->setText(dir);
}

void ProjectWizard::onTextChanged()
{
    validate();
}

void ProjectWizard::validate()
{
    const bool okName = !m_nameEdit->text().trimmed().isEmpty();
    const bool okRoot = !m_rootEdit->text().trimmed().isEmpty();
    if (m_okButton) m_okButton->setEnabled(okName && okRoot);
}