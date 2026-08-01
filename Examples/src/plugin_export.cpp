// Plugin C ABI 导出函数
// Runtime 通过 LoadLibrary/GetProcAddress 调用这些函数来加载插件

#include "PhysicsTestScene.h"
#include "ReflectionTestScene.h"

#include <ShitEngine.h>

// 跨平台导出宏：Windows 用 __declspec(dllexport)，Linux/macOS 用 visibility 属性
#if defined(_WIN32)
    #define SHIT_PLUGIN_EXPORT __declspec(dllexport)
#else
    #define SHIT_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

// ── 插件 ABI 版本（用于兼容性检查）──────────────────────
extern "C" SHIT_PLUGIN_EXPORT
int GetPluginABIVersion() {
    return 1;
}

// ── 插件元信息 ──────────────────────────────────────────
extern "C" SHIT_PLUGIN_EXPORT
const char* GetPluginName() {
    return "ExamplePlugin";
}

extern "C" SHIT_PLUGIN_EXPORT
const char* GetPluginVersion() {
    return "1.0.0";
}

// ── 注册插件中的反射类型到共享 TypeRegistry ──────────────
// 必须在 Game::Init() 之后调用（引擎内置类型已注册）
extern "C" SHIT_PLUGIN_EXPORT
void RegisterPluginTypes() {
    // Physics 组件暂未加入反射
}

// ── 创建插件的主场景 ────────────────────────────────────
// Runtime 获取此函数指针后调用，将返回的 Scene* 推入 SceneManager
// 所有权转移给 Runtime
extern "C" SHIT_PLUGIN_EXPORT
Shit::Scene* CreateMainScene() {
    auto scene = createPhysicsTestScene();
    return scene.release();
}
