#include "ShitEngine/System/System.h"

#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"

namespace Shit {
    System::System(int priority = 0) : m_priority(priority) {}
    System::~System() = default;

    void System::init() {
        auto& gameObjects = m_scene->getGameObjects();

        for (auto& go : gameObjects) {
            auto& components = go->getComponents();
            for (auto& component : components) {
                if (!component.second->isRegistered()) {
                    component.second->onAttach();
                }
            }
        }
    }
}
