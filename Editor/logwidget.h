#ifndef LOGWIDGET_H
#define LOGWIDGET_H

#include <QWidget>

class QPlainTextEdit;
class QLineEdit;
class QPushButton;

/// 底部日志：编辑器与引擎日志输出。
/// P12 起对接引擎 spdlog 的回调，将 ST_* 日志实时转到这里。
class LogWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = nullptr);

    /// 追加一行日志（可带颜色）
    void appendMessage(const QString &text, const QColor &color = QColor());

    /// 清空日志
    void clear();

    /// 设置「保存日志…」对话框的默认目录（打开项目后指向项目 .shitengine/log）
    void setDefaultDir(const QString &dir) { m_defaultDir = dir; }

private:
    /// 保存日志到文件
    void saveToFile();

    QPlainTextEdit *m_text;
    QPushButton *m_saveBtn;
    QPushButton *m_clearBtn;
    QString m_defaultDir;   ///< 保存对话框默认目录（可空）
};

#endif // LOGWIDGET_H
