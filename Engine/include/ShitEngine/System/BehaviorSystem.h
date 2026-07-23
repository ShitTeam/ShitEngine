#pragma once
#include "System.h"

namespace Shit {
    class Behavior;

    /**
     * @brief Behavior 驱动系统
     *
     * 每帧遍历已注册的 Behavior 组件，驱动其 onStart / onUpdate。
     * Behavior 的 onAttach / onDetach 会自动调用 register / unregister。
     */
    class SHIT_API BehaviorSystem final : public System {
    public:
        BehaviorSystem(int priority = 0);
        ~BehaviorSystem() override;

        void update() override;
        void destroy() override;

        void registerBehavior(Behavior *behavior);   ///< 注册 Behavior（onAttach 时自动调用）
        void unregisterBehavior(Behavior *behavior); ///< 注销 Behavior（onDetach 时自动调用）

    private:
        std::vector<Behavior*> m_behaviors;
        std::vector<Behavior*> m_pendingBehaviors;
    };
}
