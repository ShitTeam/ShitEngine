// Plugin C ABI 导出函数
// Runtime 通过 LoadLibrary/GetProcAddress 调用这些函数来加载插件。
// ABI v2：插件 = 脚本库，只注册反射类型，不导出场景工厂（场景来自 .scene 文件）。

#include "ReflectionTestTypes.h"

#include <ShitEngine.h>

// 插件反射类型注册（由 ReflectionScanner 生成）
#include "reflection/ReflectionRegisterAll.h"

// 跨平台导出宏：Windows 用 __declspec(dllexport)，Linux/macOS 用 visibility 属性
#if defined(_WIN32)
    #define SHIT_PLUGIN_EXPORT __declspec(dllexport)
#else
    #define SHIT_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

// ── 插件 ABI 版本（与引擎 PluginManager::kAbiVersion 保持一致）──
extern "C" SHIT_PLUGIN_EXPORT
int GetPluginABIVersion() {
    return Shit::PluginManager::kAbiVersion;
}

// ── 插件元信息 ──────────────────────────────────────────
extern "C" SHIT_PLUGIN_EXPORT
const char* GetPluginName() {
    return "ExamplePlugin";
}

extern "C" SHIT_PLUGIN_EXPORT
const char* GetPluginVersion() {
    return "1.3.0";
}

// ── 注册插件中的反射类型到共享 TypeRegistry ──────────────
// 必须在 Game::Init() 之后调用（引擎内置类型已注册）。
// 注册内容包括：自定义行为（Behaviors.h / ReflectionBehavior.h）与
// 反射测试类型（TestPlayer / TestEnemy / TestDirection）。
extern "C" SHIT_PLUGIN_EXPORT
void RegisterPluginTypes() {
    RegisterAllReflectedTypes();
}