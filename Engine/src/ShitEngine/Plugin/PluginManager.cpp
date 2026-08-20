#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Plugin/PluginManager.h"

#include "ShitEngine/Core/Log.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#else
    #include <dlfcn.h>
#endif

namespace Shit {

// ── 最近一次加载失败描述（P33：编辑器弹窗提示用）──
namespace {
    std::string g_lastLoadError;
    void setLoadError(const std::string& msg) {
        g_lastLoadError = msg;
        ST_CORE_ERROR("[PluginManager] {}", msg);
    }
}

const std::string& PluginManager::GetLastLoadError() {
    return g_lastLoadError;
}

// ── 平台抽象 ────────────────────────────────────────────

void* PluginManager::loadLibrary(const std::string& path) {
#ifdef _WIN32
    return reinterpret_cast<void*>(LoadLibraryA(path.c_str()));
#else
    return dlopen(path.c_str(), RTLD_LAZY);
#endif
}

void* PluginManager::getSymbol(void* handle, const char* name) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
#else
    return dlsym(handle, name);
#endif
}

void PluginManager::freeLibrary(void* handle) {
#ifdef _WIN32
    FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
    dlclose(handle);
#endif
}

// ── 加载单个插件 ────────────────────────────────────────

bool PluginManager::loadPlugin(const std::string& path) {
    void* handle = loadLibrary(path);
    if (!handle) {
#ifdef _WIN32
        setLoadError(fmt::format("加载插件失败 {}（GetLastError={}）", path, GetLastError()));
#else
        setLoadError(fmt::format("加载插件失败 {}（dlerror={}）", path, dlerror()));
#endif
        return false;
    }

    LoadedPlugin plugin;
    plugin.handle = handle;
    plugin.path   = path;

    // ABI v2：插件只负责注册反射类型，不导出场景工厂、不搭建场景
    auto* getABI   = reinterpret_cast<GetABIVersionFn>(getSymbol(handle, "GetPluginABIVersion"));
    auto* getName  = reinterpret_cast<GetNameFn>(getSymbol(handle, "GetPluginName"));
    auto* getVer   = reinterpret_cast<GetVersionFn>(getSymbol(handle, "GetPluginVersion"));
    auto* regTypes = reinterpret_cast<RegisterTypesFn>(getSymbol(handle, "RegisterPluginTypes"));

    if (!getABI || !getName || !getVer || !regTypes) {
        setLoadError(fmt::format("插件 {} 缺少必需导出函数（GetPluginABIVersion/GetPluginName/GetPluginVersion/RegisterPluginTypes）", path));
        freeLibrary(handle);
        return false;
    }

    // ABI 版本检查
    plugin.abiVersion = getABI();
    if (plugin.abiVersion != kAbiVersion) {
        setLoadError(fmt::format("插件 {} ABI 版本不匹配：{}（期望 {}）——请用当前引擎 SDK 重新构建脚本", path, plugin.abiVersion, kAbiVersion));
        freeLibrary(handle);
        return false;
    }

    plugin.name          = getName();
    plugin.version       = getVer();
    plugin.registerTypes = regTypes;

    ST_CORE_INFO("[PluginManager] Loaded plugin '{}' v{} (ABI {}) from {}",
        plugin.name, plugin.version, plugin.abiVersion, path);

    m_plugins.push_back(std::move(plugin));
    return true;
}

// ── 从配置文件加载 ──────────────────────────────────────

void PluginManager::LoadFromConfig(const std::string& configPath) {
    g_lastLoadError.clear();   // P33：每次加载前重置，供编辑器检查本次加载是否失败

    std::ifstream file(configPath);
    if (!file.is_open()) {
        ST_CORE_ERROR("[PluginManager] Cannot open config file: {}", configPath);
        return;
    }

    nlohmann::json config;
    try {
        file >> config;
    } catch (const std::exception& e) {
        ST_CORE_ERROR("[PluginManager] Failed to parse config JSON: {}", e.what());
        return;
    }

    if (!config.contains("plugins") || !config["plugins"].is_array()) {
        ST_CORE_ERROR("[PluginManager] Config missing 'plugins' array");
        return;
    }

    for (const auto& entry : config["plugins"]) {
        if (!entry.is_object()) {
            ST_CORE_WARN("[PluginManager] Skipping non-object plugin entry");
            continue;
        }
        std::string path = entry.value("path", "");
        if (path.empty()) {
            ST_CORE_WARN("[PluginManager] Skipping plugin entry with empty path");
            continue;
        }
        // P14：相对 DLL 路径以 config.json 所在目录为基准（编辑器 CWD 不可控），
        // 绝对路径照原样使用。Runtime 的 config 与 exe 同目录，行为与旧版（按 CWD）一致。
        const std::filesystem::path dllPath(path);
        if (dllPath.is_relative()) {
            const std::filesystem::path configDir =
                std::filesystem::path(configPath).parent_path();
            if (!configDir.empty())
                path = (configDir / dllPath).lexically_normal().string();
        }
        loadPlugin(path);
    }
}

// ── 注册所有类型 ────────────────────────────────────────

void PluginManager::RegisterAllTypes() {
    for (auto& plugin : m_plugins) {
        if (plugin.registerTypes) {
            ST_CORE_INFO("[PluginManager] Registering types from plugin '{}'", plugin.name);
            // 标记注册来源，便于卸载时按插件清理（factory 位于 DLL 内）
            TypeRegistry::SetRegistrationSource(plugin.name);
            plugin.registerTypes();
        }
    }
    TypeRegistry::SetRegistrationSource("");
}

// ── 卸载所有插件 ────────────────────────────────────────

void PluginManager::UnloadAll() {
    for (auto& plugin : m_plugins) {
        // 先清理插件注册的反射类型：其 factory 分配在 DLL 堆上，
        // 必须在 FreeLibrary 之前注销，否则 TypeRegistry 持有悬垂 std::function
        if (!plugin.name.empty()) {
            size_t removed = TypeRegistry::UnregisterTypesBySource(plugin.name);
            if (removed > 0) {
                ST_CORE_INFO("[PluginManager] Unregistered {} reflected type(s) from plugin '{}'",
                    removed, plugin.name);
            }
        }
        if (plugin.handle) {
            ST_CORE_INFO("[PluginManager] Unloading plugin '{}'", plugin.name);
            freeLibrary(plugin.handle);
            plugin.handle = nullptr;
        }
    }
    m_plugins.clear();
}

} // namespace Shit