#include <ShitEngine.h>

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
    #include <filesystem>
#else
    #include <unistd.h>
#endif

/// 把进程 CWD 切到 exe 所在目录（P18 导出游戏：
/// 引擎所有加载——场景/纹理/字体/插件/config.json——相对 CWD；
/// 切目录后游戏包任意位置双击/启动均可运行，不受调用方 CWD 影响）。
/// 仓库内从 bin/ 启动的旧方式 CWD 本就等于 exe 目录，行为不变。
static void chdirToExecutableDir() {
#ifdef _WIN32
    wchar_t buf[MAX_PATH] = {};
    const DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (len > 0) {
        const std::filesystem::path exe(buf);
        SetCurrentDirectoryW(exe.parent_path().c_str());
    }
#else
    // Linux: /proc/self/exe；macOS 同路径可用（Mach-O 也支持）
    char buf[4096] = {};
    const ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        std::string path(buf);
        const size_t slash = path.find_last_of('/');
        if (slash != std::string::npos) {
            path.resize(slash);   // 去文件名 → exe 目录
            if (path.empty()) path = "/";
            chdir(path.c_str());
        }
    }
#endif
}

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
    chdirToExecutableDir();   // 游戏包任意位置运行：CWD → exe 所在目录（P18）
    // 1. 初始化引擎（日志 / 配置 / 反射内置类型 / SDL / 窗口 / 渲染器 ...）
    if (!Shit::Game::Init()) {
        ST_CORE_ERROR("Game initialization failed");
        return 1;
    }

    // 2. 加载插件（脚本库：只注册反射类型，不搭建场景；PluginManager 由引擎提供）
    Shit::PluginManager pluginManager;
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
