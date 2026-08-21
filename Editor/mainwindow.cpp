#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QWheelEvent>

#include <SDL3/SDL_events.h>

#include <unordered_set>
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
#include "tilesetdock.h"
#include "animatordock.h"
#include "animationdock.h"
#include "spritesheetdock.h"
#include "undostack.h"
#include "project.h"
#include "projectwizard.h"
#include "projectsettingsdialog.h"
#include "exportdialog.h"
#include "idefinder.h"
#include "keys.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>
#include <ShitEngine/Animation/Animator.h>

#include <nlohmann/json.hpp>

#include <QFile>
#include <ShitEngine/Scene/SceneSerializer.h>
#include <ShitEngine/System/BehaviorSystem.h>



namespace {
constexpr int kMaxRecentScenes = 5;   ///< 最近场景列表长度上限
constexpr int kMaxRecentProjects = 5; ///< 最近项目列表长度上限

/// Dock 布局版本号（saveState/restoreState 第二参）。P21 起中央改为标签页叠放、
/// 底部资源+日志 Tab 合并——旧版（v0）保存的布局作废，版本不匹配时自动落回默认排列。
constexpr int kLayoutVersion = 2;

/// 复制/粘贴仅在非文本编辑场景生效：检查器/树重命名等输入框获焦时，
/// Ctrl+C/V 应交给文本控件（复制文字），不劫持为"复制对象"
bool isTextEditFocused()
{
    QWidget *w = QApplication::focusWidget();
    return qobject_cast<QLineEdit *>(w)
        || qobject_cast<QTextEdit *>(w)
        || qobject_cast<QPlainTextEdit *>(w);
}

/// 生成场景内唯一名：base / base (1) / base (2)…（树按名区分对象，重名会混乱）
QString uniqueObjectName(Shit::Scene *scene, const std::string &base)
{
    const auto &gos = scene->getGameObjects();
    auto taken = [&](const std::string &name) {
        for (const auto &go : gos)
            if (go->getName() == name) return true;
        return false;
    };
    if (!taken(base)) return QString::fromStdString(base);
    for (int i = 1; ; ++i) {
        const std::string candidate = base + " (" + std::to_string(i) + ")";
        if (!taken(candidate)) return QString::fromStdString(candidate);
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
    , m_tileset(nullptr)
    , m_animatorDock(nullptr)
    , m_animationDock(nullptr)
    , m_settings(QStringLiteral("ShitTeam"), QStringLiteral("ShitEngineEditor"))
{
    ui->setupUi(this);

    // 窗口图标：引擎 logo（Qt 资源系统编入，Editor/resources/logo.png 与仓库根同源）
    setWindowIcon(QIcon(QStringLiteral(":/resources/logo.png")));

    createDocks();
    createMenus();
    createToolbar();

    // P13：Dock 布局持久化 —— 项目级 .shitengine/state.ini（无项目时回退全局注册表）。
    // 有上次布局恢复之；否则恢复「默认布局」（P17：可经「视图 → 将当前布局设为默认」
    // 覆盖，即自定义启动时的初始排列）；首次启动无默认时保存出厂布局为默认。
    // P21：布局带版本号——旧版布局作废时自动落回 createDocks 默认排列。
    // P37b：窗口大小/位置不保存不恢复——启动一律最大化。
    {
        QSettings *s = stateSettings();
        const QByteArray userLayout = s->value("dockState").toByteArray();
        const QByteArray def = s->value("dockStateDefault").toByteArray();
        if (!userLayout.isEmpty() && restoreState(userLayout, kLayoutVersion)) {
            // 已恢复用户布局
        } else if (!def.isEmpty()) {
            restoreState(def, kLayoutVersion);   // 版本不匹配则忽略，沿用默认排列
        } else {
            s->setValue("dockStateDefault", saveState(kLayoutVersion));
        }
        // P37b：不保存窗口大小——每次启动一律最大化。
        showMaximized();
        // P25e：跨分辨率/DPI 恢复的窗口几何可能超出可用屏幕（保存时屏大、恢复时屏小，
        // 右侧「属性」/底部「资源 日志」Dock 会被推到屏幕外）。restoreGeometry 异步生效，
        // 此刻 frameGeometry 尚未更新（Windows SetWindowPos 异步），延迟到几何应用后钳制。
        QTimer::singleShot(120, this, [this] {
            if (const QScreen *screen = QGuiApplication::primaryScreen()) {
                const QRect avail = screen->availableGeometry();
                const QRect fg = frameGeometry();
                // 正溢出（宽/高超出或左边越界；容忍 -8 阴影边距）才钳制，
                // 普通小窗口/多屏布局不受干扰
                if (fg.width() > avail.width() || fg.height() > avail.height()
                    || fg.left() < avail.left() - 8 || fg.top() < avail.top() - 8) {
                    resize(std::min(1280, avail.width() - 80), std::min(800, avail.height() - 80));
                    move(avail.center().x() - width() / 2, avail.center().y() - height() / 2);
                }
            }
        });
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
    // 方案 A：检查器 Animator 入口按钮 → 显示并聚焦状态机 Dock
    connect(m_inspector, &Inspector::openAnimatorEditorRequested, this, &MainWindow::openAnimatorEditor);
    // P29：检查器 AnimationComponent 入口按钮 → 显示并聚焦帧动画 Dock
    connect(m_inspector, &Inspector::openAnimationEditorRequested, this, &MainWindow::openAnimationEditor);
    // P27 增强：瓦片面板选瓦片 → 视口画笔（-2 无选择不刷；-1 橡皮）
    connect(m_tileset, &TilesetDock::tileSelected, m_sceneViewport, &Viewport::setPaintTileId);
    // P28：Animator 状态机窗口编辑 → 撤销 + dirty
    connect(m_animatorDock, &AnimatorDock::changed, this, [this] {
        undoBegin();
        undoCommit(tr("编辑状态机"));
    });
    // P29：Animation 帧动画窗口编辑 .anim → 标脏（标题栏 *）
    connect(m_animationDock, &AnimationDock::changed, this, [this] { setDirty(true); });
    // 方案 A：Animator 状态 → 在 Animation 窗口打开其 .anim 资产
    connect(m_animatorDock, &AnimatorDock::openAssetRequested, this, [this](const QString &path) {
        if (m_animationDock->openFile(path)) {
            m_log->appendMessage(tr("已在 Animation 窗口打开：%1").arg(path));
        } else {
            m_log->appendMessage(tr("打开动画剪辑失败：%1").arg(path), Qt::red);
            return;
        }
        openAnimationEditor();
    });
    // 方案 A：Animation 窗口保存 .anim → 同步引用该资产的 Animator 状态
    connect(m_animationDock, &AnimationDock::saved, this, &MainWindow::reloadAnimatorAsset);
    // 检查器底部 Add Component 添加组件：undo 事务已由 fieldEdited 开启，此处提交
    connect(m_inspector, &Inspector::componentAdded, this, [this] { undoCommit(tr("添加组件")); });
    // 检查器组件头「✕」移除组件 / 顶部名称栏重命名（同前：fieldEdited 已开启事务，此处提交）
    connect(m_inspector, &Inspector::componentRemoved, this, [this] { undoCommit(tr("移除组件")); });
    connect(m_inspector, &Inspector::objectRenamed, this, [this] { undoCommit(tr("重命名")); });
    connect(m_inspector, &Inspector::componentRemoveBlocked, this,
            [this](const QString &reason) { m_log->appendMessage(reason, Qt::red); });
    // 场景系统操作（fieldEdited 已开启事务，此处提交 + dirty）
    connect(m_inspector, &Inspector::systemAdded, this, [this] {
        undoCommit(tr("添加系统"));
        setDirty(true);
    });
    connect(m_inspector, &Inspector::systemRemoved, this, [this] {
        undoCommit(tr("移除系统"));
        setDirty(true);
    });
    connect(m_inspector, &Inspector::systemPriorityChanged, this, [this] {
        undoCommit(tr("调整系统优先级"));
        setDirty(true);
    });
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
    // P25c：.prefab 预置资产（拖入视口实例化 / 双击实例化 / 场景树存为预置）
    connect(m_sceneViewport, &Viewport::prefabDropped, this, &MainWindow::onPrefabDropped);
    connect(m_assets, &AssetsDock::prefabOpenRequested, this, &MainWindow::onPrefabOpenRequested);
    connect(m_sceneTree, &SceneTree::prefabSaveRequested, this, &MainWindow::onSaveObjectAsPrefab);
    // P28：.anim 剪辑资产 → 应用到选中对象的 Animator 状态
    // P29：改为在 Animation 窗口打开（制作/编辑 .anim 资产）；如需应用到 Animator 状态仍可用 onAnimOpenRequested
    connect(m_assets, &AssetsDock::animOpenRequested, this, &MainWindow::onAnimationOpenRequested);
    // P38：双击 .sprite → 切到精灵表 Dock 并打开
    connect(m_assets, &AssetsDock::spriteFileRequested, this, [this](const QString &path) {
        // 找到精灵表 Dock 并显示
        for (auto *dock : m_docks) {
            if (dock->objectName() == "spriteSheetDock") {
                dock->show();
                dock->raise();
                break;
            }
        }
        m_spriteSheetDock->openSpriteFile(path);
    });

    // 单一引擎预览：共享场景，双视口同源（编辑一处，双视图同步）
    m_preview = new EnginePreview(this);
    connect(m_preview, &EnginePreview::sceneFrameReady, this, &MainWindow::onSceneFrameReady);
    connect(m_preview, &EnginePreview::gameFrameReady, m_gameViewport, &Viewport::setFrame);
    connect(m_sceneTree, &SceneTree::sceneDeleteBlocked, this,
            [this](const QString &reason) { m_log->appendMessage(reason, Qt::red); });

    // P3：场景树选中 → 属性检查器 + 场景视图 Gizmo + 瓦片面板 + Animator 状态机窗口
    connect(m_sceneTree, &SceneTree::objectSelected, this, [this](Shit::GameObject *obj) {
        syncInspectorToSelection();
        m_sceneViewport->setSelectedObject(obj);
        m_tileset->setGameObject(obj);
        m_animatorDock->setGameObject(obj);
    });
    // P36：多选增减（Ctrl/Shift）→ 批量编辑模式（单选由 objectSelected 兜底实时刷新）
    connect(m_sceneTree, &SceneTree::selectionChanged, this, [this] {
        // 多选时有 currentChanged 也会触发 objectSelected → 这里只在多选时接管，
        // 单选交给 objectSelected（避免 Gizmo 绑定的对象与 currentIndex 脱节）
        if (m_sceneTree->selectedObjects().size() > 1)
            syncInspectorToSelection();
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

    // P33：插件加载失败 → 延迟弹窗（排队到事件循环空闲，避开构造/加载中途）：
    // 此前 DLL 缺失/ABI 不匹配只进日志面板，用户打开项目后"Add Component 缺类型"难排查
    connect(m_preview, &EnginePreview::pluginLoadFailed, this,
            [this](const QString &detail) {
                QTimer::singleShot(0, this, [this, detail] {
                    QMessageBox::warning(this, tr("插件加载失败"),
                        tr("项目插件加载失败：\n%1\n\n请在「项目 → 项目设置…」检查 SDK 与脚本构建，"
                           "详细输出见日志面板。").arg(detail));
                });
            });

    // P12：播放态运行视口捕获键鼠 → 合成 SDL 事件转发给引擎（见 eventFilter）
    m_gameViewport->setFocusPolicy(Qt::StrongFocus);
    m_gameViewport->setMouseTracking(true);
    m_gameViewport->installEventFilter(this);

    if (m_preview->start()) {
        m_log->appendMessage(tr("预览已启动（场景 + 运行，共享场景）"));

        // 场景树绑定共享场景（自动选中第一项 → 检查器 + Gizmo）
        m_sceneTree->setScene(m_preview->getScene());
        m_sceneViewport->setEditScene(m_preview->getScene()); // 编辑器交互（平移/缩放/Gizmo）
        // 场景同步基准：初始绑定视为已同步（首次 tick 不无谓重建）
if (Shit::Scene *sc = m_preview->getScene(); sc) {
	            m_lastScene = sc;
	            m_lastSceneGeneration = sc->getGeneration();
	            m_inspector->setScene(sc);
	        }
        setPlaying(m_playAction->isChecked());   // 默认停止态：暂停预览逻辑

        // P14：启动时自动恢复上次打开的项目（静默；项目损坏/被删则忽略，不影响启动）
        const QString lastProject = m_settings.value("lastProjectDir").toString();
        if (!lastProject.isEmpty() && !openProjectPath(lastProject, /*silent=*/true)) {
            // P33：恢复失败不再静默——延迟弹窗说明（否则用户看到"就绪"误以为项目已打开）
            QTimer::singleShot(0, this, [this] {
                QMessageBox::information(this, tr("打开上次项目"),
                    tr("上次打开的项目未能加载（可能已被移动或删除）。\n"
                       "可通过「文件 → 打开项目…」重新选择。"));
            });
        }
    } else {
        m_log->appendMessage(tr("预览启动失败"), Qt::red);
    }

    m_savedSnapshot = snapshot();   // 启动初始场景作为存档基准（撤销/重做的 * 对比）
    updateUndoActions();
    updateWindowTitle();   // 初始标题（未命名场景）
    updateProjectMenus();  // P14：按是否有项目初始化菜单可用性
    statusBar()->showMessage(tr("就绪"));

    // P14：脚本工程编译管线（buildFinished → 热重载）
    m_scriptBuilder = new ScriptBuilder(this);
    connect(m_scriptBuilder, &ScriptBuilder::buildOutput, this,
            [this](const QString &line) { m_log->appendMessage(line); });
    connect(m_scriptBuilder, &ScriptBuilder::buildFailed, this,
            [this](const QString &reason) {
                // P33：构建失败给用户明确原因（此前只 3 秒状态栏，原因被丢弃）
                statusBar()->showMessage(tr("构建失败：%1").arg(reason), 8000);
                QMessageBox::warning(this, tr("构建失败"),
                    tr("脚本构建失败：\n%1\n\n详细输出见底部日志面板。").arg(reason));
            });
    connect(m_scriptBuilder, &ScriptBuilder::buildFinished, this, &MainWindow::onBuildFinished);
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

    // P21：场景视口 + 运行视口改为中央标签页叠放（Unity Scene/Game 同款）——
    // 不再做独立 Dock，始终占据窗口中央；四周 Dock 承载场景树 / 检查器 / 资源 / 日志。
    m_sceneViewport = new Viewport(this);
    m_gameViewport = new Viewport(this);
    m_gameViewport->setGizmoBarVisible(false);   // 运行视口不显示 Gizmo 工具条（运行态无编辑）
    auto *viewTabs = new QTabWidget(this);
    viewTabs->setObjectName("viewTabs");
    viewTabs->setDocumentMode(true);   // 扁平标签条（贴近 Unity 观感）
    viewTabs->addTab(m_sceneViewport, tr("场景视口"));
    viewTabs->addTab(m_gameViewport, tr("运行视口"));
    setCentralWidget(viewTabs);

    // 左侧：场景树
    auto *sceneDock = new QDockWidget(tr("场景"), this);
    sceneDock->setObjectName("sceneDock");
    m_sceneTree = new SceneTree(sceneDock);
    sceneDock->setWidget(m_sceneTree);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);
    m_docks.push_back(sceneDock);

    // 右侧：属性检查器（双视口移入中央后，右侧只留检查器）
    auto *inspectorDock = new QDockWidget(tr("属性"), this);
    inspectorDock->setObjectName("inspectorDock");
    inspectorDock->setMinimumWidth(260);
    m_inspector = new Inspector(inspectorDock);
    inspectorDock->setWidget(m_inspector);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);
    m_docks.push_back(inspectorDock);

    // P28：Animator 状态机窗口（Unity 风格可视化图；与检查器在右侧标签叠放）
    auto *animatorDockW = new QDockWidget(tr("Animator"), this);
    animatorDockW->setObjectName("animatorDock");
    m_animatorDock = new AnimatorDock(animatorDockW);
    animatorDockW->setWidget(m_animatorDock);
    addDockWidget(Qt::RightDockWidgetArea, animatorDockW);
    m_docks.push_back(animatorDockW);
    tabifyDockWidget(inspectorDock, animatorDockW);
    inspectorDock->raise();   // 默认显示检查器，选中 Animator 对象后用户切到 Animator 页

    // P29：Animation 帧动画窗口（Unity 风格 .anim 资产编辑器；与检查器在右侧标签叠放）
    auto *animationDockW = new QDockWidget(tr("Animation"), this);
    animationDockW->setObjectName("animationDock");
    m_animationDock = new AnimationDock(animationDockW);
    animationDockW->setWidget(m_animationDock);
    addDockWidget(Qt::RightDockWidgetArea, animationDockW);
    m_docks.push_back(animationDockW);
    tabifyDockWidget(inspectorDock, animationDockW);

    // 底部：资源面板 + 日志，标签页叠放（同区域 Tab 合并；拖标题栏可拆回独立）
    auto *assetsDock = new QDockWidget(tr("资源"), this);
    assetsDock->setObjectName("assetsDock");
    m_assets = new AssetsDock(assetsDock);
    assetsDock->setWidget(m_assets);
    addDockWidget(Qt::BottomDockWidgetArea, assetsDock);
    m_docks.push_back(assetsDock);

    auto *logDock = new QDockWidget(tr("日志"), this);
    logDock->setObjectName("logDock");
    m_log = new LogWidget(logDock);
    logDock->setWidget(m_log);
    addDockWidget(Qt::BottomDockWidgetArea, logDock);
    m_docks.push_back(logDock);

    // P27 增强：瓦片选择面板（底部标签组第三个页，选中 Tilemap 时显示瓦片网格）
    auto *tilesetDock = new QDockWidget(tr("瓦片"), this);
    tilesetDock->setObjectName("tilesetDock");
    m_tileset = new TilesetDock(tilesetDock);
    tilesetDock->setWidget(m_tileset);
    addDockWidget(Qt::BottomDockWidgetArea, tilesetDock);
    m_docks.push_back(tilesetDock);

    // 叠在一起：资源 + 日志 + 瓦片合并为底部标签组（默认显示资源页）
    tabifyDockWidget(assetsDock, logDock);
    tabifyDockWidget(assetsDock, tilesetDock);
    assetsDock->raise();

    // P38：精灵表视图 Dock（默认隐藏；资源面板双击 .sprite 时显示）
    auto *spriteSheetDockW = new QDockWidget(tr("精灵表"), this);
    spriteSheetDockW->setObjectName("spriteSheetDock");
    m_spriteSheetDock = new SpriteSheetDock(spriteSheetDockW);
    spriteSheetDockW->setWidget(m_spriteSheetDock);
    addDockWidget(Qt::RightDockWidgetArea, spriteSheetDockW);
    spriteSheetDockW->hide();   // 默认隐藏，按需从窗口菜单或双击 .sprite 显示
    m_docks.push_back(spriteSheetDockW);

    setDockNestingEnabled(true);
    resize(1280, 800);

    // P37：捕获出厂默认布局——「恢复默认布局」的兜底。
    // 此前默认布局从未入库：布局一旦被改动并在退出时自动保存（dockState），
    // 就永远回不到初始排列（resetDockLayout 只认手动保存的 dockStateDefault）。
    m_factoryLayout = saveState(kLayoutVersion);
}

void MainWindow::createMenus()
{
    auto *fileMenu = menuBar()->addMenu(tr("文件"));

    // P14：项目 —— 新建 / 打开 / 最近项目 / 关闭
    fileMenu->addAction(tr("新建项目…"), this, &MainWindow::newProject);
    fileMenu->addAction(tr("打开项目…"), this, &MainWindow::openProject);
    m_recentProjectsMenu = fileMenu->addMenu(tr("最近项目"));
    updateRecentProjectsMenu();
    m_closeProjectAction = fileMenu->addAction(tr("关闭项目"), this, &MainWindow::closeProject);
    fileMenu->addSeparator();

    m_newSceneAction = fileMenu->addAction(tr("新建场景"), this, &MainWindow::newScene);
    m_newSceneAction->setShortcut(QKeySequence::New);
    m_openSceneAction = fileMenu->addAction(tr("打开场景…"), this, &MainWindow::openScene);
    m_openSceneAction->setShortcut(QKeySequence::Open);

    // P8：最近场景（QSettings 持久化，最多 kMaxRecentScenes 条；项目态存项目 .shitengine）
    m_recentMenu = fileMenu->addMenu(tr("最近场景"));
    updateRecentMenu();

    fileMenu->addSeparator();
    m_saveSceneAction = fileMenu->addAction(tr("保存场景"), this, &MainWindow::saveScene);
    m_saveSceneAction->setShortcut(QKeySequence::Save);
    m_saveSceneAsAction = fileMenu->addAction(tr("场景另存为…"), this, &MainWindow::saveSceneAs);
    m_saveSceneAsAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
    fileMenu->addSeparator();
    m_projectSettingsAction = fileMenu->addAction(tr("项目设置…"), this, &MainWindow::onProjectSettings);
    m_exportGameAction = fileMenu->addAction(tr("导出游戏…"), this, &MainWindow::onExportGame);
    m_exportGameAction->setToolTip(tr("把当前项目导出为可独立运行的游戏目录（绿色免安装）"));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), this, &QWidget::close);

    // P9：撤销/重做（快照型命令栈）
    auto *editMenu = menuBar()->addMenu(tr("编辑"));
    m_undoAction = editMenu->addAction(tr("撤销"), this, &MainWindow::undo);
    m_undoAction->setShortcut(QKeySequence::Undo);   // Ctrl+Z
    m_redoAction = editMenu->addAction(tr("重做"), this, &MainWindow::redo);
    m_redoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z));
    updateUndoActions();

    // 复制/粘贴对象（内部剪贴板，不进系统剪贴板）：文本控件获焦时自动让位（见 isTextEditFocused）
    editMenu->addSeparator();
    auto *copyAction = editMenu->addAction(tr("复制对象"), this, &MainWindow::copySelectedObject);
    copyAction->setShortcut(QKeySequence::Copy);   // Ctrl+C
    auto *pasteAction = editMenu->addAction(tr("粘贴对象"), this, &MainWindow::pasteObject);
    pasteAction->setShortcut(QKeySequence::Paste); // Ctrl+V

    // P16：打开代码编辑器（IDE 在项目设置 → 通用 → 代码编辑器 中选择）
    editMenu->addSeparator();
    m_openIdeAction = editMenu->addAction(tr("打开代码…"), this, &MainWindow::openIde);
    m_openIdeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
    m_openIdeAction->setToolTip(tr("用项目设置中配置的 IDE 打开项目根目录（Ctrl+Shift+O）"));

    // P13：视图菜单（布局恢复）
    auto *viewMenu = menuBar()->addMenu(tr("视图"));
    viewMenu->addAction(tr("恢复默认布局"), this, &MainWindow::resetDockLayout);
    viewMenu->addAction(tr("将当前布局设为默认"), this, &MainWindow::saveLayoutAsDefault);

    // 窗口菜单：列出全部 Dock 面板（勾选 = 可见）。面板右上角关闭后，
    // 可在此重新勾选打开；toggleViewAction 的勾选状态与面板可见性自动同步。
    auto *windowMenu = menuBar()->addMenu(tr("窗口"));
    for (QDockWidget *dock : m_docks)
        windowMenu->addAction(dock->toggleViewAction());

    auto *helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(tr("关于"), this, &MainWindow::about);
}

void MainWindow::newScene()
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return;

    if (isPlaying()) setPlaying(false);   // 运行中先停止（恢复快照）再操作

    if (!confirmDiscardChanges(tr("新建场景")))
        return;

    // 先收集再删：编辑器下 removeGameObject 当场 erase，不能在遍历中删（迭代器失效）
    std::vector<Shit::GameObject *> toRemove;
    for (auto &go : scene->getGameObjects()) {
        const std::string name = go->getName();
        if (name != "scene_camera")   // 保留编辑器相机（游戏相机不定名，随场景里的走）
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
    if (isPlaying()) setPlaying(false);   // 运行中先退出再切换场景
    if (!confirmDiscardChanges(tr("打开场景")))
        return;

    // P14：项目态默认从项目 Scenes/ 目录挑场景
    const QString initialDir = hasProject() ? m_project.scenesDir() : QString();
    const QString path = QFileDialog::getOpenFileName(this, tr("打开场景"), initialDir, tr("ShitEngine 场景 (*.scene)"));
    if (path.isEmpty()) return;
    openScenePath(path);
}

bool MainWindow::openScenePath(const QString &path)
{
    if (isPlaying()) setPlaying(false);   // 运行中先退出（恢复快照），再进入新场景

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
        ST_CORE_WARN("[openScenePath] 异常: {}（{}）", e.what(), path.toStdString());
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

void MainWindow::copySelectedObject()
{
    if (isTextEditFocused()) return;   // 文本编辑中：Ctrl+C 复制文字，不劫持
    Shit::GameObject *sel = m_sceneTree->selectedObject();
    if (!sel) {
        m_log->appendMessage(tr("未选中对象，无法复制"), Qt::yellow);
        return;
    }
    m_clipboard = Shit::Prefab::Capture(sel).toJson();
    m_log->appendMessage(tr("已复制「%1」").arg(QString::fromStdString(sel->getName())));
}

void MainWindow::pasteObject()
{
    if (isTextEditFocused()) return;   // 文本编辑中：Ctrl+V 粘贴文字，不劫持
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene || m_clipboard.is_null()) {
        m_log->appendMessage(tr("没有可粘贴的对象（剪贴板为空）"), Qt::yellow);
        return;
    }
    Shit::Prefab prefab = Shit::Prefab::FromJson(m_clipboard);
    if (!prefab.hasData()) {
        m_log->appendMessage(tr("剪贴板数据无效，无法粘贴"), Qt::red);
        return;
    }

    // 源对象可能已在播放中被游戏逻辑销毁：存活才沿用其名与父级
    Shit::GameObject *src = m_sceneTree->selectedObject();
    std::string base = "Pasted Object";
    Shit::GameObject *parent = nullptr;
    if (src && scene->containsGameObject(src)) {
        base = src->getName();
        parent = src->getParent();
    }

    undoBegin();   // before 快照（须在修改前）
    auto *dup = prefab.instantiate(scene, uniqueObjectName(scene, base).toStdString());
    if (!dup) return;
    if (parent) dup->setParent(parent);   // 粘贴为源对象兄弟（同父）
    undoCommit(tr("复制对象"));
    setDirty(true);
    // 刷新树并选中新对象（联动检查器/Gizmo；播放中 setScene 另由每帧同步重建，幂等）
    m_sceneTree->setScene(scene, false);
    m_sceneTree->selectObject(dup);
}

// ── P25c：Prefab 预置资产（存为预置 / 拖入或双击实例化）──

void MainWindow::onSaveObjectAsPrefab(Shit::GameObject *object)
{
    if (!object || !m_preview) return;
    if (isPlaying()) {
        m_log->appendMessage(tr("播放中不能存为预置"), Qt::yellow);
        return;
    }
    const nlohmann::json doc = Shit::SceneSerializer::toJson(object);
    const QString base = QString::fromStdString(object->getName()).replace(' ', '_');
    // 默认存到项目 Assets/ 目录（Unity 语义：预置属于资产）；无项目回退到资源窗口根或 exe 目录
    const QString assetsDir = hasProject() ? m_project.assetsDir()
        : (m_assets->projectDir().isEmpty() ? QCoreApplication::applicationDirPath() : m_assets->projectDir());
    const QString path = QFileDialog::getSaveFileName(this, tr("存为预置"),
        assetsDir + "/" + base + ".prefab", tr("ShitEngine 预置 (*.prefab)"));
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_log->appendMessage(tr("无法写入预置文件: %1").arg(path), Qt::red);
        return;
    }
    file.write(doc.dump(2).c_str());
    m_log->appendMessage(tr("已保存预置: %1（%2 个对象，含子树）")
        .arg(QFileInfo(path).fileName()).arg(doc["objects"].size()));
    // 资源面板 QFileSystemModel 自动监听目录，无需手动刷新
}

void MainWindow::onPrefabOpenRequested(const QString &path)
{
    instantiatePrefab(path, false, 0.0f, 0.0f);
}

void MainWindow::onAnimOpenRequested(const QString &path)
{
    // 1) 读取 .anim 文件（AnimationClip 的 JSON 对象）
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        m_log->appendMessage(tr("无法读取动画剪辑：%1").arg(path), Qt::red);
        return;
    }
    const QByteArray data = f.readAll();
    f.close();

    Shit::AnimationClip clip;
    try {
        nlohmann::json j = nlohmann::json::parse(data.constData());
        if (!clip.fromJson(j)) {
            m_log->appendMessage(tr("动画剪辑解析失败（非法 JSON 结构）：%1").arg(path), Qt::red);
            return;
        }
    } catch (const std::exception &e) {
        m_log->appendMessage(tr("动画剪辑解析失败：%1 —— %2").arg(path, QString::fromStdString(e.what())), Qt::red);
        return;
    }

    // 2) 应用到选中对象的 Animator（优先当前状态，否则第一个状态）
    Shit::GameObject *sel = m_sceneTree->selectedObject();
    Shit::Animator *animator = sel ? sel->getComponent<Shit::Animator>() : nullptr;
    if (!animator) {
        m_log->appendMessage(tr("未选中含 Animator 组件的对象，无法应用剪辑"), Qt::yellow);
        return;
    }
    int idx = animator->currentStateIndex();
    if (idx < 0) idx = 0;
    if (idx >= animator->stateCount()) {
        m_log->appendMessage(tr("Animator 无状态可应用剪辑"), Qt::yellow);
        return;
    }
    undoBegin();   // 在修改前捕获快照（可撤销）
    Shit::AnimatorState state = *animator->stateAt(idx);
    state.clip = clip;
    state.assetPath = path.toStdString();   // 绑定资产引用（AnimatorDock 只认 .anim 资产）
    if (animator->setState(idx, state)) {
        m_log->appendMessage(tr("已应用动画剪辑「%1」到状态「%2」")
                                 .arg(QString::fromStdString(clip.name),
                                      QString::fromStdString(state.name)));
        undoCommit(tr("应用动画剪辑"));
        m_inspector->setGameObject(sel);  // 重绑检查器，立即反映新剪辑
    }
}

void MainWindow::reloadAnimatorAsset(const QString &path)
{
    // 方案 A：Animation 窗口保存 .anim → 把新剪辑同步回所有引用该资产的 Animator 状态。
    // 遍历选中对象 Animator 的状态，assetPath（相对项目根解析）匹配的文件重新读入并 setState。
    Shit::GameObject *sel = m_sceneTree->selectedObject();
    Shit::Animator *animator = sel ? sel->getComponent<Shit::Animator>() : nullptr;
    if (!animator) return;

    // 保存路径可能是相对（project config 或相对资产）——统一转绝对比对
    const QString savedAbs = QFileInfo(path).absoluteFilePath();
    const QString root = hasProject() ? m_project.rootDir() : QString();

    bool changedAny = false;
    for (int i = 0; i < animator->stateCount(); ++i) {
        const Shit::AnimatorState *st = animator->stateAt(i);
        if (!st || st->assetPath.empty()) continue;
        QString assetRel = QString::fromStdString(st->assetPath);
        QString assetAbs = QFileInfo(assetRel).isAbsolute()
            ? QFileInfo(assetRel).absoluteFilePath()
            : (root.isEmpty() ? assetRel : QFileInfo(root + "/" + assetRel).absoluteFilePath());
        if (assetAbs.compare(savedAbs, Qt::CaseInsensitive) != 0) continue;

        // 重新读 .anim 覆盖 clip
        Shit::AnimationClip clip;
        QFile f(savedAbs);
        if (!f.open(QIODevice::ReadOnly)) {
            // P33：资产文件读不到/解析失败不再静默跳过（此前用户以为已同步，动画实际没更新）
            QMessageBox::warning(this, tr("动画资产同步"),
                tr("无法读取动画资产：\n%1\n\n文件可能已被移动或删除。状态的剪辑将保持旧版本。").arg(savedAbs));
            break;
        }
        const QByteArray data = f.readAll();
        f.close();
        bool ok = false;
        try {
            nlohmann::json j = nlohmann::json::parse(data.constData());
            ok = clip.fromJson(j);
        } catch (const std::exception &) { ok = false; }
        if (!ok) {
            QMessageBox::warning(this, tr("动画资产同步"),
                tr("动画资产解析失败：\n%1\n\n文件可能已损坏。状态的剪辑将保持旧版本。").arg(savedAbs));
            break;
        }

        Shit::AnimatorState ns = *st;
        ns.clip = clip;
        if (animator->setState(i, ns)) changedAny = true;
    }
    if (changedAny) {
        m_animatorDock->refresh();
        if (sel) m_inspector->setGameObject(sel);  // 重绑检查器，反映新剪辑
    }
}

void MainWindow::openAnimatorEditor()
{
    // AnimatorDock 的父级即其所属 QDockWidget（createDocks 中 new AnimatorDock(animatorDockW)）
    auto *dockWidget = qobject_cast<QDockWidget *>(m_animatorDock->parentWidget());
    if (!dockWidget) return;

    // QDockWidget 被关闭（✕）后 Qt 会把它从 dock 区域摘除；再次 show() 只会回到浮动窗，
    // 不会自动吸附。这里先检查它是否已不在 dock 区域，若是则重新加回右侧并与检查器叠放（tab）。
    const bool attached = !dockWidget->isFloating() && dockWidget->isVisible();
    if (!attached) {
        QDockWidget *inspectorDock = nullptr;
        for (QDockWidget *d : m_docks) {
            if (d->objectName() == QStringLiteral("inspectorDock")) { inspectorDock = d; break; }
        }
        if (inspectorDock) {
            addDockWidget(Qt::RightDockWidgetArea, dockWidget);
            tabifyDockWidget(inspectorDock, dockWidget);
        }
    }

    dockWidget->show();
    dockWidget->raise();
    m_animatorDock->setFocus();
}

void MainWindow::openAnimationEditor()
{
    auto *dockWidget = qobject_cast<QDockWidget *>(m_animationDock->parentWidget());
    if (!dockWidget) return;
    // 若已从 dock 区域脱离（关闭/浮窗）→ 重新吸附回右侧并与检查器叠放（同 openAnimatorEditor）
    const bool attached = !dockWidget->isFloating() && dockWidget->isVisible();
    if (!attached) {
        QDockWidget *inspectorDock = nullptr;
        for (QDockWidget *d : m_docks) {
            if (d->objectName() == QStringLiteral("inspectorDock")) { inspectorDock = d; break; }
        }
        if (inspectorDock) {
            addDockWidget(Qt::RightDockWidgetArea, dockWidget);
            tabifyDockWidget(inspectorDock, dockWidget);
        }
    }
    dockWidget->show();
    dockWidget->raise();
    m_animationDock->setFocus();
}

void MainWindow::onAnimationOpenRequested(const QString &path)
{
    if (m_animationDock->openFile(path)) {
        m_log->appendMessage(tr("已在 Animation 窗口打开：%1").arg(path));
    } else {
        m_log->appendMessage(tr("打开动画剪辑失败：%1").arg(path), Qt::red);
        return;
    }
    openAnimationEditor();
}

void MainWindow::onPrefabDropped(const QString &path, float logicalX, float logicalY)
{
    instantiatePrefab(path, true, logicalX, logicalY);
}

void MainWindow::instantiatePrefab(const QString &path, bool useDropPos, float logicalX, float logicalY)
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_log->appendMessage(tr("无法读取预置文件: %1").arg(path), Qt::red);
        return;
    }
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(file.readAll().toStdString());
    } catch (const std::exception &e) {
        m_log->appendMessage(tr("预置文件解析失败: %1").arg(e.what()), Qt::red);
        return;
    }
    if (!doc.contains("objects") || !doc["objects"].is_array() || doc["objects"].empty()) {
        m_log->appendMessage(tr("预置文件为空或格式无效: %1").arg(QFileInfo(path).fileName()), Qt::red);
        return;
    }

    // 根对象重名去重（与复制粘贴同款 " (1)" 后缀；子树内部名字保留）
    if (doc["objects"][0].contains("name") && doc["objects"][0]["name"].is_string())
        doc["objects"][0]["name"] = uniqueObjectName(scene,
            doc["objects"][0]["name"].get<std::string>()).toStdString();

    // 记录现有对象 → fromJson 后找出新增根对象（用于落点定位与选中）
    std::unordered_set<Shit::GameObject *> before;
    for (auto &go : scene->getGameObjects()) before.insert(go.get());

    undoBegin();
    Shit::SceneSerializer::fromJson(doc, scene);
    Shit::GameObject *created = nullptr;
    for (auto &go : scene->getGameObjects())
        if (!before.count(go.get())) { created = go.get(); break; }
    if (created && useDropPos) {
        // 落点逻辑像素 → 世界坐标（与拾取/拖图同源）
        Shit::Vector2 world{ 0.0f, 0.0f };
        for (auto &go : scene->getGameObjects())
            if (auto *cam = go->getComponent<Shit::CameraComponent>()) {
                world = cam->screenToWorld({ logicalX, logicalY });
                break;
            }
        if (auto *t = created->getComponent<Shit::TransformComponent>())
            t->setPosition(world);
    }
    undoCommit(tr("实例化预置 %1").arg(QFileInfo(path).baseName()));
    setDirty(true);
    m_sceneTree->setScene(scene, false);
    if (created) m_sceneTree->selectObject(created);
    m_log->appendMessage(tr("已实例化预置: %1").arg(QFileInfo(path).fileName()));
}

void MainWindow::updateUndoActions()
{
    if (!m_undoAction || !m_redoAction) return;
    const bool running = isPlaying();
    const bool enabled = hasProject() && !running;
    m_undoAction->setEnabled(enabled && m_undo.canUndo());
    m_redoAction->setEnabled(enabled && m_undo.canRedo());
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

void MainWindow::onSceneFrameReady(const QImage &frame)
{
    m_sceneViewport->setFrame(frame);
    // 先同步场景树/选中态（可能重建检查器绑定），再每帧回读引擎值：
    // 若播放中游戏逻辑销毁了选中对象，syncSceneSelection 已清空检查器，
    // refresh() 不会触碰已释放的组件内存
    syncSceneSelection();
    m_inspector->refresh();
    m_animatorDock->refresh();
}

void MainWindow::syncSceneSelection()
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) {
        m_inspector->setGameObject(nullptr);
        m_sceneViewport->setSelectedObject(nullptr);
        m_lastScene = nullptr;
        m_lastSceneGeneration = 0;
        return;
    }

    // 场景未变（同一场景 + 代数未增）→ 选中态依然有效，无需处理。
    // 代数在任何对象增删/组件增删/改名/改父/销毁时递增（场景整体替换则指针不同）
    if (scene == m_lastScene && scene->getGeneration() == m_lastSceneGeneration)
        return;
    m_lastScene = scene;
    m_lastSceneGeneration = scene->getGeneration();
    m_inspector->setScene(scene);  // 场景变化时通知检查器刷新系统面板

    // 场景内容已变（含整体替换）：视口也用当前场景，避免持有已销毁场景的 m_editScene
    m_sceneViewport->setEditScene(scene);
    m_sceneViewport->setSelectedObject(nullptr);   // 重建前先清（旧选中可能已失效）

    // 场景内容已变：重建场景树。选中对象若仍存活则恢复选中（并重绑检查器——
    // 组件可能被增删，旧读回指针可能已失效）；否则视为删除，清空检查器/Gizmo。
    Shit::GameObject *sel = m_sceneTree->selectedObject();   // 仅地址比较，可能已失效
    const bool keep = sel && scene->containsGameObject(sel);

    m_sceneTree->setScene(scene, /*autoSelect=*/false);
    if (keep) {
        m_sceneTree->selectObject(sel);            // 触发 objectSelected → 检查器/Gizmo 重新绑定
    } else {
        m_inspector->setGameObject(nullptr);       // 清空检查器（防回读已释放组件）
        m_sceneViewport->setSelectedObject(nullptr);
        m_tileset->setGameObject(nullptr);         // 清空瓦片面板
        m_animatorDock->setGameObject(nullptr);    // 清空状态机窗口
    }
}

bool MainWindow::saveScene()
{
    if (m_scenePath.isEmpty())
        return saveSceneAs();
    return saveSceneTo(m_scenePath);
}

bool MainWindow::saveSceneAs()
{
    const QString initialDir = m_scenePath.isEmpty()
        ? (hasProject() ? m_project.scenesDir() : QString())
        : QFileInfo(m_scenePath).absolutePath();
    const QString path = QFileDialog::getSaveFileName(this, tr("保存场景为"), initialDir, tr("ShitEngine 场景 (*.scene)"));
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
    // P14：项目态显示项目名
    if (hasProject())
        title += QStringLiteral(" - %1").arg(m_project.name());
    title += QStringLiteral(" - ") + tr("ShitEngine 编辑器");
    setWindowTitle(title);
}

QStringList MainWindow::recentScenes() const
{
    QStringList list = stateSettings()->value("recentScenes").toStringList();
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
    stateSettings()->setValue("recentScenes", list);
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
    // 优先恢复用户手动「将当前布局设为默认」的自定义默认；
    // 没存过则回落到出厂默认布局（createDocks 的初始排列）。
    const QByteArray userDef = stateSettings()->value("dockStateDefault").toByteArray();
    if (!userDef.isEmpty() && restoreState(userDef, kLayoutVersion)) {
        statusBar()->showMessage(tr("已恢复自定义默认布局"), 2000);
        return;
    }
    if (!m_factoryLayout.isEmpty() && restoreState(m_factoryLayout, kLayoutVersion)) {
        statusBar()->showMessage(tr("已恢复出厂布局"), 2000);
        return;
    }
    statusBar()->showMessage(tr("当前布局即为默认"), 2000);
}

/// P17：把当前 Dock 布局存为默认（写入项目 .shitengine/state.ini 或全局注册表的
/// dockStateDefault；之后「恢复默认布局」即回到当前排列）。
void MainWindow::saveLayoutAsDefault()
{
    QSettings *s = stateSettings();
    s->setValue("dockStateDefault", saveState(kLayoutVersion));
    s->sync();
    statusBar()->showMessage(tr("已把当前布局设为默认布局"), 2000);
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("关于 ShitEngine 编辑器"),
        tr("<b>ShitEngine 编辑器</b>　v0.1（Qt 6 Widgets + ShitEngine 引擎）<br><br>"
           "编辑器把引擎<u>进程内嵌</u>，场景唯一事实来源为 .scene 文件，"
           "编辑 / 运行 / 切关共用同一加载器。<br><br>"
           "<b>快捷键</b><br>"
           "· Ctrl+N / Ctrl+O / Ctrl+S / Ctrl+Shift+S — 新建 / 打开 / 保存 / 另存为<br>"
           "· Ctrl+B — 构建脚本（编译 C++ 脚本工程并热重载）<br>"
           "· Ctrl+Z / Ctrl+Shift+Z — 撤销 / 重做<br>"
           "· Q / W / E — Gizmo 移动 / 旋转 / 缩放（Ctrl 吸附）<br>"
           "· F2 — 重命名对象；Del — 删除对象<br>"
           "· 滚轮缩放视图 / 中键拖拽平移相机<br><br>"
           "<b>运行态（Unity 风格）</b><br>"
           "· ▶ 运行：先编译脚本（若存在）并热加载，再播放逻辑；运行中可改属性<br>"
           "· ⏸ 暂停 / ⏯ 继续：冻结 / 恢复逻辑，画面保持（不恢复快照）<br>"
           "· ■ 停止：恢复运行前的场景与属性（运行中的改动全部回退）<br><br>"
           "Dock 布局自动记忆；「视图 → 恢复默认布局」可重置。"));
}

void MainWindow::createToolbar()
{
    auto *toolbar = addToolBar(tr("控制"));
    toolbar->setMovable(false);

    // ▶ 运行 / ■ 停止（Unity 式）：进入运行态先编译脚本并热加载，停止恢复运行前快照。
    // 运行中可编辑属性/场景；「⏸ 暂停」冻结逻辑但画面保持。
    m_playAction = toolbar->addAction(tr("▶ 运行"));
    m_playAction->setCheckable(true);
    m_playAction->setChecked(false);
    connect(m_playAction, &QAction::toggled, this, &MainWindow::setPlaying);

    // ⏸ 暂停 / 继续（仅运行态可用；与「停止」不同——暂停不恢复快照）
    m_pauseAction = toolbar->addAction(tr("⏸ 暂停"));
    m_pauseAction->setCheckable(true);
    m_pauseAction->setChecked(false);
    m_pauseAction->setEnabled(false);
    connect(m_pauseAction, &QAction::toggled, this, &MainWindow::onPauseToggled);

    // P11/P14：Gizmo 三模式工具条已移到场景视口内（左上角，Unity 风格）；
    // 这里保留窗口级 Q/W/E 快捷键（不可见于任何菜单/工具栏）。
    auto addGizmoShortcut = [this](Qt::Key key, Viewport::GizmoMode mode) {
        auto *act = new QAction(this);
        act->setShortcut(QKeySequence(key));
        connect(act, &QAction::triggered, this, [this, mode]() {
            if (m_sceneViewport) m_sceneViewport->setGizmoMode(mode);
        });
        addAction(act);   // 窗口级 action：快捷键生效、UI 不可见
        m_gizmoShortcutActions.push_back(act);
    };
    addGizmoShortcut(Qt::Key_Q, Viewport::GizmoMode::Move);
    addGizmoShortcut(Qt::Key_W, Viewport::GizmoMode::Rotate);
    addGizmoShortcut(Qt::Key_E, Viewport::GizmoMode::Scale);

    // P13：工具栏增补 —— 撤销 / 重做（与菜单共享同一 QAction）
    toolbar->addSeparator();
    toolbar->addAction(m_undoAction);
    toolbar->addAction(m_redoAction);

    // P14：构建脚本（Ctrl+B）→ 编译 C++ 脚本工程并热重载
    toolbar->addSeparator();
    m_buildAction = toolbar->addAction(tr("构建脚本"));
    m_buildAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    connect(m_buildAction, &QAction::triggered, this, &MainWindow::onBuildScripts);
}

void MainWindow::setPlaying(bool playing)
{
    if (playing) {
        // ── 进入运行态 ──
        // 1) 记录运行前快照（Unity 语义：停止时恢复，运行中的对象/属性改动全部回退）
        if (!m_hasRunSnapshot) {
            m_runSnapshot = snapshot();
            m_hasRunSnapshot = true;
        }
        // 2) 有 C++ 脚本工程 + SDK → 先编译（反射扫描）并热加载 DLL，构建成功后才进入运行；
        //    无脚本/SDK 直接运行（Unity：无脚本也能 Play）
        const bool withBuild = hasProject() && m_project.hasScripts()
                               && !m_project.sdkDir().trimmed().isEmpty() && m_scriptBuilder;
        if (withBuild) {
            m_playPendingBuild = true;
            onBuildScripts();
            if (m_playPendingBuild) return;   // 构建已启动：成功 → onBuildFinished → enterPlayMode
            // onBuildScripts 前置失败（已弹框提示）→ 取消本次运行请求，按钮复位
            if (m_playAction) m_playAction->setChecked(false);
            return;
        }
        enterPlayMode();
    } else {
        // ── 停止运行（恢复运行前快照）──
        exitPlayMode();
    }
}

void MainWindow::enterPlayMode()
{
    if (m_playAction) m_playAction->setText(tr("■ 停止"));
    if (m_pauseAction) {
        m_pauseAction->setEnabled(true);
        m_pauseAction->setChecked(false);   // 默认运行（未暂停）
    }
    // P14：运行态自动抓取键盘（无需先点击运行视口），并释放 Gizmo 快捷键
    // （Q/W/E 是窗口级 QShortcut，会抢在游戏输入前消费按键——W 既是前进键
    // 又是旋转 Gizmo 快捷键，运行中必须交给游戏）。工具栏按钮随之禁用，
    // 运行中切工具请用鼠标点击（停止后自动恢复）。
    m_gameViewport->grabKeyboard();
    for (QAction *a : m_gizmoShortcutActions) a->setEnabled(false);   // 运行态释放 Q/W/E 给游戏
    if (m_preview) {
        // Unity 式「每次运行从头开始」：时钟归零 + Behavior 重新 onStart + 清键鼠残留
        if (m_preview->context()) {
            Shit::EngineContext::setCurrent(m_preview->context());
            applyInputMappingsToEngine();   // P15：播放前刷新输入映射（改键无需重开）
            Shit::Time::ResetTotalTime();
            Shit::Input::ResetState();
            if (auto *scene = Shit::SceneManager::GetCurrentScene())
                if (auto *bs = scene->getSystem<Shit::BehaviorSystem>())
                    bs->resetAllBehaviors();
        }
        m_preview->setPlaying(true);
    }
    // P25d：播放态编辑锁——检查器只读（运行时值仍实时刷新）、视口 Gizmo/碰撞体手柄禁用
    m_inspector->setPlayMode(true);
    m_sceneViewport->setEditEnabled(false);
    updateUndoActions();
    statusBar()->showMessage(tr("运行中"));
}

void MainWindow::exitPlayMode()
{
    if (m_playAction) m_playAction->setText(tr("▶ 运行"));
    if (m_pauseAction) {
        m_pauseAction->setChecked(false);
        m_pauseAction->setEnabled(false);
    }
    m_gameViewport->releaseKeyboard();   // 释放键盘捕获（编辑器快捷键恢复）
    for (QAction *a : m_gizmoShortcutActions) a->setEnabled(true);   // 恢复 Gizmo 快捷键
    if (m_preview) m_preview->setPlaying(false);   // 引擎逻辑冻结
    // 停止时清键鼠残留/注入坐标：进入时已有 ResetState，退出也要清，否则编辑态
    // Input::GetMousePosition 一直返回播放期最后一次注入值（陈旧坐标）
    Shit::Input::ResetState();
    // 恢复运行前快照：运行期间的属性/对象改动全部回退（保留编辑器相机）
    if (m_hasRunSnapshot && !m_runSnapshot.empty()) {
        applySnapshot(m_runSnapshot);
        // 播放态快照恢复后同步场景指针（系统列表可能已变）
        if (auto *sc = m_preview ? m_preview->getScene() : nullptr)
            m_inspector->setScene(sc);
    }
    m_hasRunSnapshot = false;
    m_playPendingBuild = false;
    // P25d：解锁编辑（applySnapshot 重建检查器后调用，控件默认启用态）
    m_inspector->setPlayMode(false);
    m_sceneViewport->setEditEnabled(true);
    updateUndoActions();
    statusBar()->showMessage(tr("已停止（恢复运行前状态）"), 2500);
}

void MainWindow::onPauseToggled(bool paused)
{
    if (m_preview) m_preview->setPlaying(!paused);
    statusBar()->showMessage(m_playAction && m_playAction->isChecked()
        ? (paused ? tr("已暂停（⏸）") : tr("运行中"))
        : tr("未在运行"), 2000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (isPlaying()) setPlaying(false);   // 退出前先结束运行（恢复运行前快照）
    if (confirmDiscardChanges(tr("退出编辑器"))) {
        // P13/P14：退出前记忆 Dock 布局与窗口几何（存当前项目 .shitengine/state.ini 或全局）
        saveProjectState();
        event->accept();
    } else {
        event->ignore();
    }
}

// ═════════════════════════ P14 项目系统 ═════════════════════════

void MainWindow::newProject()
{
    if (!confirmDiscardChanges(tr("新建项目")))
        return;

    ProjectWizard wizard(this);
    if (wizard.exec() != QDialog::Accepted)
        return;

    const QString root = wizard.rootDir();
    if (root.isEmpty()) return;
    if (QFileInfo::exists(root)) {
        m_log->appendMessage(tr("新建项目失败：目录已存在，不会覆盖 — %1").arg(root), Qt::red);
        QMessageBox::warning(this, tr("新建项目"), tr("所选项目目录已存在：\n%1\n\n为防止误覆盖，请更换项目名或位置。").arg(root));
        return;
    }

    Project project;
    if (!project.create(root, wizard.projectName(), wizard.sdkDir(), wizard.withScripts())) {
        m_log->appendMessage(tr("新建项目失败：%1").arg(project.error()), Qt::red);
        QMessageBox::warning(this, tr("新建项目"), project.error());
        return;
    }

    // 全局记忆：SDK 目录 + 父目录（供下次向导预填）
    m_settings.setValue("lastSdkDir", wizard.sdkDir());
    m_settings.setValue("lastProjectParentDir", QFileInfo(root).absolutePath());

    enterProject(project);
    m_log->appendMessage(tr("项目已创建：%1").arg(project.name()));
}

void MainWindow::openProject()
{
    const QString dir = ProjectWizard::pickProjectRoot(this);
    if (!dir.isEmpty())
        openProjectPath(dir);
}

void MainWindow::closeProject()
{
    if (!hasProject()) return;
    if (isPlaying()) setPlaying(false);   // 先退出运行（恢复快照）
    if (!confirmDiscardChanges(tr("关闭项目")))
        return;
    closeProjectInternal();
}

bool MainWindow::openProjectPath(const QString &path, bool silent)
{
    if (isPlaying()) setPlaying(false);   // 运行中先退出再做项目切换
    if (!silent && !confirmDiscardChanges(tr("打开项目")))
        return false;

    Project project;
    if (!project.open(path)) {
        m_log->appendMessage(project.error(), Qt::red);
        if (!silent)
            QMessageBox::warning(this, tr("打开项目"), project.error());
        return false;
    }

    enterProject(project);
    return true;
}

void MainWindow::enterProject(const Project &project)
{
    // 1) 保存旧状态（布局/几何 → 当前 settings 目标：旧项目 state.ini 或全局）
    saveProjectState();

    // 2) 切换项目：状态持久化迁到项目 .shitengine/state.ini（IniFormat）
    delete m_projectSettings;
    m_projectSettings = nullptr;
    m_project = project;
    m_projectSettings = new QSettings(project.stateFilePath(), QSettings::IniFormat);
    m_projectSettings->setFallbacksEnabled(false);   // 项目态独立于注册表

    // P32：打开项目后引擎文件日志写入项目 .shitengine/log（不 chdir，
    // 避免影响相对路径资源加载；关闭项目时恢复默认目录）
    Shit::Log::SetLogDirectory((project.rootDir() + "/.shitengine/log").toStdString());
    m_log->setDefaultDir(project.rootDir() + "/.shitengine/log");   // 日志面板保存默认目录

    // 3) 恢复项目布局（窗口大小不恢复——P37b：一律最大化启动）
    {
        const QByteArray layout = m_projectSettings->value("dockState").toByteArray();
        if (!layout.isEmpty()) restoreState(layout, kLayoutVersion);
    }

    // 4) 资源窗口绑定整个项目根（Unity 式：树里可见 Assets/ Scenes/ Scripts/）；
    //    最近场景切到项目级列表；Animator 相对路径基准设为项目根
    m_assets->applyProjectDir(project.rootDir());
    m_animatorDock->setProjectRoot(project.rootDir());
    m_spriteSheetDock->setProjectRoot(project.rootDir());
    updateRecentMenu();

    // 5) 插件 + 场景：卸载旧插件（项目脚本库）→ 加载本项目插件 → 载入项目场景
    if (m_preview && m_preview->isRunning()) {
        m_preview->loadProjectConfig(project.configPath());
        applyInputMappingsToEngine();   // 项目输入映射 → 引擎（播放直接用项目按键设置）

        const QString scene = project.scenePath();
        if (!scene.isEmpty() && QFileInfo::exists(scene)) {
            if (!openScenePath(scene))
                m_log->appendMessage(tr("项目场景损坏或无法载入：%1").arg(scene), Qt::red);
        } else {
            // 首次进入：清成双相机空场景并落盘为 Main.scene（引擎序列化保证格式正确）
            resetToEmptyScene();
            const QString freshScene = project.scenesDir() + "/Main.scene";
            if (saveSceneTo(freshScene)) {
                m_project.setScenePath(m_scenePath);
                m_project.saveConfig();
            }
        }
    }

    // 6) 全局记忆 + 最近项目（跨项目共享）
    m_settings.setValue("lastProjectDir", project.rootDir());
    addRecentProject(project.rootDir());
    m_settings.sync();

    // 7) 标题 / 菜单可用性
    updateProjectMenus();
    updateWindowTitle();
    statusBar()->showMessage(tr("已打开项目：%1").arg(project.name()), 3000);
}

void MainWindow::closeProjectInternal()
{
    if (!hasProject()) return;
    saveProjectState();   // 布局/几何 → 项目 .shitengine/state.ini

    // 卸载项目插件前先清场景（组件析构调用 DLL 内代码，必须在 FreeLibrary 前完成）
    if (m_preview) m_preview->unloadPlugins();
    resetToEmptyScene();

    delete m_projectSettings;
    m_projectSettings = nullptr;
    m_project = Project();

    Shit::Log::SetLogDirectory(std::string());   // 无项目：文件日志回落到默认（CWD/.shitengine/log）
    m_log->setDefaultDir(QString());

    m_assets->applyProjectDir(m_settings.value("projectDir").toString());
    m_animatorDock->setProjectRoot(QString());   // 无项目：资产路径存绝对
    m_spriteSheetDock->setProjectRoot(QString());
    updateRecentMenu();          // 最近场景回退到全局列表
    updateProjectMenus();
    updateWindowTitle();
    statusBar()->showMessage(tr("项目已关闭"), 3000);
}

QSettings *MainWindow::stateSettings() const
{
    return m_projectSettings ? m_projectSettings : const_cast<QSettings *>(&m_settings);
}

void MainWindow::saveProjectState()
{
    stateSettings()->setValue("dockState", saveState(kLayoutVersion));
    // P37b：不保存窗口大小/位置——启动一律最大化
    stateSettings()->sync();
}

QStringList MainWindow::recentProjects() const
{
    QStringList list = m_settings.value("recentProjects").toStringList();
    // 剔除已删除的目录
    list.erase(std::remove_if(list.begin(), list.end(),
        [](const QString &p) { return !QFileInfo::exists(p + "/config.json"); }), list.end());
    return list;
}

void MainWindow::addRecentProject(const QString &path)
{
    QStringList list = recentProjects();
    list.removeAll(path);
    list.prepend(path);
    while (list.size() > kMaxRecentProjects)
        list.removeLast();
    m_settings.setValue("recentProjects", list);
    updateRecentProjectsMenu();
}

void MainWindow::updateRecentProjectsMenu()
{
    if (!m_recentProjectsMenu) return;
    m_recentProjectsMenu->clear();

    const QStringList recents = recentProjects();
    if (recents.isEmpty()) {
        m_recentProjectsMenu->addAction(tr("（空）"))->setEnabled(false);
        return;
    }
    for (const QString &path : recents) {
        auto *act = m_recentProjectsMenu->addAction(QFileInfo(path).fileName());
        act->setToolTip(path);
        connect(act, &QAction::triggered, this, [this, path]() {
            openProjectPath(path);
        });
    }
}

void MainWindow::onProjectSettings()
{
    if (!hasProject()) return;

    ProjectSettingsDialog dialog(m_project, this);
    if (dialog.exec() != QDialog::Accepted) return;

    dialog.applyToProject();                 // 写回 SDK / 启动场景 / inputMappings
    m_project.saveConfig();
    // 全局记忆须在 applyToProject 之后取值：否则记住的是修改前的旧 SDK 目录，
    // 「新建项目」向导会预填过期路径
    const QString sdk = m_project.sdkDir();
    m_settings.setValue("lastSdkDir", sdk);
    applyInputMappingsToEngine();             // 输入映射立即生效（播放中也热更新）
    m_log->appendMessage(sdk.isEmpty()
        ? tr("项目设置已更新（输入映射已生效）")
        : tr("项目设置已更新：输入映射已生效，SDK 目录 %1").arg(sdk));
}

void MainWindow::onExportGame()
{
    if (!hasProject()) return;
    ExportDialog dialog(m_project, this);
    dialog.exec();
}

void MainWindow::openIde()
{
    if (!hasProject()) return;

    const QString ide = m_project.ideExePath();
    if (ide.isEmpty()) {
        QMessageBox::information(this, tr("打开代码"),
            tr("尚未配置代码编辑器。\n\n"
               "请通过「文件 → 项目设置… → 通用 → 代码编辑器」选择已安装的 IDE（本机可自动探测），保存后即可一键打开。"));
        return;
    }

    const IdeInfo info{ QFileInfo(ide).fileName(), ide };
    QString err;
    if (startIdeProject(info, m_project.rootDir(), &err)) {
        m_log->appendMessage(tr("已启动代码编辑器 %1，打开项目：%2")
            .arg(info.name, m_project.rootDir()));
    } else {
        m_log->appendMessage(err, Qt::red);
        QMessageBox::warning(this, tr("打开代码"), err);
    }
}

/// 把项目 config.json 的 inputMappings 推入引擎（P15）：
/// Config::loadFromJson 只替换 inputMappings 段（整体清空重载），随后
/// Input::InitMappings 重新编译绑定——未映射时传空对象等价于清空全部绑定。
void MainWindow::applyInputMappingsToEngine()
{
    if (!m_preview || !m_preview->context()) return;
    Shit::EngineContext::setCurrent(m_preview->context());
    Shit::Config::GetInstance().loadFromJson(
        { { "inputMappings", m_project.inputMappings() } });
    Shit::Input::InitMappings();
}

void MainWindow::onBuildScripts()
{
    if (!hasProject()) return;
    if (!m_scriptBuilder) return;
    // 运行中手动构建：先退出运行（Unity 语义：运行中不编译）。
    // 注意：由「▶ 运行」发起的构建（m_playPendingBuild）不能在这里退出——
    // 否则按钮被弹回、m_playPendingBuild 被清，构建完成后不会进入运行态。
    if (isPlaying() && !m_playPendingBuild) setPlaying(false);
    if (m_scriptBuilder->isBuilding()) {
        statusBar()->showMessage(tr("构建进行中…"), 1500);
        return;
    }
    if (!m_project.hasScripts()) {
        m_playPendingBuild = false;   // 「▶ 运行」的构建请求取消（下方弹框已提示）
        QMessageBox::information(this, tr("构建脚本"),
            tr("当前项目没有 C++ 脚本工程（%1 不存在）。\n"
               "请使用「新建项目」时勾选脚本工程，或在项目目录下手动创建。")
                .arg(m_project.scriptsDir() + "/CMakeLists.txt"));
        return;
    }
    if (m_project.sdkDir().trimmed().isEmpty()) {
        m_playPendingBuild = false;   // 「▶ 运行」的构建请求取消（下方弹框已提示）
        QMessageBox::information(this, tr("构建脚本"),
            tr("尚未配置引擎 SDK 目录。\n请先通过「文件 → 项目设置…」指定 SDK（cmake --install 的安装目录）。"));
        return;
    }

    // 热重载用内存 JSON 快照恢复场景（preview 内部完成），无需先保存到磁盘
    m_log->appendMessage(tr("──── 构建脚本开始 ────"));
    m_scriptBuilder->build(m_project.scriptsDir(), m_project.buildDir(),
                           m_project.sdkDir(), m_project.binDir());
    statusBar()->showMessage(tr("正在构建脚本…"));
}

void MainWindow::openFromCommandLine(const QString &projectDir, const QString &sceneFile)
{
    QString project = projectDir;
    // 打开 .scene：若它在某项目目录内（向上找 config.json）则连带打开该项目
    if (project.isEmpty() && !sceneFile.isEmpty()) {
        QDir dir = QFileInfo(sceneFile).absoluteDir();
        while (!dir.isRoot()) {
            if (QFileInfo::exists(dir.filePath(QStringLiteral("config.json")))) {
                project = dir.absolutePath();
                break;
            }
            dir.cdUp();
        }
    }
    if (!project.isEmpty())
        openProjectPath(project, /*silent=*/true);
    if (!sceneFile.isEmpty())
        openScenePath(sceneFile);
}

void MainWindow::syncInspectorToSelection()
{
    const auto objs = m_sceneTree->selectedObjects();
    if (objs.size() > 1)
        m_inspector->setGameObjects(objs);
    else if (objs.size() == 1)
        m_inspector->setGameObject(objs.first());
    else
        m_inspector->setGameObject(nullptr);
}

void MainWindow::onBuildFinished(bool success)
{
    if (!success) {
        // 构建失败：若正在等待「运行」→ 取消运行（按钮复位）
        if (m_playPendingBuild) {
            m_playPendingBuild = false;
            if (m_playAction) m_playAction->setChecked(false);
        }
        statusBar()->showMessage(tr("构建失败（详见日志面板）"), 4000);
        return;
    }
    if (!m_preview || !m_preview->isRunning()) return;

    // 构建成功：产物在 build/out/（MSBuild 写临时目录，避开编辑器对 bin/ DLL 的占用）
    const QString srcDll = m_project.buildOutDir() + "/" + m_project.pluginDllFileName();
    const QString dstDll = m_project.pluginDllPath();
// P33：构建产物缺失 → 状态栏红字（此前只有日志面板）
    if (!QFile::exists(srcDll)) {
        m_log->appendMessage(tr("构建产物缺失（%1）——未重载 DLL").arg(srcDll), Qt::red);
        statusBar()->showMessage(tr("构建产物缺失，脚本未更新"), 8000);
        return;
    }

    // 热重载：reload 内部先卸载旧 DLL（释放文件锁）→ 回调中覆盖拷贝新 DLL → 重新加载
    const bool reloaded = m_preview->reloadProjectPlugins(
        m_project.configPath(),
        [this, srcDll, dstDll]() -> bool {
            if (QFile::exists(dstDll) && !QFile::remove(dstDll)) {
                m_log->appendMessage(tr("无法替换旧 DLL（仍被占用）：%1").arg(dstDll), Qt::red);
                return false;
            }
            if (!QFile::copy(srcDll, dstDll)) {
                m_log->appendMessage(tr("DLL 复制失败：%1 → %2").arg(srcDll, dstDll), Qt::red);
                return false;
            }
            return true;
        });
    if (reloaded) {
        // 场景被重建（对象全部换新）→ 先清旧选中态再重挂树/检查器/瓦片面板
        m_inspector->setGameObject(nullptr);
        m_sceneViewport->setSelectedObject(nullptr);
        m_tileset->setGameObject(nullptr);
        m_animatorDock->setGameObject(nullptr);
        m_sceneTree->setScene(m_preview->getScene());
        m_sceneViewport->setEditScene(m_preview->getScene());
        m_savedSnapshot = snapshot();
        refreshDirtyFromSaved();
        statusBar()->showMessage(tr("脚本已重载"), 3000);
    } else {
        m_log->appendMessage(tr("热重载失败：插件加载异常（详见日志）"), Qt::red);
        // P33：热重载失败（最常见：DLL 被占用/复制失败）弹窗明示——此前仅日志红字，
        // 用户可能浑然不觉场景仍是旧脚本
        statusBar()->showMessage(tr("热重载失败，脚本未更新"), 8000);
        QMessageBox::warning(this, tr("热重载失败"),
            tr("新脚本未能加载：\n通常是因为 DLL 文件被占用或复制失败。\n\n"
               "请关闭正在运行的游戏/导出进程后重试 Ctrl+B，详细输出见日志面板。"));
    }

    // 由「▶ 运行」触发的构建：成功后热重载完毕 → 进入运行态
    if (m_playPendingBuild) {
        m_playPendingBuild = false;
        enterPlayMode();
    }
}

void MainWindow::updateProjectMenus()
{
    const bool enabled = hasProject();
    if (m_newSceneAction)  m_newSceneAction->setEnabled(enabled);
    if (m_openSceneAction) m_openSceneAction->setEnabled(enabled);
    if (m_saveSceneAction) m_saveSceneAction->setEnabled(enabled);
    if (m_saveSceneAsAction) m_saveSceneAsAction->setEnabled(enabled);
    if (m_closeProjectAction) m_closeProjectAction->setEnabled(enabled);
    if (m_projectSettingsAction) m_projectSettingsAction->setEnabled(enabled);
    if (m_exportGameAction) m_exportGameAction->setEnabled(enabled);
    if (m_openIdeAction) m_openIdeAction->setEnabled(enabled);
    if (m_buildAction) m_buildAction->setEnabled(enabled);
    if (m_playAction) m_playAction->setEnabled(enabled);
    for (QAction *a : m_gizmoShortcutActions) a->setEnabled(enabled);
    if (m_recentMenu) m_recentMenu->setEnabled(enabled);
    updateUndoActions();   // 撤销/重做可用性同样受项目态约束
}

void MainWindow::resetToEmptyScene()
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return;

    // 先收集再删（编辑器下 removeGameObject 当场删，迭代中不能删）
    std::vector<Shit::GameObject *> toRemove;
    for (auto &go : scene->getGameObjects()) {
        const std::string name = go->getName();
        if (name != "scene_camera")   // 保留编辑器相机；游戏相机不定名，随场景
            toRemove.push_back(go.get());
    }
    for (auto *go : toRemove)
        scene->removeGameObject(go);

    m_scenePath.clear();
    m_undo.clear();
    m_savedSnapshot = snapshot();
    refreshDirtyFromSaved();
    updateUndoActions();
    m_sceneTree->setScene(scene);
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
            // 组合键（Ctrl+…）保留给编辑器快捷键（Ctrl+S 保存等），不转发给游戏
            if (ke->modifiers() & Qt::ControlModifier)
                return false;
            const SDL_Scancode sc = sdlScancodeForQtKey(ke->key());
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

    // 找到场景视图用的相机：优先约定名 scene_camera（编辑器相机，场景视图视角）；
    // 缺失时回退场景中任一相机（旧场景兼容）。
    Shit::CameraComponent *camera = nullptr;
    for (auto &go : scene->getGameObjects()) {
        auto *cam = go->getComponent<Shit::CameraComponent>();
        if (!cam) continue;
        if (go->getName() == "scene_camera") { camera = cam; break; }
        if (!camera) camera = cam;
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
        // 点空白 → 清空检查器 + 瓦片面板
        m_inspector->setGameObject(nullptr);
        m_tileset->setGameObject(nullptr);
        m_animatorDock->setGameObject(nullptr);

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
    }
}
