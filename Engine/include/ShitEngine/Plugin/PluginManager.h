#pragma once

#include <ShitEngine/Core/Core.h>
#include <ShitEngine/Reflection/TypeRegistry.h>

#include <string>
#include <vector>

namespace Shit {

/**
 * @brief 动态插件管理器（引擎级共享：Runtime 与编辑器共用）
 *
 * 负责加载/卸载插件 DLL，调用插件的 C ABI 函数，支持从 JSON 配置文件批量加载。
 *
 * 插件 = 脚本库（ABI v2）：只负责注册反射类型，不搭建场景（场景来自 .scene 文件）。
 * 使用前提：调用前须处于目标 EngineContext（TypeRegistry 按上下文存放），
 * 插件注册的类型 factory 位于 DLL 内，故 UnloadAll() 必须先于引擎销毁执行。
 */
class SHIT_API PluginManager final {
public:
    /// 插件 ABI 版本（插件侧通过 GetPluginABIVersion() 返回该常量）
    static constexpr int kAbiVersion = 2;

    /// 插件 C ABI 函数指针类型
    using GetABIVersionFn  = int (*)();
    using GetNameFn        = const char* (*)();
    using GetVersionFn     = const char* (*)();
    using RegisterTypesFn  = void (*)();

    ~PluginManager() { UnloadAll(); }

    /// 已加载的插件信息
    struct LoadedPlugin {
        void*           handle        = nullptr;  ///< 平台句柄 (HMODULE / void*)
        std::string     name;                     ///< 插件名称
        std::string     path;                     ///< DLL 路径
        int             abiVersion    = 0;        ///< ABI 版本
        std::string     version;                  ///< 插件版本
        RegisterTypesFn registerTypes = nullptr;  ///< 注册类型函数
    };

    /// 从 JSON 配置文件加载所有插件（config → "plugins": [{"path": ...}]）
    /// @param configPath 配置文件路径（相对于 CWD）
    void LoadFromConfig(const std::string& configPath);

    /// 调用所有已加载插件的 RegisterPluginTypes()
    /// 必须在 Game::Init() 之后调用（引擎内置类型已注册）
    void RegisterAllTypes();

    /// @brief 最近一次插件加载失败的描述（P33：编辑器据此弹窗提示用户）。
    /// 每次 LoadFromConfig 开始时清空；加载成功/未加载任何插件时为空字符串。
    static const std::string& GetLastLoadError();

    /// 卸载所有插件（FreeLibrary / dlclose）。注意：必须先于引擎销毁执行。
    void UnloadAll();

    /// 获取已加载插件列表
    const std::vector<LoadedPlugin>& GetPlugins() const { return m_plugins; }

private:
    /// 加载单个插件 DLL 并解析其导出函数
    bool loadPlugin(const std::string& path);

    /// 平台抽象：加载动态库
    static void* loadLibrary(const std::string& path);
    /// 平台抽象：获取函数指针
    static void* getSymbol(void* handle, const char* name);
    /// 平台抽象：释放动态库
    static void  freeLibrary(void* handle);

    std::vector<LoadedPlugin> m_plugins;
};

} // namespace Shit