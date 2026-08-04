#include "logwidget.h"

#include <QPlainTextEdit>
#include <QTextCursor>
#include <QVBoxLayout>

LogWidget::LogWidget(QWidget *parent)
    : QWidget(parent)
    , m_text(new QPlainTextEdit(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_text);

    m_text->setReadOnly(true);
    m_text->setMaximumBlockCount(5000); // 防止无限增长
}

void LogWidget::appendMessage(const QString &text, const QColor &color)
{
    QTextCursor cursor = m_text->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text + "\n");
    if (color.isValid())
        cursor.select(QTextCursor::LineUnderCursor);
    m_text->ensureCursorVisible();
}

void LogWidget::clear()
{
    m_text->clear();
}
