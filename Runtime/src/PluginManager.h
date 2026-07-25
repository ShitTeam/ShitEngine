#pragma once

#include <ShitEngine.h>

#include <string>
#include <vector>

/// @brief 动态插件管理器
/// @details 负责加载/卸载插件 DLL，调用插件的 C ABI 函数
///          支持从 JSON 配置文件批量加载插件
class PluginManager {
public:
    /// 插件 ABI 函数指针类型
    using GetABIVersionFn  = int (*)();
    using GetNameFn        = const char* (*)();
    using GetVersionFn     = const char* (*)();
    using RegisterTypesFn  = void (*)();
    using CreateSceneFn    = Shit::Scene* (*)();

    /// 已加载的插件信息
    struct LoadedPlugin {
        void*           handle       = nullptr;  ///< 平台句柄 (HMODULE / void*)
        std::string     name;                    ///< 插件名称
        std::string     path;                    ///< DLL 路径
        int             abiVersion   = 0;        ///< ABI 版本
        std::string     version;                 ///< 插件版本
        CreateSceneFn   createScene  = nullptr;  ///< 创建主场景函数
        RegisterTypesFn registerTypes = nullptr; ///< 注册类型函数
    };

    /// 从 JSON 配置文件加载所有插件
    /// @param configPath 配置文件路径（相对于 CWD）
    void LoadFromConfig(const std::string& configPath);

    /// 调用所有已加载插件的 RegisterPluginTypes()
    /// 必须在 Game::Init() 之后调用
    void RegisterAllTypes();

    /// 创建所有已加载插件的主场景
    /// @return 场景指针列表（调用方获取所有权）
    std::vector<Shit::Scene*> CreateAllScenes();

    /// 卸载所有插件（FreeLibrary / dlclose）
    void UnloadAll();

    /// 获取已加载插件列表
    const std::vector<LoadedPlugin>& GetPlugins() const { return m_plugins; }

private:
    /// 加载单个插件 DLL 并解析其导出函数
    /// @param path DLL 路径
    /// @return 是否加载成功
    bool loadPlugin(const std::string& path);

    /// 平台抽象：加载动态库
    static void* loadLibrary(const std::string& path);
    /// 平台抽象：获取函数指针
    static void* getSymbol(void* handle, const char* name);
    /// 平台抽象：释放动态库
    static void  freeLibrary(void* handle);

    std::vector<LoadedPlugin> m_plugins;
};
