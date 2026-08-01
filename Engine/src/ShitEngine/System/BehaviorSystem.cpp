#include "ShitEngine/Core/pch.h"
#include "ShitEngine/System/BehaviorSystem.h"

#include "ShitEngine/Component/Behavior.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"

namespace Shit {
    BehaviorSystem::BehaviorSystem(int priority) : System(priority) {
    }

    BehaviorSystem::~BehaviorSystem() = default;

    void BehaviorSystem::update() {
        // 处理延迟添加
        for (auto& b : m_pendingBehaviors) {
            if (b) m_behaviors.push_back(b);
            else ST_CORE_WARN("试图注册 Behavior 空指针！");
        }
        m_pendingBehaviors.clear();

        // 按下标遍历更新。用户代码（onStart/onUpdate）可能注销/销毁当前或后续 Behavior，
        // unregisterBehavior 会把条目置为墓碑（nullptr），遍历结束后统一压缩。
        for (size_t i = 0; i < m_behaviors.size(); ++i) {
            Behavior* b = m_behaviors[i];
            if (!b) continue;
            if (!b->isStarted()) {
                b->onStart();
                b->setStarted(true);
            }
            b->onUpdate();
        }

        // 压缩墓碑（遍历后统一清理，避免遍历期间 vector 元素移动导致跳过/悬垂）
        if (std::find(m_behaviors.begin(), m_behaviors.end(), nullptr) != m_behaviors.end()) {
            m_behaviors.erase(
                std::remove(m_behaviors.begin(), m_behaviors.end(), nullptr),
                m_behaviors.end());
        }
    }

    void BehaviorSystem::destroy() {
        m_behaviors.clear();
        m_pendingBehaviors.clear();
    }

    bool BehaviorSystem::onComponentAttached(Component* component) {
        if (auto* behavior = dynamic_cast<Behavior*>(component)) {
            registerBehavior(behavior);
            return true;
        }
        return false;
    }

    void BehaviorSystem::onComponentDetached(Component* component) {
        if (auto* behavior = dynamic_cast<Behavior*>(component)) {
            unregisterBehavior(behavior);
        }
    }

    void BehaviorSystem::registerBehavior(Behavior *behavior) {
        if (!behavior) {
            auto* scene = getScene();
            ST_CORE_WARN("试图在场景 {} 中注册 Behavior 空指针！",
                scene ? scene->getName() : "null");
            return;
        }

        m_pendingBehaviors.push_back(static_cast<Behavior *>(behavior));
    }

    void BehaviorSystem::unregisterBehavior(Behavior *behavior) {
        if (!behavior) {
            auto* scene = getScene();
            ST_CORE_WARN("试图在场景 {} 中移除 Behavior 空指针！",
                scene ? scene->getName() : "null");
            return;
        }

        // 如果在延迟添加列表内，直接移除
        m_pendingBehaviors.erase(
            std::remove(m_pendingBehaviors.begin(), m_pendingBehaviors.end(), behavior),
            m_pendingBehaviors.end()
        );

        // 置为墓碑（nullptr），update() 遍历结束后统一压缩。
        // 注意：不能立即 erase——update 正在遍历 m_behaviors，
        // 立即擦除会使当前迭代位置之后的元素前移，导致跳过或悬垂。
        for (auto& b : m_behaviors) {
            if (b == behavior) {
                b = nullptr;
                break;
            }
        }
    }
}
