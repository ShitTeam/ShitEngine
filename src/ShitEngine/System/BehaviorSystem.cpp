#include "ShitEngine/System/BehaviorSystem.h"

#include "ShitEngine/Component/Behavior.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Scene/Scene.h"

namespace Shit {
    BehaviorSystem::BehaviorSystem() : System(0) {
    }

    BehaviorSystem::~BehaviorSystem() = default;

    void BehaviorSystem::update() {
        // 处理延迟添加
        for (auto& b : m_pendingBehaviors) {
            if (b) m_behaviors.push_back(b);
            else ST_CORE_WARN("试图注册 Behavior 空指针！");
        }
        m_pendingBehaviors.clear();

        // 更新
        for (auto& b : m_behaviors) {
            if (!b->isStarted()) {
                b->onStart(); // 启动组件
                b->setStarted(true);
            }
            b->onUpdate();
        }
    }

    void BehaviorSystem::destroy() {
        m_behaviors.clear();
        m_pendingBehaviors.clear();
    }

    void BehaviorSystem::registerBehavior(Behavior *behavior) {
        if (!behavior) {
            ST_CORE_WARN("试图在场景 {} 中注册 Behavior 空指针！", getScene()->getName());
            return;
        }

        m_pendingBehaviors.push_back(static_cast<Behavior *>(behavior));
    }

    void BehaviorSystem::unregisterBehavior(Behavior *behavior) {
        if (!behavior) {
            ST_CORE_WARN("试图在场景 {} 中移除 Behavior 空指针！", getScene()->getName());
            return;
        }

        // 如果在延迟添加列表内
        m_pendingBehaviors.erase(
            std::remove(m_pendingBehaviors.begin(), m_pendingBehaviors.end(), behavior),
            m_pendingBehaviors.end()
        );

        // 从列表内删除
        m_behaviors.erase(
            std::remove(m_behaviors.begin(), m_behaviors.end(), behavior),
            m_behaviors.end()
        );
    }
}
