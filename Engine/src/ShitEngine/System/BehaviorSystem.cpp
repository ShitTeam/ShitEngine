#include "ShitEngine/Core/pch.h"
#include "ShitEngine/System/BehaviorSystem.h"

#include "ShitEngine/Component/Behavior.h"
#include "ShitEngine/Core/Game.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/GameObject/GameObject.h"
#include "ShitEngine/Scene/Scene.h"

namespace Shit {
    BehaviorSystem::BehaviorSystem(int priority) : System(priority) {
    }

    BehaviorSystem::~BehaviorSystem() = default;

    void BehaviorSystem::flushPendingBehaviors() {
        // 处理延迟添加
        for (auto& b : m_pendingBehaviors) {
            if (b) m_behaviors.push_back(b);
            else ST_CORE_WARN("试图注册 Behavior 空指针！");
        }
        m_pendingBehaviors.clear();
    }

    void BehaviorSystem::compactTombstones() {
        // 压缩墓碑（遍历后统一清理，避免遍历期间 vector 元素移动导致跳过/悬垂）
        if (std::find(m_behaviors.begin(), m_behaviors.end(), nullptr) != m_behaviors.end()) {
            m_behaviors.erase(
                std::remove(m_behaviors.begin(), m_behaviors.end(), nullptr),
                m_behaviors.end());
        }
    }

    void BehaviorSystem::fixedUpdate(float fixedDt) {
        // 处理延迟添加（与变步长阶段共用同一队列，先到先处理）
        flushPendingBehaviors();

        // 正常由 Scene 在非暂停态调用；防御性再查一次暂停，也覆盖直接调用场景。
        // 冻结语义与 update 一致：onStart 照常执行（避免卡在未启动状态），onFixedUpdate 冻结。
        const bool paused = Shit::Game::IsPaused();

        // 与 update 相同的墓碑安全遍历：用户代码可能注销/销毁当前或后续 Behavior，
        // unregisterBehavior 会把条目置为墓碑（nullptr），遍历结束后统一压缩。
        for (size_t i = 0; i < m_behaviors.size(); ++i) {
            Behavior* b = m_behaviors[i];
            if (!b) continue;
            GameObject* owner = b->getOwner();
            if (owner && !owner->isActiveInHierarchy()) continue;   // 失活对象不驱动（重新激活后 onStart 照常补跑）
            if (!b->isStarted()) {
                b->onStart();
                // onStart 可能注销/销毁当前 Behavior（removeComponent/removeGameObject）：
                // 条目可能已被置为墓碑（nullptr），重新读取，避免对已释放内存 setStarted/onFixedUpdate。
                b = m_behaviors[i];
                if (!b) continue;
                b->setStarted(true);
            }
            if (!paused) {
                b->onFixedUpdate(fixedDt);
                // 调用后不再触碰 b：其可能已注销/销毁，循环尾部统一压缩墓碑
            }
        }

        compactTombstones();
    }

    void BehaviorSystem::update() {
        // 处理延迟添加
        flushPendingBehaviors();

        // 全局暂停：冻结 onUpdate（onStart 仍执行，避免新组件卡在未启动状态）
        const bool paused = Shit::Game::IsPaused();

        // 按下标遍历更新。用户代码（onStart/onUpdate）可能注销/销毁当前或后续 Behavior，
        // unregisterBehavior 会把条目置为墓碑（nullptr），遍历结束后统一压缩。
        for (size_t i = 0; i < m_behaviors.size(); ++i) {
            Behavior* b = m_behaviors[i];
            if (!b) continue;
            GameObject* owner = b->getOwner();
            if (owner && !owner->isActiveInHierarchy()) continue;   // 失活对象不驱动（重新激活后 onStart 照常补跑）
            if (!b->isStarted()) {
                b->onStart();
                // onStart 可能注销/销毁当前 Behavior（removeComponent/removeGameObject）：
                // 条目可能已被置为墓碑（nullptr），重新读取，避免对已释放内存 setStarted/onUpdate。
                b = m_behaviors[i];
                if (!b) continue;
                b->setStarted(true);
            }
            if (!paused) {
                b->onUpdate();
                // onUpdate 后不再触碰 b：其可能已注销/销毁，循环尾部统一压缩墓碑
            }
        }

        compactTombstones();
    }

    void BehaviorSystem::destroy() {
        // 重置认领组件的注册状态：系统注销后，这些 Behavior 在同系统重新注册时
        // 需能被 System::init 补扫重新认领（否则 m_isRegistered 残留 true 会永久失去驱动）
        for (auto* b : m_behaviors) {
            if (b) resetComponent(b);
        }
        for (auto* b : m_pendingBehaviors) {
            if (b) resetComponent(b);
        }
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

    void BehaviorSystem::resetAllBehaviors() {
        // 置为「未启动」：下次 update 对每个 Behavior 重新执行 onStart
        // （进入运行态时调用 —— Unity 式每局从头开始）
        for (auto* b : m_behaviors) {
            if (b) b->setStarted(false);
        }
        for (auto* b : m_pendingBehaviors) {
            if (b) b->setStarted(false);
        }
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
