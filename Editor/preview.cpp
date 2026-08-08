#include "preview.h"

#include <QCoreApplication>
#include <QFile>

#include <ShitEngine.h>
#include <ShitEngine/Core/EngineContext.h>
#include <ShitEngine/Core/Log.h>
#include <ShitEngine/Plugin/PluginManager.h>
#include <ShitEngine/Scene/SceneSerializer.h>

#include <nlohmann/json.hpp>

EnginePreview::EnginePreview(QObject *parent)
    : QObject(parent)
{
}

EnginePreview::~EnginePreview()
{
    stop();
}

bool EnginePreview::start()
{
    if (m_running) return true;

    m_context = std::make_unique<Shit::EngineContext>();
    Shit::EngineContext::setCurrent(m_context.get());

    Shit::Window::SetHidden(true); // 离屏渲染：隐藏窗口，渲染照常进行
    if (!Shit::Game::Init()) return false;

    // P12：引擎 spdlog → 编辑器日志面板（队列连接，兼容任意线程抵达）
    Shit::Log::SetMessageCallback([this](bool isCore, int level, const std::string &message) {
        emit engineLogMessage(isCore, level, QString::fromStdString(message));
    });

    // 加载插件（脚本库：注册自定义行为/组件类型 → 编辑器可实例化/序列化）。
    // 配置文件与 exe 同目录（与 Runtime 一致），缺失时忽略，不阻塞编辑器启动。
    m_plugins = std::make_unique<Shit::PluginManager>();
    const QString configPath = QCoreApplication::applicationDirPath() + "/config.json";
    if (QFile::exists(configPath)) {
        m_plugins->LoadFromConfig(configPath.toStdString());
        m_plugins->RegisterAllTypes();
    }

    // 构建共享场景：默认空场景，仅两个相机（无测试对象/行为）
    auto scene = std::make_unique<Shit::Scene>("preview");
    scene->init();

    auto *editorGo = scene->createGameObject("scene_camera");
    editorGo->addComponent<Shit::TransformComponent>();
    m_editorCam = editorGo->addComponent<Shit::CameraComponent>();
    m_editorCam->setZoom(1.0f);

    auto *gameGo = scene->createGameObject("game_camera");
    gameGo->addComponent<Shit::TransformComponent>();
    m_gameCam = gameGo->addComponent<Shit::CameraComponent>();
    m_gameCam->setZoom(1.0f);

    Shit::SceneManager::LoadScene(std::move(scene));
    m_scene = Shit::SceneManager::GetCurrentScene();
    refreshCameras();   // 按名定位编辑器/游戏相机

    m_logicalWidth = Shit::Renderer::GetLogicalWidth();
    m_logicalHeight = Shit::Renderer::GetLogicalHeight();
    m_pixels.resize(static_cast<size_t>(m_logicalWidth) * m_logicalHeight * 4);

    connect(&m_timer, &QTimer::timeout, this, &EnginePreview::tick);
    m_timer.start(1000 / 60);
    m_running = true;
    return true;
}

void EnginePreview::stop()
{
    if (!m_running) return;
    m_timer.stop();
    Shit::Log::SetMessageCallback(nullptr);   // 先解除日志转发（回调捕获 this，避免悬垂）
    if (m_context) {
        Shit::EngineContext::setCurrent(m_context.get());
        // 先卸载插件（其反射类型 factory 在 DLL 内），再销毁引擎
        if (m_plugins) m_plugins->UnloadAll();
        Shit::Game::Destroy();
        Shit::EngineContext::resetCurrent();
        m_context.reset();
    }
    m_scene = nullptr;
    m_editorCam = nullptr;
    m_gameCam = nullptr;
    m_running = false;
}

Shit::Scene *EnginePreview::getScene()
{
    if (!m_context) return nullptr;
    Shit::EngineContext::setCurrent(m_context.get());
    return Shit::SceneManager::GetCurrentScene();
}

bool EnginePreview::loadProjectConfig(const QString &configPath)
{
    if (!m_running || !m_context) return false;
    Shit::EngineContext::setCurrent(m_context.get());

    clearSceneObjects();
    if (m_plugins) m_plugins->UnloadAll();
    m_plugins = std::make_unique<Shit::PluginManager>();

    if (!QFile::exists(configPath)) {
        ST_CORE_INFO("[Preview] 无插件配置文件（仅卸载旧插件）: {}", configPath.toStdString());
        return true;
    }
    m_plugins->LoadFromConfig(configPath.toStdString());
    m_plugins->RegisterAllTypes();
    return true;
}

void EnginePreview::unloadPlugins()
{
    if (!m_running || !m_context) return;
    Shit::EngineContext::setCurrent(m_context.get());

    clearSceneObjects();
    if (m_plugins) m_plugins->UnloadAll();
    m_plugins = std::make_unique<Shit::PluginManager>();
}

void EnginePreview::clearSceneObjects()
{
    auto *scene = Shit::SceneManager::GetCurrentScene();
    if (!scene) return;
    // 先收集再删（removeGameObject 当场 erase，不能边遍历边删）
    std::vector<Shit::GameObject *> all;
    for (auto &go : scene->getGameObjects()) {
        if (go->getName() != "scene_camera")
            all.push_back(go.get());
    }
    for (auto *go : all)
        scene->removeGameObject(go);
}

bool EnginePreview::reloadProjectPlugins(const QString &configPath,
                                         const std::function<bool()> &onDllReplaced)
{
    if (!m_running || !m_context) return false;
    Shit::EngineContext::setCurrent(m_context.get());

    auto *scene = Shit::SceneManager::GetCurrentScene();
    if (!scene) return false;

    // 1) 场景全量快照（含编辑器相机：恢复后编辑视点不丢）
    nlohmann::json snapshot;
    try {
        snapshot = Shit::SceneSerializer::toJson(scene);
    } catch (const std::exception &e) {
        ST_CORE_ERROR("[Preview] 热重载快照失败: {}", e.what());
        return false;
    }

    // 2) 销毁场景对象（组件析构调用旧 DLL 代码，必须在 FreeLibrary 前完成）
    clearSceneObjects();

    // 3) 卸载旧插件 → 释放 DLL 文件锁
    if (m_plugins) m_plugins->UnloadAll();
    m_plugins = std::make_unique<Shit::PluginManager>();

    // 4) 文件替换回调（复制新 DLL 覆盖 bin/；此时旧 DLL 已无锁）
    if (onDllReplaced && !onDllReplaced()) {
        ST_CORE_ERROR("[Preview] 热重载：DLL 替换失败，已恢复原场景（未加载新插件）");
        try {
            Shit::SceneSerializer::fromJson(snapshot, scene);
        } catch (const std::exception &e) {
            ST_CORE_ERROR("[Preview] 热重载失败后场景恢复又失败: {}", e.what());
        }
        refreshCameras();
        return false;
    }

    // 5) 加载新 DLL + 注册反射类型
    if (!QFile::exists(configPath)) {
        ST_CORE_WARN("[Preview] 热重载：项目配置不存在，仅卸载插件: {}", configPath.toStdString());
        return true;
    }
    m_plugins->LoadFromConfig(configPath.toStdString());
    m_plugins->RegisterAllTypes();

    // 6) 从快照恢复场景（相机兜底：快照内含相机，不会新增）
    try {
        Shit::SceneSerializer::fromJson(snapshot, scene);
    } catch (const std::exception &e) {
        ST_CORE_ERROR("[Preview] 热重载场景恢复失败: {}", e.what());
        return false;
    }

    refreshCameras();
    ST_CORE_INFO("[Preview] 热重载完成：插件已替换，场景已恢复（{} 个对象）", scene->getGameObjects().size());
    return true;
}

void EnginePreview::setPlaying(bool playing)
{
    if (!m_context) return;
    Shit::EngineContext::setCurrent(m_context.get());
    Shit::Game::SetPaused(!playing);
}

void EnginePreview::refreshCameras()
{
    m_editorCam = nullptr;
    m_gameCam = nullptr;
    if (!m_scene) return;
    for (auto &go : m_scene->getGameObjects()) {
        if (go->getName() == "scene_camera")
            m_editorCam = go->getComponent<Shit::CameraComponent>();
        else if (go->getName() == "game_camera")
            m_gameCam = go->getComponent<Shit::CameraComponent>();
    }
}

void EnginePreview::tick()
{
    if (!m_context) return;
    Shit::EngineContext::setCurrent(m_context.get());

    refreshCameras();   // 每帧按名定位（场景加载/编辑后相机可能重建，防悬空指针）
    if (!m_editorCam || !m_gameCam) return;

    Shit::Time::Update();
    Shit::EventBus::ProcessEvents();
    Shit::Input::Update();
    Shit::AudioPlayer::Update();

    // 游戏视图 pass：驱动逻辑一次（Behavior 等），渲染游戏相机 → target → 读图
    m_gameCam->setEnabled(true);
    m_editorCam->setEnabled(false);
    Shit::Renderer::BeginOffscreen();
    Shit::SceneManager::Update();
    if (Shit::Renderer::ReadPixels(m_pixels.data(), m_logicalWidth * 4)) {
        QImage image(m_pixels.data(), m_logicalWidth, m_logicalHeight,
                     m_logicalWidth * 4, QImage::Format_ARGB32);
        emit gameFrameReady(image.copy());
    }

    // 场景视图 pass：仅重渲染编辑器相机（不重跑逻辑），复用同一目标纹理
    m_editorCam->setEnabled(true);
    m_gameCam->setEnabled(false);
    if (m_scene) {
        if (auto *renderSystem = m_scene->getSystem<Shit::RenderSystem>())
            renderSystem->update();
    }
    if (Shit::Renderer::ReadPixels(m_pixels.data(), m_logicalWidth * 4)) {
        QImage image(m_pixels.data(), m_logicalWidth, m_logicalHeight,
                     m_logicalWidth * 4, QImage::Format_ARGB32);
        emit sceneFrameReady(image.copy());
    }
    Shit::Renderer::EndOffscreen();

    // 收尾复位：编辑 pass 结束后把 game_camera 恢复为启用。否则保存场景时若恰好
    // 处于禁用态（双 pass 竞态），会被序列化成禁用相机，下次加载触发兜底，
    // 甚至新建第二个 game_camera 导致对象渲染双份。编辑器相机不入库，无需复位。
    m_gameCam->setEnabled(true);
    m_editorCam->setEnabled(false);
}
