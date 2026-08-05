#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QSplitter>
#include <QTimer>
#include <QToolBar>

#include <fstream>
#include <vector>

#include "viewport.h"
#include "scenetree.h"
#include "inspector.h"
#include "logwidget.h"
#include "preview.h"

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>
#include <ShitEngine/GameObject/Prefab.h>

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

    if (m_preview->start()) {
        m_log->appendMessage(tr("预览已启动（场景 + 运行，共享场景）"));

        // 场景树绑定共享场景（自动选中第一项 → 检查器 + Gizmo）
        m_sceneTree->setScene(m_preview->getScene());
        m_sceneViewport->setEditScene(m_preview->getScene()); // 编辑器交互（平移/缩放/Gizmo）
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
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return;
    // 先收集再删：编辑器下 removeGameObject 当场 erase，不能在遍历中删（迭代器失效）
    std::vector<Shit::GameObject *> toRemove;
    for (auto &go : scene->getGameObjects()) {
        const std::string name = go->getName();
        if (name != "scene_camera" && name != "game_camera")   // 保留两个相机
            toRemove.push_back(go.get());
    }
    for (auto *go : toRemove)
        scene->removeGameObject(go);
    m_sceneTree->setScene(scene);
    m_log->appendMessage(tr("已新建空场景"));
    statusBar()->showMessage(tr("已新建空场景"));
}

void MainWindow::openScene()
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return;

    const QString path = QFileDialog::getOpenFileName(this, tr("打开场景"), QString(), tr("ShitEngine 场景 (*.scene)"));
    if (path.isEmpty()) return;

    try {
        std::ifstream file(path.toStdString());
        nlohmann::json doc;
        file >> doc;

        // 先收集再删（编辑器下 removeGameObject 当场 erase，不能在遍历中删）
        std::vector<Shit::GameObject *> toRemove;
        for (auto &go : scene->getGameObjects())
            if (go->getName() != "scene_camera")
                toRemove.push_back(go.get());
        for (auto *go : toRemove)
            scene->removeGameObject(go);

        // 从存档重建对象
        if (doc.contains("objects")) {
            for (auto &obj : doc["objects"]) {
                const std::string name = obj.value("name", "Object");
                Shit::Prefab::FromJson(obj.value("data", nlohmann::json::array()))
                    .instantiate(scene, name);
            }
        }
        // 兜底：存档没有游戏相机则补一个（否则运行视图无法渲染）
        bool hasGameCam = false;
        for (auto &go : scene->getGameObjects())
            if (go->getName() == "game_camera") { hasGameCam = true; break; }
        if (!hasGameCam) {
            auto *gc = scene->createGameObject("game_camera");
            gc->addComponent<Shit::TransformComponent>();
            gc->addComponent<Shit::CameraComponent>()->setZoom(5.0f);
        }
        // 诊断：载入后的对象清单
        QString names;
        for (auto &go : scene->getGameObjects()) {
            if (!names.isEmpty()) names += ", ";
            names += QString::fromStdString(go->getName());
        }
        m_log->appendMessage(QString("载入后对象(%1): %2").arg(scene->getGameObjects().size()).arg(names));
        m_sceneTree->setScene(scene);
        m_log->appendMessage(tr("场景已从 %1 载入").arg(path));
    } catch (const std::exception &e) {
        m_log->appendMessage(tr("打开场景失败: %1").arg(e.what()), Qt::red);
    }
    statusBar()->showMessage(tr("打开场景完成"));
}

void MainWindow::saveScene()
{
    Shit::Scene *scene = m_preview ? m_preview->getScene() : nullptr;
    if (!scene) return;

    const QString path = QFileDialog::getSaveFileName(this, tr("保存场景"), QString(), tr("ShitEngine 场景 (*.scene)"));
    if (path.isEmpty()) return;

    // 收集除编辑器相机外的对象（编辑器相机是编辑基础设施，不入库）
    nlohmann::json objects = nlohmann::json::array();
    for (auto &go : scene->getGameObjects()) {
        if (go->getName() == "scene_camera") continue;
        nlohmann::json entry;
        entry["name"] = go->getName();
        entry["data"] = Shit::Prefab::Capture(go.get()).toJson();
        objects.push_back(entry);
    }
    const nlohmann::json doc = { { "objects", objects } };

    std::ofstream file(path.toStdString(), std::ios::trunc);
    file << doc.dump(2);
    m_log->appendMessage(tr("场景已保存到 %1（%2 个对象）").arg(path).arg(objects.size()));
    statusBar()->showMessage(tr("场景已保存"));
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
    if (m_preview) m_preview->setPlaying(playing);
    statusBar()->showMessage(playing ? tr("运行中") : tr("已暂停"));
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
