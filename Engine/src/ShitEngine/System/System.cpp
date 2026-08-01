#include "ShitEngine/Core/pch.h"
#include "ShitEngine/System/System.h"

#include "ShitEngine/Core/Log.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"

namespace Shit {
    System::System(int priority) : m_priority(priority) {}
    System::~System() = default;

    void System::init() {
        // 逻辑不变量：System 必须经 registerSystem 注册（内部已 setScene）后才 init
        ST_CORE_ASSERT(m_scene != nullptr, "System::init 需要 m_scene——System 必须经 Scene::registerSystem 注册");
        if (!m_scene) return;  // Release 下断言为 no-op，这里兜底防空指针

        auto& gameObjects = m_scene->getGameObjects();

        // 补扫：系统注册时，对尚未注册的组件重新执行 onAttach
        //（组件先加、驱动系统后注册时生效）。用只读遍历，不暴露内部 map。
        for (auto& go : gameObjects) {
            go->forEachComponent([this](Component* comp) {
                if (!comp->isRegistered()) {
                    comp->onAttach();
                }
            });
        }
    }
}
