#ifndef PROJECTSETTINGSDIALOG_H
#define PROJECTSETTINGSDIALOG_H

#include <QDialog>
#include <QStringList>

class Project;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QComboBox;
class QTableWidget;
class QTabWidget;

/// 键捕获小窗：按下任意键（键盘/鼠标键）时捕获；Esc 取消，Backspace/Delete 清除。
/// 捕获结果经 capturedName() 读取（SDL 键名或 "MouseButton.X"；清除返回空串）。
class KeyCaptureDialog : public QDialog
{
    Q_OBJECT
public:
    explicit KeyCaptureDialog(QWidget *parent = nullptr);
    /// 捕获到的绑定键名（"Space"/"Left Shift"/"MouseButton.Left"；清除返回空）
    QString capturedName() const { return m_captured; }

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    void acceptCapture(const QString &name);

    QString m_captured;   ///< 捕获结果（空 = 用户要求清除该绑定）
};

/// 一行多键绑定：键名按钮（点击改绑、右键菜单删除）+ 「＋」添加（键盘捕获 / 鼠标键菜单）
class BindingListWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BindingListWidget(QWidget *parent = nullptr);

    void setBindings(const QStringList &names);
    QStringList bindings() const;

private:
    void rebuild();
    void replaceBinding(int index);          ///< 改绑（Backspace 清除该条）
    void addBindingFromCapture();            ///< 「＋ → 键盘按键…」捕获后追加
    void appendBinding(const QString &name);
    void removeBinding(int index);

    QHBoxLayout *m_layout = nullptr;
    QStringList m_names;   ///< 绑定名（SDL 名 / MouseButton.X），顺序即存储顺序
};

/// 项目设置对话框：通用（名称/SDK/启动场景）+ 输入（动作/轴按键映射）。
/// 内容在 OK 时写入 m_project（config.json 的 inputMappings / engine.sdkDir / scene）。
class ProjectSettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ProjectSettingsDialog(Project &project, QWidget *parent = nullptr);

    /// 把界面内容写回 Project（调用方随后 saveConfig() + 应用映射）
    void applyToProject();

protected:
    /// OK 前做重复键校验；冲突时弹警告，用户确认后才 accept()
    void tryAccept();

private:
    void buildGeneralTab(QWidget *page);
    void buildInputTab(QWidget *page);
    void loadInputFromProject();
    void browseIde();   ///< 浏览选择 IDE 可执行文件（加入下拉并选中）

    void addActionRow(const QString &name, const QStringList &bindings);
    void addAxisRow(const QString &name, const QStringList &negative, const QStringList &positive);
    QStringList duplicateKeys() const;   ///< 全局重复绑定的键名列表（空 = 无冲突）

    Project &m_project;
    QLineEdit *m_sdkEdit = nullptr;
    QComboBox *m_sceneCombo = nullptr;
    QComboBox *m_ideCombo = nullptr;   ///< 代码编辑器（探测列表 + 浏览自定义）
    QTableWidget *m_actionTable = nullptr;
    QTableWidget *m_axisTable = nullptr;
    QString m_projectName;   ///< 仅显示
};
#endif // PROJECTSETTINGSDIALOG_H