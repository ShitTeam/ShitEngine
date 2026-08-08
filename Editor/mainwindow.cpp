#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QCloseEvent>
#include <QDockWidget>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QWheelEvent>

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>

#include <algorithm>
#include <fstream>
#include <limits>
#include <optional>
#include <vector>

#include "viewport.h"
#include "scenetree.h"
#include "inspector.h"
#include "logwidget.h"
#include "preview.h"
#include "assetsdock.h"
#include "undostack.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>
#include <ShitEngine/Scene/SceneSerializer.h>



namespace {
constexpr int kMaxRecentScenes = 5;   ///< 最近场景列表长度上限

/// P12：Qt 键盘键 → SDL 扫描码（播放态输入转发；未识别返回 UNKNOWN 丢弃）
SDL_Scancode qtKeyToSDLScancode(int qtKey)
{
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
        return static_cast<SDL_Scancode>(SDL_SCANCODE_A + (qtKey - Qt::Key_A));
    if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
        return static_cast<SDL_Scancode>(SDL_SCANCODE_0 + (qtKey - Qt::Key_0));
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12)
        return static_cast<SDL_Scancode>(SDL_SCANCODE_F1 + (qtKey - Qt::Key_F1));

    switch (qtKey) {
        case Qt::Key_Space:       return SDL_SCANCODE_SPACE;
        case Qt::Key_Return:      return SDL_SCANCODE_RETURN;
        case Qt::Key_Enter:       return SDL_SCANCODE_RETURN;   // 小键盘回车
        case Qt::Key_Backspace:   return SDL_SCANCODE_BACKSPACE;
        case Qt::Key_Tab:         return SDL_SCANCODE_TAB;
        case Qt::Key_Escape:      return SDL_SCANCODE_ESCAPE;
        case Qt::Key_Delete:      return SDL_SCANCODE_DELETE;
        case Qt::Key_Home:        return SDL_SCANCODE_HOME;
        case Qt::Key_End:         return SDL_SCANCODE_END;
        case Qt::Key_PageUp:      return SDL_SCANCODE_PAGEUP;
        case Qt::Key_PageDown:    return SDL_SCANCODE_PAGEDOWN;
        case Qt::Key_Up:          return SDL_SCANCODE_UP;
        case Qt::Key_Down:        return SDL_SCANCODE_DOWN;
        case Qt::Key_Left:        return SDL_SCANCODE_LEFT;
        case Qt::Key_Right:       return SDL_SCANCODE_RIGHT;
        case Qt::Key_Shift:       return SDL_SCANCODE_LSHIFT;
        case Qt::Key_Control:     return SDL_SCANCODE_LCTRL;
        case Qt::Key_Alt:         return SDL_SCANCODE_LALT;
        case Qt::Key_Meta:        return SDL_SCANCODE_LGUI;
        case Qt::Key_Semicolon:   return SDL_SCANCODE_SEMICOLON;
        case Qt::Key_Apostrophe:  return SDL_SCANCODE_APOSTROPHE;
        case Qt::Key_Comma:       return SDL_SCANCODE_COMMA;
        case Qt::Key_Period:      return SDL_SCANCODE_PERIOD;
        case Qt::Key_Slash:       return SDL_SCANCODE_SLASH;
        case Qt::Key_Backslash:   return SDL_SCANCODE_BACKSLASH;
        case Qt::Key_BracketLeft: return SDL_SCANCODE_LEFTBRACKET;
        case Qt::Key_BracketRight:return SDL_SCANCODE_RIGHTBRACKET;
        case Qt::Key_Minus:       return SDL_SCANCODE_MINUS;
        case Qt::Key_Equal:       return SDL_SCANCODE_EQUALS;
        case Qt::Key_QuoteLeft:   return SDL_SCANCODE_GRAVE;
        default:                  return SDL_SCANCODE_UNKNOWN;
    }
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_sceneViewport(nullptr)
    , m_gameViewport(nullptr)
    , m_sceneTree(nullptr)
    , m_inspector(nullptr)
    , m_log(nullptr)
    , m_preview(nullptr)
    , m_settings(QStringLiteral("ShitTeam"), QStringLiteral("ShitEngineEditor"))
{
    ui->setupUi(this);

    // 窗口图标：引擎 logo（Qt 资源系统编入，Editor/resources/logo.png 与仓库根同源）
    setWindowIcon(QIcon(QStringLiteral(":/resources/logo.png")));

    createDocks();
    createMenus();
    createToolbar();

    // P13：Dock 布局持久化 —— 首次启动存出厂默认，之后恢复上次布局；窗口几何同理
    {
        const QByteArray userLayout = m_settings.value("dockState").toByteArray();
        if (userLayout.isEmpty())
            m_settings.setValue("dockStateDefault", saveState(0));
        else
            restoreState(userLayout);
        restoreGeometry(m_settings.value("windowGeometry").toByteArray());
    }

    // 场景视图点击 → 拾取（须在双视口创建后连接）
    connect(m_sceneViewport, &Viewport::logicalClicked, this, &MainWindow::pickSceneAt);

    // P8 编辑会话安全：任何编辑动作 → dirty。Gizmo 拖拽结束 / 树操作置脏在 P9 连接中同步置位，
    // 此处保留"编辑进行中"即置脏的检查器路径（字段一旦改动标题立刻带 *）。
    connect(m_inspector, &Inspector::fieldEdited, this, [this] { setDirty(true); });

    // P9 撤销/重做：编辑手势 begin → commit（before/after 全场景快照对比）
    m_undo.setSnapshotter([this] { return snapshot(); });
    connect(m_sceneViewport, &Viewport::gizmoDragStarted, this, [this] { undoBegin(); });
    connect(m_sceneViewport, &Viewport::gizmoDragFinished, this, [this] {
        undoCommit(tr("移动对象"));
        setDirty(true);
    });
    connect(m_inspector, &Inspector::fieldEdited, this, [this] { undoBegin(); });
    connect(m_inspector, &Inspector::fieldCommitted, this, [this] { undoCommit(tr("编辑属性")); });
    connect(m_sceneTree, &SceneTree::sceneActionStarted, this, [this] { undoBegin(); });
    connect(m_sceneTree, &SceneTree::sceneEdited, this, [this] {
        undoCommit(tr("场景结构编辑"));
        setDirty(true);
    });

    // P10 资源面板：双击 .scene 打开 / 拖图片到视口创建精灵
    connect(m_assets, &AssetsDock::sceneOpenRequested, this, [this](const QString &path) {
        if (confirmDiscardChanges(tr("打开场景")))
            openScenePath(path);
    });
    connect(m_sceneViewport, &Viewport::assetDropped, this, &MainWindow::onViewportAssetDropped);

    // 单一引擎预览：共享场景，双视口同源（编辑一处，双视图同步）
    m_preview = new EnginePreview(this);
    connect(m_preview, &EnginePreview::sceneFrameReady, m_sceneViewport, &Viewport::setFrame);
    connect(m_preview, &EnginePreview::gameFrameReady, m_gameViewport, &Viewport::setFrame);
    connect(m_preview, &EnginePreview::sceneFrameReady, m_inspector, &Inspector::refresh); // 每帧回读引擎值

    // P3：场景树选中 → 属性检查器 + 场景视图 Gizmo
    connect(m_sceneTree, &SceneTree::objectSelected, this, [this](Shit::GameObject *obj) {
        m_inspector->setGameObject(obj);
        m_sceneViewport->setSelectedObject(obj);
    });
    connect(m_inspector, &Inspector::buildInfo, this, [this](int components, int fields) {
        m_log->appendMessage(QString("检查器: 渲染 %1 个组件 / %2 个字段").arg(components).arg(fields));
    });

    // P12：引擎日志 → 日志面板（队列连接，兼容引擎任意线程抵达；spdlog 等级 0=trace..5=critical）
    connect(m_preview, &EnginePreview::engineLogMessage, this,
            [this](bool isCore, int level, const QString &message) {
                QColor color(180, 190, 200);
                if (level >= 4) color = Qt::red;                    // err/critical
                else if (level == 3) color = QColor(230, 150, 0);   // warn
                m_log->appendMessage(QString("[%1] %2").arg(isCore ? tr("引擎") : tr("游戏"), message), color);
            }, Qt::QueuedConnection);

    // P12：播放态运行视口捕获键鼠 → 合成 SDL 事件转发给引擎（见 eventFilter）
    m_gameViewport->setFocusPolicy(Qt::StrongFocus);
    m_gameViewport->setMouseTracking(true);
    m_gameViewport->installEventFilter(this);

    if (m_preview->start()) {
        m_log->appendMessage(tr("预览已启动（场景 + 运行，共享场景）"));

        // 场景树绑定共享场景（自动选中第一项 → 检查器 + Gizmo）
        m_sceneTree->setScene(m_preview->getScene());
        m_sceneViewport->setEditScene(m_preview->getScene()); // 编辑器交互（平移/缩放/Gizmo）
        setPlaying(m_playAction->isChecked());   // 默认停止态：暂停预览逻辑
    } else {
        m_log->appendMessage(tr("预览启动失败"), Qt::red);
    }

    m_savedSnapshot = snapshot();   // 启动初始场景作为存档基准（撤销/重做的 * 对比）
    updateUndoActions();
    updateWindowTitle();   // 初始标题（未命名场景）
    statusBar()->showMessage(tr("就绪"));
    m_log->appendMessage(tr("ShitEngine 编辑器已启动"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createDocks()
{
    // 所有面板可拖拽 Tab 合并：拖标题栏到另一个面板中央 → 变标签页（再拖走恢复独立）。
    // AllowTabbedDocks 显式开启标签化；AllowNestedDocks 支持嵌套停靠；AnimatedDocks 平滑过渡。
    setDockOptions(QMainWindow::AnimatedDocks | QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);

    // 双视口 + 检查器全部做成独立 Dock（可拖动/停靠/浮动/Tab 合并，对齐 Unity 面板习惯）：
    // 中央改空占位——主视口不再是中央 widget，拖走面板后不残留空白。
    m_sceneViewport = new Viewport(this);
    m_gameViewport = new Viewport(this);
    auto *centerPlaceholder = new QWidget(this);   // 空占位（dock 覆盖中央区）
    setCentralWidget(centerPlaceholder);

    // 左侧：场景树
    auto *sceneDock = new QDockWidget(tr("场景"), this);
    sceneDock->setObjectName("sceneDock");
    m_sceneTree = new SceneTree(sceneDock);
    sceneDock->setWidget(m_sceneTree);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);

    // 右侧上方：场景视口（独立 Dock）
    auto *sceneViewportDock = new QDockWidget(tr("场景视口"), this);
    sceneViewportDock->setObjectName("sceneViewportDock");
    sceneViewportDock->setWidget(m_sceneViewport);
    addDockWidget(Qt::RightDockWidgetArea, sceneViewportDock);

    // 右侧：运行视口（独立 Dock）
    auto *gameDock = new QDockWidget(tr("运行视口"), this);
    gameDock->setObjectName("gameViewportDock");
    gameDock->setWidget(m_gameViewport);
    addDockWidget(Qt::RightDockWidgetArea, gameDock);

    // 右侧下方：属性检查器
    auto *inspectorDock = new QDockWidget(tr("属性"), this);
    inspectorDock->setObjectName("inspectorDock");
    m_inspector = new Inspector(inspectorDock);
    inspectorDock->setWidget(m_inspector);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // 底部：资源面板 + 日志（并排）
    auto *assetsDock = new QDockWidget(tr("资源"), this);
    assetsDock->setObjectName("assetsDock");
    m_assets = new AssetsDock(assetsDock);
    assetsDock->setWidget(m_assets);
    addDockWidget(Qt::BottomDockWidgetArea, assetsDock);

    auto *logDock = new QDockWidget(tr("日志"), this);
    logDock->setObjectName("logDock");
    m_log = new LogWidget(logDock);
    logDock->setWidget(m_log);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);

    setDockNestingEnabled(true);
    resize(1280, 800);
}

void MainWindow::createMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("文件"));

    auto *newAction = fileMenu->addAction(tr("新建场景"), this, &MainWindow::newScene);
    newAction->setShortcut(QKeySequence::New);
    auto *openAction = fileMenu->addAction(tr("打开场景…"), this, &MainWindow::openScene);
    openAction->setShortcut(QKeySequence::Open);

    // P8：最近场景（QSettings 持久化，最多 kMaxRecentScenes 条）
    m_recentMenu = fileMenu->addMenu(tr("最近场景"));
    updateRecentMenu();

    fileMenu->addSeparator();
    auto *saveAction = fileMenu->addAction(tr("保存场景"), this, &MainWindow::saveScene);
    saveAction->setShortcut(QKeySequence::Save);
    auto *saveAsAction = fileMenu->addAction(tr("场景另存为…"), this, &MainWindow::saveSceneAs);
    saveAsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), this, &QWidget::close);

    // P9：撤销/重做（快照型命令栈）
    auto *editMenu = menuBar()->addMenu(tr("编辑"));
    m_undoAction = editMenu->addAction(tr("撤销"), this, &MainWindow::undo);
    m_undoAction->setShortcut(QKeySequence::Undo);   // Ctrl+Z
    m_redoAction = editMenu->addAction(tr("重做"), this, &MainWindow::redo);
    m_redoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
    updateUndoActions();

    // P13：视图菜单（布局恢复）
    auto *viewMenu = menuBar()->addMenu(tr("视图"));
    viewMenu->addAction(tr("恢复默认布局"), this, &MainWindow::resetDockLayout);

    auto *helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(tr("关于"), this, &MainWindow::about);
}

void MainWindow::newScene()
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return;

    if (!confirmDiscardChanges(tr("新建场景")))
        return;

    // 先收集再删：编辑器下 removeGameObject 当场 erase，不能在遍历中删（迭代器失效）
    std::vector<Shit::GameObject *> toRemove;
    for (auto &go : scene->getGameObjects()) {
        const std::string name = go->getName();
        if (name != "scene_camera" && name != "game_camera")   // 保留两个相机（模板）
            toRemove.push_back(go.get());
    }
    for (auto *go : toRemove)
        scene->removeGameObject(go);

    m_scenePath.clear();
    m_undo.clear();
    m_savedSnapshot = snapshot();   // 新空场景作为存档基准（dirty 对比）
    refreshDirtyFromSaved();
    updateUndoActions();
    m_sceneTree->setScene(scene);
    m_log->appendMessage(tr("已新建空场景"));
    statusBar()->showMessage(tr("已新建空场景"));
}

void MainWindow::openScene()
{
    if (!confirmDiscardChanges(tr("打开场景")))
        return;

    const QString path = QFileDialog::getOpenFileName(this, tr("打开场景"), QString(), tr("ShitEngine 场景 (*.scene)"));
    if (path.isEmpty()) return;
    openScenePath(path);
}

bool MainWindow::openScenePath(const QString &path)
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return false;

    // 打开前全量快照（含编辑器相机）：fromJson 失败时整体恢复，保证不丢数据
    std::optional<nlohmann::json> backup;
    try {
        std::ifstream file(path.toStdString());
        if (!file.is_open()) {
            m_log->appendMessage(tr("无法打开文件: %1").arg(path), Qt::red);
            return false;
        }
        nlohmann::json doc;
        file >> doc;   // 解析失败在此抛出 —— 场景尚未被触碰，无需回滚

        // 结构校验：.scene 必须有 objects 数组（v1/v2 均如此）。
        // 缺数组会走得通 fromJson 的"仅相机兜底"分支——那会把当前场景清成空，
        // 视为损坏文件，必须在不触碰场景的情况下拒绝。
        if (!doc.is_object() || !doc.contains("objects") || !doc["objects"].is_array()) {
            m_log->appendMessage(tr("打开场景失败：文件不是有效的 .scene（缺少 objects 数组）：%1").arg(path), Qt::red);
            return false;
        }

        backup = Shit::SceneSerializer::toJson(scene);

        // 先收集再删（编辑器下 removeGameObject 当场 erase，不能在遍历中删）
        std::vector<Shit::GameObject *> toRemove;
        for (auto &go : scene->getGameObjects())
            if (go->getName() != "scene_camera")
                toRemove.push_back(go.get());
        for (auto *go : toRemove)
            scene->removeGameObject(go);

        // 共享加载器：对象 + 组件 + 层级 + 相机兜底（唯一 .scene 事实标准）
        Shit::SceneSerializer::fromJson(doc, scene);   // 可能抛异常 → 统一走回滚

        // 诊断：载入后的对象清单
        QString names;
        for (auto &go : scene->getGameObjects()) {
            if (!names.isEmpty()) names += ", ";
            names += QString::fromStdString(go->getName());
        }
        m_log->appendMessage(QString("载入后对象(%1): %2").arg(scene->getGameObjects().size()).arg(names));

        m_scenePath = path;
        m_undo.clear();
        m_savedSnapshot = snapshot();   // 载入态作为存档基准
        refreshDirtyFromSaved();
        updateUndoActions();
        addRecentScene(path);
        m_sceneTree->setScene(scene);
        m_log->appendMessage(tr("场景已从 %1 载入").arg(path));
        statusBar()->showMessage(tr("打开场景完成"));
        return true;
    } catch (const std::exception &e) {
        if (backup) {
            rollbackScene(*backup);
            m_log->appendMessage(tr("打开场景失败: %1 —— 已回滚到原场景").arg(e.what()), Qt::red);
            statusBar()->showMessage(tr("打开场景失败，已回滚"));
        } else {
            m_log->appendMessage(tr("打开场景失败: %1").arg(e.what()), Qt::red);
        }
        return false;
    }
}

void MainWindow::rollbackScene(const nlohmann::json &snapshot)
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene || snapshot.empty()) return;

    // 先清掉指向旧对象的选中态（对象即将销毁），再清场
    m_inspector->setGameObject(nullptr);
    m_sceneViewport->setSelectedObject(nullptr);

    // 全清（含编辑器相机；快照里都有）
    std::vector<Shit::GameObject *> all;
    for (auto &go : scene->getGameObjects())
        all.push_back(go.get());
    for (auto *go : all)
        scene->removeGameObject(go);

    // 从快照整体重建（快照内已有相机，相机兜底不会新增）
    try {
        Shit::SceneSerializer::fromJson(snapshot, scene);
    } catch (const std::exception &e) {
        m_log->appendMessage(tr("回滚失败: %1 —— 场景可能不完整").arg(e.what()), Qt::red);
    }

    m_sceneTree->setScene(scene);   // 自动重选第一项 → 检查器/Gizmo 联动刷新
    refreshDirtyFromSaved();        // 恢复的是打开前状态，dirty 应与存档基准一致
}

// ═════════════════════════ P9 撤销/重做 ═════════════════════════

nlohmann::json MainWindow::snapshot() const
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return nlohmann::json();
    // 排除编辑器相机（scene_camera 不入库、不参与撤销，保持编辑视点稳定）
    return Shit::SceneSerializer::toJson(scene, { "scene_camera" });
}

void MainWindow::refreshDirtyFromSaved()
{
    m_dirty = !(snapshot() == m_savedSnapshot);
    updateWindowTitle();
}

void MainWindow::applySnapshot(const nlohmann::json &snap)
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene || snap.empty()) return;

    // 保留编辑器相机，其余全清 → 从快照重建
    std::vector<Shit::GameObject *> toRemove;
    for (auto &go : scene->getGameObjects())
        if (go->getName() != "scene_camera")
            toRemove.push_back(go.get());
    for (auto *go : toRemove)
        scene->removeGameObject(go);

    Shit::SceneSerializer::fromJson(snap, scene);
    m_sceneTree->setScene(scene);   // 重挂树 → 自动选中并联动检查器/Gizmo
    refreshDirtyFromSaved();
    updateUndoActions();
}

void MainWindow::undoBegin()
{
    if (isPlaying()) return;   // 运行态编辑不记录
    m_undo.begin();
}

void MainWindow::undoCommit(const QString &label)
{
    if (isPlaying()) return;
    if (m_undo.commit(label))
        updateUndoActions();
}

void MainWindow::undo()
{
    if (isPlaying()) return;
    auto target = m_undo.undo();
    if (!target) {
        statusBar()->showMessage(tr("没有可撤销的操作"), 1500);
        return;
    }
    applySnapshot(*target);
    m_log->appendMessage(tr("已撤销"));
}

void MainWindow::redo()
{
    if (isPlaying()) return;
    auto target = m_undo.redo();
    if (!target) {
        statusBar()->showMessage(tr("没有可重做的操作"), 1500);
        return;
    }
    applySnapshot(*target);
    m_log->appendMessage(tr("已重做"));
}

void MainWindow::updateUndoActions()
{
    if (!m_undoAction || !m_redoAction) return;
    const bool running = isPlaying();
    m_undoAction->setEnabled(!running && m_undo.canUndo());
    m_redoAction->setEnabled(!running && m_undo.canRedo());
}

void MainWindow::onViewportAssetDropped(const QString &path, float logicalX, float logicalY)
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return;

    // 落点逻辑像素 → 世界坐标（用场景内第一个相机，与拾取同源）
    Shit::Vector2 world{ 0.0f, 0.0f };
    for (auto &go : scene->getGameObjects()) {
        if (auto *cam = go->getComponent<Shit::CameraComponent>()) {
            world = cam->screenToWorld({ logicalX, logicalY });
            break;
        }
    }

    undoBegin();
    const QString base = QFileInfo(path).baseName();
    auto *go = scene->createGameObject("Sprite_" + base.toStdString());
    if (auto *t = go->addComponent<Shit::TransformComponent>())
        t->setPosition(world);
    if (auto *sr = go->addComponent<Shit::SpriteRenderer>())
        sr->setTexturePath(path.toStdString());
    undoCommit(tr("拖入图片 %1").arg(base));
    setDirty(true);

    m_sceneTree->setScene(scene);   // 刷新层级（新对象入模型）
    m_sceneTree->selectObject(go);  // 选中新精灵 → 检查器 + Gizmo
    m_log->appendMessage(QString("已从资源创建精灵: %1 @(%2, %3)")
        .arg(base).arg(world.x, 0, 'f', 1).arg(world.y, 0, 'f', 1));
}

bool MainWindow::saveScene()
{
    if (m_scenePath.isEmpty())
        return saveSceneAs();
    return saveSceneTo(m_scenePath);
}

bool MainWindow::saveSceneAs()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("保存场景为"), m_scenePath, tr("ShitEngine 场景 (*.scene)"));
    if (path.isEmpty()) return false;
    return saveSceneTo(path);
}

bool MainWindow::saveSceneTo(const QString &path)
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return false;

    // 编辑器相机（scene_camera）是编辑基础设施，不入库；其余对象含层级整体序列化（v2）
    const nlohmann::json doc = Shit::SceneSerializer::toJson(scene, { "scene_camera" });

    {
        std::ofstream file(path.toStdString(), std::ios::trunc);
        if (!file.is_open()) {
            m_log->appendMessage(tr("保存失败：无法写入 %1").arg(path), Qt::red);
            return false;
        }
        file << doc.dump(2);
        file.flush();
        if (!file.good()) {
            m_log->appendMessage(tr("保存失败：写入出错 %1").arg(path), Qt::red);
            return false;
        }
    }

    const size_t objectCount = doc.contains("objects") && doc["objects"].is_array()
        ? doc["objects"].size() : 0;
    m_scenePath = path;
    m_savedSnapshot = snapshot();   // 存档快照刷新（undo 栈保留，可继续撤销到更早）
    refreshDirtyFromSaved();
    updateUndoActions();
    addRecentScene(path);
    m_log->appendMessage(tr("场景已保存到 %1（%2 个对象）").arg(path).arg(objectCount));
    statusBar()->showMessage(tr("场景已保存"));
    return true;
}

bool MainWindow::confirmDiscardChanges(const QString &actionName)
{
    if (!m_dirty) return true;

    QMessageBox box(QMessageBox::Warning, tr("未保存的更改"),
                    tr("当前场景有未保存的修改。\n在%1之前要保存更改吗？").arg(actionName),
                    QMessageBox::NoButton, this);
    QPushButton *saveBtn = box.addButton(tr("保存"), QMessageBox::AcceptRole);
    QPushButton *discardBtn = box.addButton(tr("不保存"), QMessageBox::DestructiveRole);
    box.addButton(tr("取消"), QMessageBox::RejectRole);
    box.setDefaultButton(saveBtn);

    box.exec();
    if (box.clickedButton() == saveBtn)
        return saveScene();          // 保存被取消/失败 → 视为不继续
    return box.clickedButton() == discardBtn;
}

void MainWindow::setDirty(bool dirty)
{
    if (m_dirty == dirty) return;
    m_dirty = dirty;
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    const QString sceneName = m_scenePath.isEmpty()
        ? tr("未命名场景")
        : QFileInfo(m_scenePath).fileName();
    QString title = sceneName;
    if (m_dirty) title += QStringLiteral(" *");
    title += QStringLiteral(" - ") + tr("ShitEngine 编辑器");
    setWindowTitle(title);
}

QStringList MainWindow::recentScenes() const
{
    QStringList list = m_settings.value("recentScenes").toStringList();
    // 剔除已被删除的文件
    list.erase(std::remove_if(list.begin(), list.end(),
        [](const QString &p) { return !QFileInfo::exists(p); }), list.end());
    return list;
}

void MainWindow::addRecentScene(const QString &path)
{
    QStringList list = recentScenes();
    list.removeAll(path);
    list.prepend(path);
    while (list.size() > kMaxRecentScenes)
        list.removeLast();
    m_settings.setValue("recentScenes", list);
    updateRecentMenu();
}

void MainWindow::updateRecentMenu()
{
    if (!m_recentMenu) return;
    m_recentMenu->clear();

    const QStringList recents = recentScenes();
    if (recents.isEmpty()) {
        m_recentMenu->addAction(tr("（空）"))->setEnabled(false);
        return;
    }
    for (const QString &path : recents) {
        auto *act = m_recentMenu->addAction(QFileInfo(path).fileName());
        act->setToolTip(path);
        connect(act, &QAction::triggered, this, [this, path]() {
            if (confirmDiscardChanges(tr("打开场景")))
                openScenePath(path);
        });
    }
}

void MainWindow::resetDockLayout()
{
    const QByteArray def = m_settings.value("dockStateDefault").toByteArray();
    if (!def.isEmpty()) {
        restoreState(def);
        statusBar()->showMessage(tr("已恢复默认布局"), 2000);
    } else {
        statusBar()->showMessage(tr("当前布局即为默认"), 2000);
    }
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("关于 ShitEngine 编辑器"),
        tr("<b>ShitEngine 编辑器</b>　v0.1（Qt 6 Widgets + ShitEngine 引擎）<br><br>"
           "编辑器把引擎<u>进程内嵌</u>，场景唯一事实来源为 .scene 文件，"
           "编辑 / 运行 / 切关共用同一加载器。<br><br>"
           "<b>快捷键</b><br>"
           "· Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S — 新建 / 打开 / 保存 / 另存为<br>"
           "· Ctrl+Z / Ctrl+Shift+Z — 撤销 / 重做<br>"
           "· Q / W / E — Gizmo 移动 / 旋转 / 缩放（Ctrl 吸附）<br>"
           "· F2 — 重命名对象；Del — 删除对象<br>"
           "· 滚轮缩放视图 / 中键拖拽平移相机<br>"
           "· ▶ 播放后点击运行视口即可用键鼠驱动游戏<br><br>"
           "Dock 布局自动记忆；「视图 → 恢复默认布局」可重置。"));
}

void MainWindow::createToolbar()
{
    auto *toolbar = addToolBar(tr("控制"));
    toolbar->setMovable(false);

    // ▶ 播放 / ⏹ 停止：控制预览引擎逻辑运行（默认停止）
    m_playAction = toolbar->addAction(tr("▶ 播放"));
    m_playAction->setCheckable(true);
    m_playAction->setChecked(false);
    connect(m_playAction, &QAction::toggled, this, &MainWindow::setPlaying);

    // P11：Gizmo 三模式（移动/旋转/缩放，Q/W/E 快捷键）
    toolbar->addSeparator();
    m_gizmoGroup = new QActionGroup(this);

    auto addGizmoAction = [this, toolbar](const QString &text, Qt::Key key,
                                          Viewport::GizmoMode mode) {
        auto *act = toolbar->addAction(text);
        act->setCheckable(true);
        act->setShortcut(QKeySequence(key));
        act->setChecked(mode == Viewport::GizmoMode::Move);
        m_gizmoGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, mode]() {
            if (m_sceneViewport) m_sceneViewport->setGizmoMode(mode);
        });
        return act;
    };
    addGizmoAction(tr("移动"), Qt::Key_Q, Viewport::GizmoMode::Move);
    addGizmoAction(tr("旋转"), Qt::Key_W, Viewport::GizmoMode::Rotate);
    addGizmoAction(tr("缩放"), Qt::Key_E, Viewport::GizmoMode::Scale);

    // P13：工具栏增补 —— 撤销 / 重做（与菜单共享同一 QAction）
    toolbar->addSeparator();
    toolbar->addAction(m_undoAction);
    toolbar->addAction(m_redoAction);
}

void MainWindow::setPlaying(bool playing)
{
    if (m_playAction)
        m_playAction->setText(playing ? tr("⏹ 停止") : tr("▶ 播放"));
    if (m_preview) m_preview->setPlaying(playing);
    updateUndoActions();   // 运行态撤销/重做禁用
    statusBar()->showMessage(playing ? tr("运行中") : tr("已暂停"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (confirmDiscardChanges(tr("退出编辑器"))) {
        // P13：退出前记忆 Dock 布局与窗口几何（下次启动恢复）
        m_settings.setValue("dockState", saveState(0));
        m_settings.setValue("windowGeometry", saveGeometry());
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // P12：仅播放态、且事件来自运行视口时转发给引擎（合成 SDL_Event）
    if (watched != m_gameViewport)
        return QMainWindow::eventFilter(watched, event);
    if (!isPlaying() || !m_preview || !m_preview->context())
        return QMainWindow::eventFilter(watched, event);

    // 注入前切到预览引擎上下文（Input 单例按 EngineContext::current() 定位）
    Shit::EngineContext::setCurrent(m_preview->context());

    switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease: {
            auto *ke = static_cast<QKeyEvent *>(event);
            const SDL_Scancode sc = qtKeyToSDLScancode(ke->key());
            if (sc == SDL_SCANCODE_UNKNOWN) return false;
            SDL_Event ev{};
            ev.type = (event->type() == QEvent::KeyPress) ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
            ev.key.scancode = sc;
            Shit::Input::HandleEvent(ev);
            return false;   // 不吞噬，Qt 照常处理
        }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease: {
            auto *me = static_cast<QMouseEvent *>(event);
            const QPointF logical = m_gameViewport->mapToLogical(me->pos());
            Shit::Input::SetMousePosition({ static_cast<float>(logical.x()), static_cast<float>(logical.y()) });
            int sdlBtn = 0;
            if (me->button() == Qt::LeftButton) sdlBtn = 1;
            else if (me->button() == Qt::MiddleButton) sdlBtn = 2;
            else if (me->button() == Qt::RightButton) sdlBtn = 3;
            if (sdlBtn > 0) {
                SDL_Event ev{};
                ev.type = (event->type() == QEvent::MouseButtonPress)
                    ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
                ev.button.button = static_cast<Uint8>(sdlBtn);
                Shit::Input::HandleEvent(ev);
            }
            return false;
        }
        case QEvent::MouseMove: {
            auto *me = static_cast<QMouseEvent *>(event);
            const QPointF logical = m_gameViewport->mapToLogical(me->pos());
            Shit::Input::SetMousePosition({ static_cast<float>(logical.x()), static_cast<float>(logical.y()) });
            return false;
        }
        case QEvent::Wheel: {
            auto *we = static_cast<QWheelEvent *>(event);
            SDL_Event ev{};
            ev.type = SDL_EVENT_MOUSE_WHEEL;
            ev.wheel.y = we->angleDelta().y() / 120.0f;
            Shit::Input::HandleEvent(ev);
            return false;
        }
        default:
            break;
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::pickSceneAt(float x, float y)
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) { m_log->appendMessage(tr("pick: 无场景"), Qt::red); return; }

    // 找到场景视图用的相机（编辑器相机）
    Shit::CameraComponent *camera = nullptr;
    for (auto &go : scene->getGameObjects()) {
        if (auto *cam = go->getComponent<Shit::CameraComponent>()) { camera = cam; break; }
    }
    if (!camera) { m_log->appendMessage(tr("pick: 未找到相机"), Qt::red); return; }

    // 逻辑像素点击
    const Shit::Vector2 click(x, y);

    // 拾取：正变换法 —— 将每个精灵的世界包围盒用相机正向映射成屏幕矩形，
    // 测点击是否落在其中（与渲染同变换，屏幕位置精确一致）。
    // 加小容差(tol)，容忍边缘点击的少量偏差，贴近真实编辑器手感。
    const float tol = 6.0f;
    Shit::GameObject *hit = nullptr;
    int bestZ = std::numeric_limits<int>::min();
    for (auto &go : scene->getGameObjects()) {
        if (auto *sprite = go->getComponent<Shit::SpriteRenderer>()) {
            const SDL_FRect b = sprite->getGlobalBounds();
            const Shit::Vector2 tl = camera->worldToScreen({ b.x, b.y });
            const Shit::Vector2 br = camera->worldToScreen({ b.x + b.w, b.y + b.h });
            const float sxl = std::min(tl.x, br.x) - tol;
            const float syt = std::min(tl.y, br.y) - tol;
            const float sxr = std::max(tl.x, br.x) + tol;
            const float syb = std::max(tl.y, br.y) + tol;
            if (click.x >= sxl && click.x <= sxr && click.y >= syt && click.y <= syb) {
                const int z = sprite->getZIndex();
                if (z >= bestZ) { bestZ = z; hit = go.get(); }   // P11：zIndex 越高越优先
            }
        }
    }

    // P11：无精灵命中 → 变换点命中（支持相机/空对象等无几何对象，编辑器相机除外）
    if (!hit) {
        const float kPointTol = 14.0f;
        for (auto &go : scene->getGameObjects()) {
            if (go->getName() == "scene_camera") continue;
            auto *t = go->getComponent<Shit::TransformComponent>();
            if (!t) continue;
            const Shit::Vector2 sp = camera->worldToScreen(t->getPosition());
            if (std::fabs(sp.x - click.x) <= kPointTol && std::fabs(sp.y - click.y) <= kPointTol) {
                hit = go.get();
                break;
            }
        }
    }

    if (hit) {
        m_log->appendMessage(QString("pick: 命中 %1 @logical(%2,%3)")
            .arg(QString::fromStdString(hit->getName())).arg(click.x, 0, 'f', 1).arg(click.y, 0, 'f', 1));
        m_inspector->setGameObject(hit);        // 直接驱动检查器
        m_sceneViewport->setSelectedObject(hit); // 场景视图显示 Gizmo
        m_sceneTree->selectObject(hit);          // 同步场景树高亮
    } else {
        // 诊断：打印首个含精灵的对象其屏幕矩形，区分"点偏了" vs "映射错"
        QString rectInfo;
        for (auto &go : scene->getGameObjects()) {
            if (auto *sp = go->getComponent<Shit::SpriteRenderer>()) {
                const SDL_FRect b = sp->getGlobalBounds();
                const Shit::Vector2 tl = camera->worldToScreen({ b.x, b.y });
                const Shit::Vector2 br = camera->worldToScreen({ b.x + b.w, b.y + b.h });
                rectInfo = QString(" %1 screen[%2..%3]×[%4..%5]")
                    .arg(QString::fromStdString(go->getName()))
                    .arg(tl.x, 0, 'f', 0).arg(br.x, 0, 'f', 0)
                    .arg(tl.y, 0, 'f', 0).arg(br.y, 0, 'f', 0);
                break;
            }
        }
        m_log->appendMessage(QString("pick: 未命中 @logical(%1,%2)%3")
            .arg(click.x, 0, 'f', 1).arg(click.y, 0, 'f', 1).arg(rectInfo), Qt::yellow);
        m_inspector->setGameObject(nullptr); // 点空白 → 清空检查器
    }
}
