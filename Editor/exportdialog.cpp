#include "exportdialog.h"

#include "gameexporter.h"
#include "project.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

ExportDialog::ExportDialog(Project &project, QWidget *parent)
    : QDialog(parent)
    , m_project(project)
{
    setWindowTitle(tr("导出游戏 - %1").arg(project.name()));
    resize(560, 420);

    auto *form = new QFormLayout;

    // 游戏名（默认 = 项目名；输出 <名字>.exe）
    m_nameEdit = new QLineEdit(project.name(), this);
    m_nameEdit->setPlaceholderText(tr("如 MyGame"));
    form->addRow(tr("游戏名称"), m_nameEdit);

    // 输出目录
    auto *dirRow = new QHBoxLayout;
    dirRow->setContentsMargins(0, 0, 0, 0);
    m_dirEdit = new QLineEdit(this);
    m_dirEdit->setPlaceholderText(tr("输出的游戏文件夹（绿色免安装）"));
    dirRow->addWidget(m_dirEdit, 1);
    auto *browseBtn = new QPushButton(tr("浏览…"), this);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("选择导出目录"), QFileInfo(m_dirEdit->text()).absolutePath());
        if (!dir.isEmpty()) m_dirEdit->setText(dir);
    });
    dirRow->addWidget(browseBtn);
    form->addRow(tr("输出目录"), dirRow);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);

    // 日志区
    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(2000);
    layout->addWidget(m_logView, 1);

    // 按钮
    auto *buttons = new QHBoxLayout;
    buttons->addStretch(1);
    auto *exportBtn = new QPushButton(tr("导出"), this);
    exportBtn->setDefault(true);
    auto *closeBtn = new QPushButton(tr("关闭"), this);
    buttons->addWidget(exportBtn);
    buttons->addWidget(closeBtn);
    layout->addLayout(buttons);

connect(exportBtn, &QPushButton::clicked, this, &ExportDialog::runExport);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ExportDialog::runExport()
{
    if (m_running) return;
    m_running = true;

    const QString name = m_nameEdit->text().trimmed();
    const QString dir = m_dirEdit->text().trimmed();
    m_logView->clear();

    if (m_project.scenePath().isEmpty()) {
        m_logView->appendPlainText(tr("✗ 项目未设置启动场景——请先在「文件 → 项目设置… → 通用 → 启动场景」选择导出的场景。"));
        m_running = false;
        return;
    }

    GameExportOptions options;
    options.gameName = name;
    options.outDir = dir;
    options.scenePath = m_project.scenePath();

    QString err;
    const bool ok = exportGame(m_project, options,
        [this](const QString &msg) { m_logView->appendPlainText(msg); },
        &err);
    if (!ok)
        m_logView->appendPlainText(tr("✗ 导出失败：%1").arg(err));
    m_running = false;
}