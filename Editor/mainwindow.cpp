#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QDockWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QSplitter>
#include <QToolBar>

#include "viewport.h"
#include "scenetree.h"
#include "inspector.h"
#include "logwidget.h"
#include "preview.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_sceneViewport(nullptr)
    , m_gameViewport(nullptr)
    , m_sceneTree(nullptr)
    , m_inspector(nullptr)
    , m_log(nullptr)
{
    ui->setupUi(this);

    createDocks();
    createMenus();
    createToolbar();

    // 场景视图点击 → 拾取（须在双视口创建后连接）
    connect(m_sceneViewport, &Viewport::logicalClicked, this, &MainWindow::pickSceneAt);

    // 双视口预览：Scene=编辑器相机全貌，Game=游戏相机居中（对齐 Unity/Godot）
    m_scenePreview = new EnginePreview(ViewMode::Scene, this);
    connect(m_scenePreview, &EnginePreview::frameReady, m_sceneViewport, &Viewport::setFrame);
    connect(m_scenePreview, &EnginePreview::frameReady, m_inspector, &Inspector::refresh); // 每帧回读引擎值

    m_gamePreview = new EnginePreview(ViewMode::Game, this);
    connect(m_gamePreview, &EnginePreview::frameReady, m_gameViewport, &Viewport::setFrame);

    // P3：场景树选中 → 属性检查器
    connect(m_sceneTree, &SceneTree::objectSelected, m_inspector, &Inspector::setGameObject);

    if (m_scenePreview->start() && m_gamePreview->start()) {
        m_log->appendMessage(tr("双视口预览已启动（场景 + 运行）"));

        // P3：场景树绑定场景（自动选中第一项，触发检查器）
        m_sceneTree->setScene(m_scenePreview->getScene());
        setPlaying(m_playAction->isChecked());   // 默认停止态：暂停预览逻辑
    } else {
        m_log->appendMessage(tr("预览启动失败"), Qt::red);
    }

    statusBar()->showMessage(tr("就绪"));
    m_log->appendMessage(tr("ShitEngine 编辑器已启动"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createDocks()
{
    // 中央双视口：左=场景视图（编辑器相机全貌），右=运行视图（游戏相机居中）
    m_sceneViewport = new Viewport(this);
    m_gameViewport = new Viewport(this);

    auto *splitter = new QSplitter(this);
    splitter->addWidget(m_sceneViewport);
    splitter->addWidget(m_gameViewport);
    splitter->setSizes({600, 600});
    setCentralWidget(splitter);

    // 左侧：场景树
    auto *sceneDock = new QDockWidget(tr("场景"), this);
    sceneDock->setObjectName("sceneDock");
    m_sceneTree = new SceneTree(sceneDock);
    sceneDock->setWidget(m_sceneTree);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);

    // 右侧：属性检查器
    auto *inspectorDock = new QDockWidget(tr("属性"), this);
    inspectorDock->setObjectName("inspectorDock");
    m_inspector = new Inspector(inspectorDock);
    inspectorDock->setWidget(m_inspector);
    addDockWidget(Qt::RightDockWidgetArea, inspectorDock);

    // 底部：日志
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
    fileMenu->addAction(tr("新建场景"), this, &MainWindow::newScene);
    fileMenu->addAction(tr("打开场景…"), this, &MainWindow::openScene);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("保存场景"), this, &MainWindow::saveScene);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出"), this, &QWidget::close);

    auto *helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(tr("关于"), this, &MainWindow::about);
}

void MainWindow::newScene()
{
    m_log->appendMessage(tr("新建场景（P3 实现）"));
    statusBar()->showMessage(tr("新建场景…（开发中）"));
}

void MainWindow::openScene()
{
    m_log->appendMessage(tr("打开场景（P5 实现）"));
    statusBar()->showMessage(tr("打开场景…（开发中）"));
}

void MainWindow::saveScene()
{
    m_log->appendMessage(tr("保存场景（P5 实现）"));
    statusBar()->showMessage(tr("保存场景…（开发中）"));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("关于 ShitEngine 编辑器"),
        tr("ShitEngine 编辑器\n"
           "基于 Qt 6 与 ShitEngine 引擎。\n"
           "P1：UI 骨架（视口 / 场景树 / 属性 / 日志）。"));
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
}

void MainWindow::setPlaying(bool playing)
{
    if (m_playAction)
        m_playAction->setText(playing ? tr("⏹ 停止") : tr("▶ 播放"));
    if (m_scenePreview) m_scenePreview->setPlaying(playing);
    if (m_gamePreview)  m_gamePreview->setPlaying(playing);
    statusBar()->showMessage(playing ? tr("运行中") : tr("已暂停"));
}

void MainWindow::pickSceneAt(float x, float y)
{
    Shit::Scene *scene = m_scenePreview ? m_scenePreview->getScene() : nullptr;
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
    // 测点击是否落在其中（与渲染同变换，屏幕位置精确一致，比逆变换更稳）
    Shit::GameObject *hit = nullptr;
    for (auto &go : scene->getGameObjects()) {
        if (auto *sprite = go->getComponent<Shit::SpriteRenderer>()) {
            const SDL_FRect b = sprite->getGlobalBounds();
            const Shit::Vector2 tl = camera->worldToScreen({ b.x, b.y });
            const Shit::Vector2 br = camera->worldToScreen({ b.x + b.w, b.y + b.h });
            const float sxl = std::min(tl.x, br.x);
            const float syt = std::min(tl.y, br.y);
            const float sxr = std::max(tl.x, br.x);
            const float syb = std::max(tl.y, br.y);
            if (click.x >= sxl && click.x <= sxr && click.y >= syt && click.y <= syb) {
                hit = go.get();
                break;
            }
        }
    }

    if (hit) {
        m_log->appendMessage(QString("pick: 命中 %1 @logical(%2,%3)")
            .arg(QString::fromStdString(hit->getName())).arg(click.x, 0, 'f', 1).arg(click.y, 0, 'f', 1));
        m_sceneTree->selectObject(hit);   // → objectSelected → 检查器
    } else {
        m_log->appendMessage(QString("pick: 未命中 @logical(%1,%2)")
            .arg(click.x, 0, 'f', 1).arg(click.y, 0, 'f', 1), Qt::yellow);
        m_inspector->setGameObject(nullptr); // 点空白 → 清空检查器
    }
}
