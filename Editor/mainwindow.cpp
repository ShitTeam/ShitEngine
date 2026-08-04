#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>

#include "viewport.h"
#include "scenetree.h"
#include "inspector.h"
#include "logwidget.h"
#include "preview.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_viewport(nullptr)
    , m_sceneTree(nullptr)
    , m_inspector(nullptr)
    , m_log(nullptr)
{
    ui->setupUi(this);

    createDocks();
    createMenus();

    // 引擎预览：离屏渲染 → 视口显示
    m_preview = new EnginePreview(this);
    connect(m_preview, &EnginePreview::frameReady, m_viewport, &Viewport::setFrame);
    if (m_preview->start())
        m_log->appendMessage(tr("引擎预览已启动（离屏渲染）"));
    else
        m_log->appendMessage(tr("引擎预览启动失败"), Qt::red);

    statusBar()->showMessage(tr("就绪"));
    m_log->appendMessage(tr("ShitEngine 编辑器已启动"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createDocks()
{
    // 中央视口（游戏渲染区域）
    m_viewport = new Viewport(this);
    setCentralWidget(m_viewport);

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
