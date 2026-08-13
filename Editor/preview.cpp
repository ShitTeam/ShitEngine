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

    // 构建共享场景：默认空场景，仅编辑器相机（无测试对象/行为）。
    // 游戏相机不定名（Unity 语义）：由 refreshCameras 从场景中挑选任意启用相机。
    auto scene = std::make_unique<Shit::Scene>("preview");
    scene->init();

    auto *editorGo = scene->createGameObject("scene_camera");
    editorGo->addComponent<Shit::TransformComponent>();
    m_editorCam = editorGo->addComponent<Shit::CameraComponent>();
    m_editorCam->setZoom(1.0f);

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
        Shit::Game::SetIsRunning(false);   // 复位运行态标记，避免残留影响后续初始化
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

    // 2.5) 卸载前清理插件注册的系统（防止 DLL 卸载后 vtable 悬垂）
    // 遍历场景系统，卸载所有 TypeInfo.source 非空（来自插件）的系统
    for (const auto& name : scene->getRegisteredSystemTypeNames()) {
        const auto* ti = Shit::TypeRegistry::Get(name);
        if (ti && !ti->source.empty()) {
            scene->unregisterSystem(name);
        }
    }

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
    // 播放 = 引擎运行态：Scene 增删走延时路径（与 Runtime 一致），
    // 游戏逻辑在迭代中删除对象安全；停止播放时复位回编辑态语义
    Shit::Game::SetIsRunning(playing);
}

void EnginePreview::refreshCameras()
{
    Shit::CameraComponent *prev = m_gameCam;   // 保存上一帧选择的游戏相机（清零前取，延续逻辑才生效）
    m_editorCam = nullptr;
    m_gameCam = nullptr;
    if (!m_scene) return;

    // 编辑器相机：约定名 scene_camera（场景视图基础设施，不入库/不参与游戏相机选择）
    for (auto &go : m_scene->getGameObjects()) {
        if (go->getName() == "scene_camera") {
            m_editorCam = go->getComponent<Shit::CameraComponent>();
            break;
        }
    }

    // 游戏相机：Unity 语义——场景里任何启用相机都行（不按名约定）。上一帧选中的
    // 用户相机若仍在场景则延续（避免 tick 双 pass 切换 enabled 造成选择抖动）；
    // 否则取 priority 最小的相机（与 RenderSystem 渲染顺序一致），无用户相机时
    // 兜底编辑器相机，保证运行视口画面不断流。
    // 注意：延续前必须先用指针比对做存活校验——prev 是跨帧缓存，播放中游戏逻辑
    // 销毁相机/切换场景（旧场景整体销毁）后它就是悬垂指针，直接 prev->getOwner()
    // 会解引用已释放内存（调试堆 0xDD 填充 → std::string::_Equal 读 0xffffffff
    // 访问违例，曾在此崩溃）。先遍历当前场景按指针比对确认存活，再解引用。
    bool prevAlive = false;
    if (prev && m_scene) {
        for (auto &go : m_scene->getGameObjects()) {
            if (go->getComponent<Shit::CameraComponent>() == prev) { prevAlive = true; break; }
        }
    }
    if (prevAlive && prev->getOwner()
            && prev->getOwner()->getName() != "scene_camera"
            && m_scene->containsGameObject(prev->getOwner())
            && prev->getOwner()->getComponent<Shit::CameraComponent>() == prev) {
        m_gameCam = prev;
        return;
    }

    Shit::CameraComponent *best = nullptr;
    for (auto &go : m_scene->getGameObjects()) {
        if (go->getName() == "scene_camera") continue;   // 编辑器相机不作为游戏相机
        auto *cam = go->getComponent<Shit::CameraComponent>();
        if (!cam) continue;
        if (!best || cam->getPriority() < best->getPriority())
            best = cam;
    }
    m_gameCam = best ? best : m_editorCam;
}

void EnginePreview::ensureCameras()
{
    if (!m_scene) return;
    // 只保证编辑器相机（scene_camera）存在：它是场景视图基础设施。
    // 游戏相机不定名，由 refreshCameras 从场景挑选（Unity 语义），缺失时
    // 兜底编辑器相机，无需补建任何游戏相机。
    if (!m_editorCam) {
        auto *go = m_scene->createGameObject("scene_camera");
        go->addComponent<Shit::TransformComponent>();
        m_editorCam = go->addComponent<Shit::CameraComponent>();
        if (m_editorCam) m_editorCam->setZoom(1.0f);
    }
}

void EnginePreview::tick()
{
    if (!m_context) return;
    Shit::EngineContext::setCurrent(m_context.get());

    // 场景指针每帧跟随 SceneManager：播放中游戏代码 LoadScene 会整体替换场景
    //（旧场景销毁），若仍持有旧指针，refreshCameras/渲染会解引用已释放内存。
    m_scene = Shit::SceneManager::GetCurrentScene();
    refreshCameras();   // 每帧重新定位（场景加载/编辑后相机可能重建，防悬空指针）
    if (!m_editorCam) {
        ensureCameras();        // 编辑器相机被（误）删/未建：自愈补齐，保证循环继续
        refreshCameras();
    }
    // 注意：不因相机缺失而提前 return——播放态 ensureCameras() 新建的对象走延时
    // 添加（m_pendingAdditions），要等下方 SceneManager::Update() 统一 flush 才进
    // 容器；提前 return 会让对象永远进不了场景，双视口永久冻结。缺失的相机由
    // 各 pass 的空守卫跳过、pass 后重新定位。

    Shit::Time::Update();
    Shit::EventBus::ProcessEvents();
    Shit::Input::Update();
    Shit::AudioPlayer::Update();

    // 游戏视图 pass：驱动逻辑一次（Behavior 等），渲染游戏相机 → target → 读图
    if (m_editorCam) m_editorCam->setEnabled(false);
    if (m_gameCam) m_gameCam->setEnabled(true);
    Shit::Renderer::BeginOffscreen();
    Shit::SceneManager::Update();
    // 播放中游戏逻辑可能 LoadScene 整体替换场景（旧场景销毁）——立即重取 m_scene，
    // 否则下方 refreshCameras/渲染会解引用已释放的 Scene（UAF）。
    m_scene = Shit::SceneManager::GetCurrentScene();
    // 播放中 ensureCameras()/游戏逻辑创建的对象此刻才入容器；游戏逻辑也可能在
    // update 中销毁相机 → 每个 pass 后重新定位，既让新建的相机立即可用，也防
    // 本帧缓存指针在 pass 间变成悬垂。
    refreshCameras();
    if (m_gameCam && Shit::Renderer::ReadPixels(m_pixels.data(), m_logicalWidth * 4)) {
        QImage image(m_pixels.data(), m_logicalWidth, m_logicalHeight,
                     m_logicalWidth * 4, QImage::Format_ARGB32);
        emit gameFrameReady(image.copy());
    }

    // 场景视图 pass：只渲染编辑器相机（不重跑逻辑），复用同一目标纹理。
    // 其余相机（游戏相机/分屏相机）渲染期间暂时禁用——否则它们的内容会叠加进
    // 编辑帧，多相机分屏时场景视图被各视口内容淹没（Unity 场景视图同样只看
    // 编辑器视角；游戏侧多相机 viewport 在运行视图 pass 照常全部渲染）。
    std::vector<std::pair<Shit::GameObject *, Shit::CameraComponent *>> userCams;
    if (m_scene) {
        for (auto &go : m_scene->getGameObjects()) {
            if (go->getName() == "scene_camera") continue;
            if (auto *cam = go->getComponent<Shit::CameraComponent>())
                userCams.emplace_back(go.get(), cam);
        }
    }
    if (m_editorCam) m_editorCam->setEnabled(true);
    for (auto &[owner, cam] : userCams) cam->setEnabled(false);
    if (m_scene) {
        if (auto *renderSystem = m_scene->getSystem<Shit::RenderSystem>())
            renderSystem->update();
    }
    // 恢复其余相机：渲染回调可能重入销毁对象。先以地址比对 owner 仍在场景再解引用，
    // 避免对已释放的 cam 调 getOwner()（悬垂 UAF）。
    for (auto &[owner, cam] : userCams) {
        if (!m_scene) break;
        if (m_scene->containsGameObject(owner)
            && owner->getComponent<Shit::CameraComponent>() == cam)
            cam->setEnabled(true);
    }
    // 渲染回调可能重入增删对象 → 发帧前再重新定位一次，避免把悬垂指针传给界面
    refreshCameras();
    if (m_editorCam && Shit::Renderer::ReadPixels(m_pixels.data(), m_logicalWidth * 4)) {
        QImage image(m_pixels.data(), m_logicalWidth, m_logicalHeight,
                     m_logicalWidth * 4, QImage::Format_ARGB32);
        emit sceneFrameReady(image.copy());
    }
    Shit::Renderer::EndOffscreen();

    // 收尾复位：编辑 pass 结束后把游戏相机恢复为启用。否则保存场景时若恰好
    // 处于禁用态（双 pass 竞态），会被序列化成禁用相机，下次加载触发引擎兜底。
    // 编辑器相机（scene_camera）不入库，无需复位。
    if (m_gameCam) m_gameCam->setEnabled(true);
    if (m_editorCam) m_editorCam->setEnabled(false);
}
