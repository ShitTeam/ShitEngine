#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>

class QPlainTextEdit;

/// 底部日志：编辑器与引擎日志输出。
/// P2 起可对接引擎 spdlog 的回调，将 ST_* 日志实时转到这里。
class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = nullptr);

    /// 追加一行日志（可带颜色）
    void appendMessage(const QString &text, const QColor &color = QColor());

    /// 清空日志
    void clear();

private:
    QPlainTextEdit *m_text;
};

#endif // LOGWIDGET_H
