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

        // --- getter & setter ---
        Scene* getScene() const { return m_scene; }
        void setScene(Scene* scene) { m_scene = scene; }

        int getPriority() const { return m_priority; }
        void setPriority(int priority) { m_priority = priority; }

    protected:
        int m_priority;          ///< 优先级（小值先执行）
        Scene* m_scene = nullptr; ///< 所属场景
    };
}
