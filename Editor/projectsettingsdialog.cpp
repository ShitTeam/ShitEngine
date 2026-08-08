#include "projectsettingsdialog.h"

#include "idefinder.h"
#include "keys.h"
#include "project.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QVBoxLayout>

#include <nlohmann/json.hpp>

// =========================================================================
// KeyCaptureDialog：按下任意键（键盘/鼠标钮）即捕获
// =========================================================================

KeyCaptureDialog::KeyCaptureDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("绑定按键"));
    auto *label = new QLabel(tr("按下任意键或鼠标键…\n（Esc 取消 / Backspace 清除该绑定）"), this);
    label->setAlignment(Qt::AlignCenter);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(label);
    setFixedSize(320, 120);
}

void KeyCaptureDialog::acceptCapture(const QString &name)
{
    m_captured = name;
    accept();
}

void KeyCaptureDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) { reject(); return; }
    // Backspace / Delete = 清除该条绑定（区别于 Esc 取消）
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        acceptCapture(QString());
        return;
    }
    const QString name = sdlKeyNameForQtKey(event->key());
    if (!name.isEmpty()) { acceptCapture(name); return; }
    QDialog::keyPressEvent(event);
}

void KeyCaptureDialog::mousePressEvent(QMouseEvent *event)
{
    const QString name = mouseBindingName(event->button());
    if (!name.isEmpty()) { acceptCapture(name); return; }
    QDialog::mousePressEvent(event);
}

// =========================================================================
// BindingListWidget：一行多键（键名按钮 + ＋ 添加）
// =========================================================================

BindingListWidget::BindingListWidget(QWidget *parent)
    : QWidget(parent)
{
    rebuild();
}

QStringList BindingListWidget::bindings() const
{
    return m_names;
}

void BindingListWidget::setBindings(const QStringList &names)
{
    m_names = names;
    rebuild();
}

void BindingListWidget::rebuild()
{
    if (!m_layout) {
        m_layout = new QHBoxLayout(this);
        m_layout->setContentsMargins(0, 0, 0, 0);
        m_layout->setSpacing(2);
    } else {
        while (QLayoutItem *item = m_layout->takeAt(0)) {
            if (QWidget *w = item->widget()) w->deleteLater();
            delete item;
        }
    }

    // 键名 chips：点击改绑（捕获窗）、右键菜单改绑/删除
    for (int i = 0; i < m_names.size(); ++i) {
        const int index = i;
        auto *btn = new QPushButton(m_names.at(index), this);
        btn->setToolTip(tr("点击改绑；右键可删除"));
        connect(btn, &QPushButton::clicked, this, [this, index]() { replaceBinding(index); });
        connect(btn, &QWidget::customContextMenuRequested, this, [this, btn, index]() {
            QMenu menu(btn);
            connect(menu.addAction(tr("改绑…")), &QAction::triggered, this,
                    [this, index]() { replaceBinding(index); });
            connect(menu.addAction(tr("删除")), &QAction::triggered, this,
                    [this, index]() { removeBinding(index); });
            menu.exec(QCursor::pos());
        });
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        m_layout->addWidget(btn);
    }

    // 「＋」：键盘捕获 or 直接选鼠标键
    auto *addBtn = new QToolButton(this);
    addBtn->setText(QStringLiteral("＋"));
    addBtn->setToolTip(tr("添加绑定"));
    addBtn->setPopupMode(QToolButton::InstantPopup);
    auto *menu = new QMenu(addBtn);
    connect(menu->addAction(tr("按键…（按下任意键）")), &QAction::triggered, this,
            [this]() { addBindingFromCapture(); });
    menu->addSeparator();
    connect(menu->addAction(tr("鼠标左键")), &QAction::triggered, this,
            [this]() { appendBinding(QStringLiteral("MouseButton.Left")); });
    connect(menu->addAction(tr("鼠标右键")), &QAction::triggered, this,
            [this]() { appendBinding(QStringLiteral("MouseButton.Right")); });
    connect(menu->addAction(tr("鼠标中键")), &QAction::triggered, this,
            [this]() { appendBinding(QStringLiteral("MouseButton.Middle")); });
    connect(menu->addAction(tr("鼠标侧键 1")), &QAction::triggered, this,
            [this]() { appendBinding(QStringLiteral("MouseButton.XButton1")); });
    connect(menu->addAction(tr("鼠标侧键 2")), &QAction::triggered, this,
            [this]() { appendBinding(QStringLiteral("MouseButton.XButton2")); });
    addBtn->setMenu(menu);
    m_layout->addWidget(addBtn);

    m_layout->addStretch(1);
}

void BindingListWidget::replaceBinding(int index)
{
    KeyCaptureDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    const QString captured = dlg.capturedName();
    if (captured.isEmpty()) {
        removeBinding(index);   // Backspace：清除该条
        return;
    }
    if (!m_names.contains(captured)) {
        m_names[index] = captured;
        rebuild();
    }
}

void BindingListWidget::addBindingFromCapture()
{
    KeyCaptureDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    appendBinding(dlg.capturedName());
}

void BindingListWidget::appendBinding(const QString &name)
{
    if (name.isEmpty() || m_names.contains(name)) return;   // 行内去重
    m_names.append(name);
    rebuild();
}

void BindingListWidget::removeBinding(int index)
{
    if (index < 0 || index >= m_names.size()) return;
    m_names.removeAt(index);
    rebuild();
}

// =========================================================================
// ProjectSettingsDialog
// =========================================================================

ProjectSettingsDialog::ProjectSettingsDialog(Project &project, QWidget *parent)
    : QDialog(parent)
    , m_project(project)
    , m_projectName(project.name())
{
    setWindowTitle(tr("项目设置 - %1").arg(m_projectName));
    resize(760, 560);

    auto *tabs = new QTabWidget(this);
    auto *general = new QWidget(tabs);
    auto *input = new QWidget(tabs);
    buildGeneralTab(general);
    buildInputTab(input);
    tabs->addTab(general, tr("通用"));
    tabs->addTab(input, tr("输入"));
    loadInputFromProject();

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(tabs, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ProjectSettingsDialog::tryAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void ProjectSettingsDialog::buildGeneralTab(QWidget *page)
{
    auto *form = new QFormLayout(page);

    form->addRow(tr("项目名称"), new QLabel(m_projectName, page));

    // SDK 目录（行内编辑 + 浏览）
    m_sdkEdit = new QLineEdit(m_project.sdkDir(), page);
    auto *sdkRow = new QHBoxLayout;
    sdkRow->setContentsMargins(0, 0, 0, 0);
    sdkRow->addWidget(m_sdkEdit, 1);
    auto *browseBtn = new QToolButton(page);
    browseBtn->setText(tr("浏览…"));
    connect(browseBtn, &QToolButton::clicked, this, [this]() {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("选择引擎 SDK 目录"), QFileInfo(m_sdkEdit->text()).absolutePath());
        if (!dir.isEmpty()) m_sdkEdit->setText(dir);
    });
    sdkRow->addWidget(browseBtn);
    form->addRow(tr("引擎 SDK 目录"), sdkRow);

    // 启动场景：下拉扫描 项目 Scenes/ 下的 .scene
    m_sceneCombo = new QComboBox(page);
    m_sceneCombo->addItem(tr("（未设置）"), QString());
    const QString currentScene = m_project.scenePath();
    int currentIndex = 0;
    const QFileInfoList scenes =
        QDir(m_project.scenesDir()).entryInfoList({ "*.scene" }, QDir::Files, QDir::Name);
    for (const QFileInfo &info : scenes) {
        m_sceneCombo->addItem(info.fileName(), info.filePath());
        if (info.filePath() == currentScene) currentIndex = m_sceneCombo->count() - 1;
    }
    m_sceneCombo->setCurrentIndex(currentIndex);
    form->addRow(tr("启动场景"), m_sceneCombo);

    // 代码编辑器：下拉 = 本机探测到的 IDE + 浏览自定义（P16）
    m_ideCombo = new QComboBox(page);
    m_ideCombo->addItem(tr("（未配置 — 菜单「打开代码」将引导先选择）"), QString());
    const QList<IdeInfo> ides = detectInstalledIdes();
    for (const IdeInfo &ide : ides)
        m_ideCombo->addItem(ide.name, ide.executable);
    auto *ideRow = new QHBoxLayout;
    ideRow->setContentsMargins(0, 0, 0, 0);
    ideRow->addWidget(m_ideCombo, 1);
    auto *ideBrowseBtn = new QToolButton(page);
    ideBrowseBtn->setText(tr("浏览…"));
    connect(ideBrowseBtn, &QToolButton::clicked, this, &ProjectSettingsDialog::browseIde);
    ideRow->addWidget(ideBrowseBtn);
    form->addRow(tr("代码编辑器"), ideRow);

    // 选中项目已配置的 IDE（未在探测列表中也加入，保证显示可读）
    const QString ideExe = m_project.ideExePath();
    if (!ideExe.isEmpty()) {
        const int idx = m_ideCombo->findData(ideExe);
        if (idx >= 0) {
            m_ideCombo->setCurrentIndex(idx);
        } else {
            m_ideCombo->addItem(QFileInfo(ideExe).fileName(), ideExe);
            m_ideCombo->setCurrentIndex(m_ideCombo->count() - 1);
        }
    }
}

void ProjectSettingsDialog::browseIde()
{
    const QString exe = QFileDialog::getOpenFileName(
        this, tr("选择 IDE 可执行文件"),
        qEnvironmentVariable("ProgramFiles"),
        tr("可执行程序 (*.exe)"));
    if (exe.isEmpty()) return;
    const int idx = m_ideCombo->findData(exe);
    if (idx >= 0) {
        m_ideCombo->setCurrentIndex(idx);
        return;
    }
    m_ideCombo->addItem(QFileInfo(exe).fileName(), exe);
    m_ideCombo->setCurrentIndex(m_ideCombo->count() - 1);
}

void ProjectSettingsDialog::buildInputTab(QWidget *page)
{
    auto *layout = new QVBoxLayout(page);

    // ── 动作 ──
    auto *actionTitle = new QLabel(tr("动作（Input::IsActionPressed(\"…\") 等查询；一个动作可绑多个键）"),
                                   page);
    actionTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    layout->addWidget(actionTitle);

    m_actionTable = new QTableWidget(0, 2, page);
    m_actionTable->setHorizontalHeaderLabels({ tr("动作名称"), tr("绑定按键") });
    m_actionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_actionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_actionTable->verticalHeader()->setVisible(false);
    m_actionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_actionTable->setAlternatingRowColors(true);
    layout->addWidget(m_actionTable, 1);

    auto *actionButtons = new QHBoxLayout;
    actionButtons->addStretch(1);
    auto *addActionBtn = new QPushButton(tr("添加动作"), page);
    auto *removeActionBtn = new QPushButton(tr("删除动作"), page);
    actionButtons->addWidget(addActionBtn);
    actionButtons->addWidget(removeActionBtn);
    layout->addLayout(actionButtons);
    connect(addActionBtn, &QPushButton::clicked, this, [this]() {
        addActionRow(QStringLiteral("NewAction"), {});
    });
    connect(removeActionBtn, &QPushButton::clicked, this, [this]() {
        const int row = m_actionTable->currentRow();
        if (row >= 0) m_actionTable->removeRow(row);
    });

    layout->addSpacing(12);

    // ── 轴 ──
    auto *axisTitle = new QLabel(tr("轴（Input::GetAxis(\"…\")；负向/正向各可绑多个键）"), page);
    axisTitle->setStyleSheet(QStringLiteral("font-weight: bold;"));
    layout->addWidget(axisTitle);

    m_axisTable = new QTableWidget(0, 3, page);
    m_axisTable->setHorizontalHeaderLabels({ tr("轴名称"), tr("负向"), tr("正向") });
    m_axisTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_axisTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_axisTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_axisTable->verticalHeader()->setVisible(false);
    m_axisTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_axisTable->setAlternatingRowColors(true);
    layout->addWidget(m_axisTable, 1);

    auto *axisButtons = new QHBoxLayout;
    axisButtons->addStretch(1);
    auto *addAxisBtn = new QPushButton(tr("添加轴"), page);
    auto *removeAxisBtn = new QPushButton(tr("删除轴"), page);
    axisButtons->addWidget(addAxisBtn);
    axisButtons->addWidget(removeAxisBtn);
    layout->addLayout(axisButtons);
    connect(addAxisBtn, &QPushButton::clicked, this, [this]() {
        addAxisRow(QStringLiteral("NewAxis"), {}, {});
    });
    connect(removeAxisBtn, &QPushButton::clicked, this, [this]() {
        const int row = m_axisTable->currentRow();
        if (row >= 0) m_axisTable->removeRow(row);
    });
}

void ProjectSettingsDialog::loadInputFromProject()
{
    const nlohmann::json mappings = m_project.inputMappings();

    if (mappings.contains("actions") && mappings["actions"].is_object()) {
        for (auto it = mappings["actions"].begin(); it != mappings["actions"].end(); ++it) {
            QStringList keys;
            if (it->is_array()) {
                for (const auto &k : *it)
                    if (k.is_string()) keys << QString::fromStdString(k.get<std::string>());
            }
            addActionRow(QString::fromStdString(it.key()), keys);
        }
    }
    if (mappings.contains("axes") && mappings["axes"].is_object()) {
        for (auto it = mappings["axes"].begin(); it != mappings["axes"].end(); ++it) {
            QStringList negative, positive;
            const auto &axis = *it;
            if (axis.is_object()) {
                for (const auto &k : axis.value("negative", nlohmann::json::array()))
                    if (k.is_string()) negative << QString::fromStdString(k.get<std::string>());
                for (const auto &k : axis.value("positive", nlohmann::json::array()))
                    if (k.is_string()) positive << QString::fromStdString(k.get<std::string>());
            }
            addAxisRow(QString::fromStdString(it.key()), negative, positive);
        }
    }
}

void ProjectSettingsDialog::addActionRow(const QString &name, const QStringList &bindings)
{
    const int row = m_actionTable->rowCount();
    m_actionTable->insertRow(row);
    auto *nameItem = new QTableWidgetItem(name);
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
    m_actionTable->setItem(row, 0, nameItem);
    auto *cell = new BindingListWidget(m_actionTable);
    cell->setBindings(bindings);
    m_actionTable->setRowHeight(row, 40);
    m_actionTable->setCellWidget(row, 1, cell);
}

void ProjectSettingsDialog::addAxisRow(const QString &name,
                                       const QStringList &negative, const QStringList &positive)
{
    const int row = m_axisTable->rowCount();
    m_axisTable->insertRow(row);
    auto *nameItem = new QTableWidgetItem(name);
    nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
    m_axisTable->setItem(row, 0, nameItem);
    auto *negCell = new BindingListWidget(m_axisTable);
    negCell->setBindings(negative);
    auto *posCell = new BindingListWidget(m_axisTable);
    posCell->setBindings(positive);
    m_axisTable->setRowHeight(row, 40);
    m_axisTable->setCellWidget(row, 1, negCell);
    m_axisTable->setCellWidget(row, 2, posCell);
}

QStringList ProjectSettingsDialog::duplicateKeys() const
{
    QMap<QString, int> count;
    auto collect = [](QTableWidget *table, int column, QMap<QString, int> &counter) {
        for (int row = 0; row < table->rowCount(); ++row) {
            auto *cell = qobject_cast<BindingListWidget *>(table->cellWidget(row, column));
            if (!cell) continue;
            for (const QString &k : cell->bindings()) counter[k] += 1;
        }
    };
    collect(m_actionTable, 1, count);
    collect(m_axisTable, 1, count);
    collect(m_axisTable, 2, count);

    QStringList dups;
    for (auto it = count.constBegin(); it != count.constEnd(); ++it)
        if (it.value() > 1) dups << it.key();
    return dups;
}

void ProjectSettingsDialog::tryAccept()
{
    const QStringList dups = duplicateKeys();
    if (!dups.isEmpty()) {
        const QMessageBox::StandardButton ret = QMessageBox::warning(
            this, tr("重复按键"),
            tr("以下按键被多个动作/轴绑定（运行时可能互相冲突）：\n%1\n\n仍要保存吗？")
                .arg(dups.join(QStringLiteral("、"))),
            QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Cancel);
        if (ret != QMessageBox::Save) return;
    }
    accept();
}

void ProjectSettingsDialog::applyToProject()
{
    // 通用页
    m_project.setSdkDir(m_sdkEdit->text());
    m_project.setScenePath(m_sceneCombo->currentData().toString());   // 空 → 清除 scene 字段
    m_project.setIdeExePath(m_ideCombo->currentData().toString());    // 空 → 清除 editor.ideExe

    // 输入页 → config.json 的 inputMappings（与引擎 settings.json 同构）
    nlohmann::json mappings;
    mappings["actions"] = nlohmann::json::object();
    mappings["axes"] = nlohmann::json::object();

    for (int row = 0; row < m_actionTable->rowCount(); ++row) {
        const QString name = m_actionTable->item(row, 0)->text().trimmed();
        if (name.isEmpty()) continue;
        auto *cell = qobject_cast<BindingListWidget *>(m_actionTable->cellWidget(row, 1));
        const QStringList keys = cell ? cell->bindings() : QStringList();
        if (keys.isEmpty()) continue;
        nlohmann::json arr = nlohmann::json::array();
        for (const QString &k : keys) arr.push_back(k.toStdString());
        mappings["actions"][name.toStdString()] = std::move(arr);
    }
    for (int row = 0; row < m_axisTable->rowCount(); ++row) {
        const QString name = m_axisTable->item(row, 0)->text().trimmed();
        if (name.isEmpty()) continue;
        auto *negCell = qobject_cast<BindingListWidget *>(m_axisTable->cellWidget(row, 1));
        auto *posCell = qobject_cast<BindingListWidget *>(m_axisTable->cellWidget(row, 2));
        nlohmann::json axis;
        if (negCell && !negCell->bindings().isEmpty()) {
            axis["negative"] = nlohmann::json::array();
            for (const QString &k : negCell->bindings()) axis["negative"].push_back(k.toStdString());
        }
        if (posCell && !posCell->bindings().isEmpty()) {
            axis["positive"] = nlohmann::json::array();
            for (const QString &k : posCell->bindings()) axis["positive"].push_back(k.toStdString());
        }
        if (!axis.empty()) mappings["axes"][name.toStdString()] = std::move(axis);
    }
    m_project.setInputMappings(mappings);
}