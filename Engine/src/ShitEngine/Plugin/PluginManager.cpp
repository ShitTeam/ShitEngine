#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Plugin/PluginManager.h"

#include "ShitEngine/Core/Log.h"

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
#else
    #include <dlfcn.h>
#endif

namespace Shit {

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
        ST_CORE_ERROR("[PluginManager] Failed to load plugin: {}", path);
#ifdef _WIN32
        ST_CORE_ERROR("  GetLastError() = {}", GetLastError());
#else
        ST_CORE_ERROR("  dlerror() = {}", dlerror());
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
        ST_CORE_ERROR("[PluginManager] Plugin '{}' missing required exports", path);
        freeLibrary(handle);
        return false;
    }

    // ABI 版本检查
    plugin.abiVersion = getABI();
    if (plugin.abiVersion != kAbiVersion) {
        ST_CORE_ERROR("[PluginManager] Plugin '{}' has unsupported ABI version: {} (expected {})",
            path, plugin.abiVersion, kAbiVersion);
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