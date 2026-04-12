#pragma once
#include "System.h"

namespace Shit {
    class Behavior;

    /**
     * @brief BehaviorSystem 行为系统
     *
     * 负责执行 Behavior 组件的系统
     */
    class SHIT_API BehaviorSystem final : public System {
    public:
        BehaviorSystem();

        ~BehaviorSystem() override;

        void update() override;
        void destroy() override;

        void registerBehavior(Behavior *behavior);
        void unregisterBehavior(Behavior *behavior);

    private:
        std::vector<Behavior*> m_behaviors; // 挂载的行为组件
        std::vector<Behavior*> m_pendingBehaviors; // 延迟注册 Behavior
    };
}
