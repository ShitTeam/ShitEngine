#include "logwidget.h"

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVBoxLayout>

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
    , m_text(new QPlainTextEdit(this))
    , m_saveBtn(new QPushButton(tr("保存日志…"), this))
    , m_clearBtn(new QPushButton(tr("清除"), this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    // 工具行：保存（编辑器侧日志仅存内存，崩溃即丢——保存后可查）/ 清除
    auto *toolRow = new QHBoxLayout;
    toolRow->setContentsMargins(4, 2, 4, 0);
    toolRow->addStretch(1);
    m_saveBtn->setToolTip(tr("把当前日志面板内容导出为 txt（默认存到项目 .shitengine/log/）"));
    m_clearBtn->setToolTip(tr("清空日志面板"));
    toolRow->addWidget(m_saveBtn);
    toolRow->addWidget(m_clearBtn);
    layout->addLayout(toolRow);

    layout->addWidget(m_text);

    m_text->setReadOnly(true);
    m_text->setMaximumBlockCount(5000); // 防止无限增长

    connect(m_saveBtn, &QPushButton::clicked, this, &LogWidget::saveToFile);
    connect(m_clearBtn, &QPushButton::clicked, this, &LogWidget::clear);
}

void LogWidget::appendMessage(const QString &text, const QColor &color)
{
    QTextCursor cursor = m_text->textCursor();
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat format;
    if (color.isValid())
        format.setForeground(color);   // 错误红/警告橙等区分级别
    cursor.insertText(text + "\n", format);
    m_text->ensureCursorVisible();
}

void LogWidget::clear()
{
    m_text->clear();
}

void LogWidget::saveToFile()
{
    const QString text = m_text->toPlainText();
    if (text.isEmpty()) {
        QMessageBox::information(this, tr("保存日志"), tr("日志面板为空，无需保存。"));
        return;
    }

    QString initial = m_defaultDir.isEmpty() ? QString() : m_defaultDir;
    const QString path = QFileDialog::getSaveFileName(this, tr("保存日志"),
        initial + "/log_editor_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".txt",
        tr("文本文件 (*.txt)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("保存日志"), tr("无法写入文件：\n%1").arg(path));
        return;
    }
    f.write(text.toUtf8());
    f.close();
}
