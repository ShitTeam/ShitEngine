#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QSplitter>

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

    // 双视口预览：Scene=编辑器相机全貌，Game=游戏相机居中（对齐 Unity/Godot）
    m_scenePreview = new EnginePreview(ViewMode::Scene, this);
    connect(m_scenePreview, &EnginePreview::frameReady, m_sceneViewport, &Viewport::setFrame);
    connect(m_scenePreview, &EnginePreview::frameReady, m_inspector, &Inspector::refresh); // 每帧回读引擎值

    m_gamePreview = new EnginePreview(ViewMode::Game, this);
    connect(m_gamePreview, &EnginePreview::frameReady, m_gameViewport, &Viewport::setFrame);

    if (m_scenePreview->start() && m_gamePreview->start()) {
        m_log->appendMessage(tr("双视口预览已启动（场景 + 运行）"));

        // P4：默认选中场景视图里的玩家对象，属性检查器反射其组件
        Shit::Scene *scene = m_scenePreview->getScene();
        if (scene && !scene->getGameObjects().empty())
            m_inspector->setGameObject(scene->getGameObjects().front().get());
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
