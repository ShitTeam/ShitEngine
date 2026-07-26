#include <ShitEngine.h>

#include "PluginManager.h"

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#endif

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif

    // 1. 初始化引擎（日志 / 配置 / 反射内置类型 / SDL / 窗口 / 渲染器 ...）
    if (!Shit::Game::Init()) {
        ST_CORE_ERROR("Game initialization failed");
        return 1;
    }

    // 2. 加载插件
    PluginManager pluginManager;
    pluginManager.LoadFromConfig("config.json");

    // 3. 注册插件中的反射类型（引擎类型已在 Game::Init() 中注册）
    pluginManager.RegisterAllTypes();

    // 4. 创建插件场景并推入 SceneManager
    auto scenes = pluginManager.CreateAllScenes();
    for (auto* scene : scenes) {
        Shit::SceneManager::PushScene(std::unique_ptr<Shit::Scene>(scene));
    }

    // 5. 主循环
    Shit::Game::Run();

    // 6. 清理（先卸载插件再销毁引擎，避免 SDL_Quit 后 DLL 析构访问 SDL 状态）
    pluginManager.UnloadAll();
    Shit::Game::Destroy();

    return 0;
}
