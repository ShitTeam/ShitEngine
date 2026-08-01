#include "ShitEngine/Core/pch.h"
#include "ShitEngine/System/System.h"

#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"

namespace Shit {
    System::System(int priority) : m_priority(priority) {}
    System::~System() = default;

    void System::init() {
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
