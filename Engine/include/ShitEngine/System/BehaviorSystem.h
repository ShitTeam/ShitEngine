#pragma once
#include "System.h"
#include "ShitEngine/Reflection/Macros.h"

namespace Shit {
    class Behavior;

    /**
     * @brief Behavior 驱动系统
     *
     * 每帧遍历已注册的 Behavior 组件，驱动其 onStart / onUpdate。
     * Behavior 的 onAttach / onDetach 会自动调用 register / unregister。
     */
    class SHIT_API SHIT_REFLECT(WhiteList) BehaviorSystem final : public System {
        SHIT_REFLECT_BODY(BehaviorSystem)
    public:
        BehaviorSystem(int priority = 0);
        ~BehaviorSystem() override;

        void update() override;
        void destroy() override;

        // 组件认领：Behavior 及其子类（如 AnimationComponent）
        bool onComponentAttached(Component* component) override;
        void onComponentDetached(Component* component) override;

        void registerBehavior(Behavior *behavior);   ///< 注册 Behavior（onAttach 时自动调用）
        void unregisterBehavior(Behavior *behavior); ///< 注销 Behavior（onDetach 时自动调用）

        /// 重置全部 Behavior 的启动标志（isStarted=false）——进入运行态时调用，
        /// 使 onStart 在运行第一帧重新执行（Unity 式「每次运行从头开始」）。
        void resetAllBehaviors();

    private:
        std::vector<Behavior*> m_behaviors;
        std::vector<Behavior*> m_pendingBehaviors;
    };
}
