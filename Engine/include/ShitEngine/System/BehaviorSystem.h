#pragma once
#include "System.h"
#include "ShitEngine/Reflection/Macros.h"

namespace Shit {
    class Behavior;

    /**
     * @brief Behavior 驱动系统
     *
     * 变步长阶段每帧驱动 onStart / onUpdate；固定步阶段驱动 onFixedUpdate(fixedDt)（与物理同拍）。
     * Behavior 的 onAttach / onDetach 会自动调用 register / unregister。
     */
    class SHIT_API SHIT_REFLECT(WhiteList) BehaviorSystem final : public System {
        SHIT_REFLECT_BODY(BehaviorSystem)
    public:
        BehaviorSystem(int priority = 0);
        ~BehaviorSystem() override;

        void update() override;
        /// @brief 固定步阶段（Scene 固定步循环节拍调用）：补跑 onStart 后驱动 onFixedUpdate(fixedDt)
        void fixedUpdate(float fixedDt) override;
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

        /// @brief 处理延迟添加队列（fixed/update 两阶段共用，先到先处理）
        void flushPendingBehaviors();
        /// @brief 压缩墓碑条目（遍历结束后统一清理，避免遍历期间元素移动导致跳过/悬垂）
        void compactTombstones();
    };
}
