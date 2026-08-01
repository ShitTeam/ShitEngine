#pragma once
#include "ShitEngine/Core/Core.h"


namespace Shit {
    class Scene;
    class Component;
    /**
     * @brief System 基类
     *
     * 所有自定义系统必须继承此类并实现 update() 与 destroy()。
     * 系统按 priority 值排序，小值先执行。
     * 通过 Scene::registerSystem<T>() 注册到场景。
     */
    class SHIT_API System {
    public:
        System(int priority = 0);
        virtual ~System();

        virtual void init();        ///< 初始化（可覆写）
        virtual void update() = 0;    ///< 每帧更新（纯虚）
        virtual void destroy() = 0;   ///< 销毁（纯虚）

        // --- 组件生命周期回调（解耦：组件不再查具体系统类型，由 Scene 广播给所有系统） ---
        /// @brief 组件挂载时调用。返回 true 表示本系统认领并处理了该组件（组件据此确定已注册）。
        virtual bool onComponentAttached(Component* component) { (void)component; return false; }
        /// @brief 组件卸下时调用。
        virtual void onComponentDetached(Component* component) { (void)component; }

        // --- getter & setter ---
        Scene* getScene() const { return m_scene; }
        void setScene(Scene* scene) { m_scene = scene; }

        int getPriority() const { return m_priority; }
        void setPriority(int priority) { m_priority = priority; }

    protected:
        int m_priority;          ///< 优先级（小值先执行）
        Scene* m_scene = nullptr; ///< 所属场景

        /// @brief 内部：把认领过的组件标记为未注册（系统销毁/注销时调用）。
        /// 否则组件 m_isRegistered 残留 true，同类系统重新注册时 System::init 补扫会跳过它们，
        /// 导致组件永久失去驱动系统。
        void resetComponent(Component* comp);
    };
}
