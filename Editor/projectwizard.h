#ifndef PROJECTWIZARD_H
#define PROJECTWIZARD_H

#include <QDialog>

class QLineEdit;
class QCheckBox;

/// P14 新建项目向导：项目名 / 位置 / 引擎 SDK 目录 / 是否生成 C++ 脚本工程。
/// 打开已有项目走静态 pickProjectRoot()（目录选择 + config.json 校验）。
class ProjectWizard : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectWizard(QWidget *parent = nullptr);

    QString projectName() const;
    /// 项目根目录（父目录 + 名称拼接后的绝对路径）
    QString rootDir() const;
    QString sdkDir() const;
    bool withScripts() const;

    /// 选择项目的根目录（供打开项目用）；选中目录含 config.json 才返回，否则空。
    static QString pickProjectRoot(QWidget *parent = nullptr);

private slots:
    void browseRoot();
    void browseSdk();
    void onTextChanged();   ///< 名称/目录变化时刷新 OK 可用性

private:
    void validate();

    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_rootEdit = nullptr;
    QLineEdit *m_sdkEdit = nullptr;
    QCheckBox *m_scriptsCheck = nullptr;
    class QPushButton *m_okButton = nullptr;
};

#endif // PROJECTWIZARD_H