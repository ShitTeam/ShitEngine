#pragma once

#include <QDialog>

class Project;
class QLineEdit;
class QPlainTextEdit;

/// P18 导出游戏对话框：游戏名 + 输出目录 + 导出日志区
class ExportDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExportDialog(Project &project, QWidget *parent = nullptr);

private:
    void runExport();   ///< 同步执行导出（日志逐行进 m_logView）

    Project &m_project;
    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_dirEdit = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    bool m_running = false;
};