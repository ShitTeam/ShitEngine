// Plugin C ABI 导出函数
// Runtime 通过 LoadLibrary/GetProcAddress 调用这些函数来加载插件

#include "ReflectionTestScene.h"
#include "ReflectionTestTypes.h"

#include <ShitEngine.h>

#include "reflection/ReflectionRegisterAll.h"

// ── 插件 ABI 版本（用于兼容性检查）──────────────────────
extern "C" __declspec(dllexport)
int GetPluginABIVersion() {
    return 1;
}

// ── 插件元信息 ──────────────────────────────────────────
extern "C" __declspec(dllexport)
const char* GetPluginName() {
    return "ExamplePlugin";
}

extern "C" __declspec(dllexport)
const char* GetPluginVersion() {
    return "1.0.0";
}

// ── 注册插件中的反射类型到共享 TypeRegistry ──────────────
// 必须在 Game::Init() 之后调用（引擎内置类型已注册）
extern "C" __declspec(dllexport)
void RegisterPluginTypes() {
    RegisterAllReflectedTypes();
}

// ── 创建插件的主场景 ────────────────────────────────────
// Runtime 获取此函数指针后调用，将返回的 Scene* 推入 SceneManager
// 所有权转移给 Runtime
extern "C" __declspec(dllexport)
Shit::Scene* CreateMainScene() {
    auto scene = createReflectionTestScene();
    return scene.release();
}
