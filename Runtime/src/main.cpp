#include <ShitEngine.h>

#include "PluginManager.h"

#include <nlohmann/json.hpp>

#include <fstream>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#endif

/// 空场景 + 默认相机（无 scene 配置时兜底；相机兜底由 SceneSerializer::fromJson 统一处理）
static std::unique_ptr<Shit::Scene> createEmptyScene() {
    auto scene = std::make_unique<Shit::Scene>("empty");
    scene->init();
    Shit::SceneSerializer::fromJson(
        nlohmann::json({ { "version", 2 }, { "objects", nlohmann::json::array() } }),
        scene.get());
    return scene;
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    // 1. 初始化引擎（日志 / 配置 / 反射内置类型 / SDL / 窗口 / 渲染器 ...）
    if (!Shit::Game::Init()) {
        ST_CORE_ERROR("Game initialization failed");
        return 1;
    }

    // 2. 加载插件（脚本库：只注册反射类型，不搭建场景）
    PluginManager pluginManager;
    pluginManager.LoadFromConfig("config.json");
    pluginManager.RegisterAllTypes();

    // 3. 从 .scene 文件加载场景（config.json 顶层 "scene" 字段，启动/切关共用一套加载器）
    nlohmann::json config;
    {
        std::ifstream cf("config.json");
        if (cf.is_open()) {
            try { cf >> config; }
            catch (const std::exception& e) {
                ST_CORE_WARN("配置文件解析失败（继续按空场景启动）: {}", e.what());
            }
        }
    }
    const std::string scenePath = config.value("scene", "");
    if (!scenePath.empty()) {
        if (Shit::SceneManager::LoadSceneFromFile(scenePath)) {
            ST_CORE_INFO("已从 .scene 加载场景: {}", scenePath);
        } else {
            ST_CORE_ERROR("场景加载失败（{}），按空场景继续运行", scenePath);
            Shit::SceneManager::LoadScene(std::move(createEmptyScene()));
        }
    } else {
        ST_CORE_WARN("config.json 未配置 scene 字段，启动空场景 + 默认相机");
        Shit::SceneManager::LoadScene(std::move(createEmptyScene()));
    }

    // 4. 主循环
    Shit::Game::Run();

    // 5. 清理（先卸载插件再销毁引擎，避免 SDL_Quit 后 DLL 析构访问 SDL 状态）
    pluginManager.UnloadAll();
    Shit::Game::Destroy();

    return 0;
}
