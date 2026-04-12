#pragma once
#include "ShitEngine/Core/Core.h"


namespace Shit {
    class Scene;
    class Component;
    /**
     * @brief System 基类
     * 所有的 System 都必须继承于此
     */
    class SHIT_API System {
    public:
        System(int priority = 0);
        virtual ~System();

        virtual void init();      // 初始化
        virtual void update() = 0;  // 更新
        virtual void destroy() = 0; // 销毁


        // --- getter & setter ---
        Scene* getScene() const { return m_scene; }
        void setScene(Scene* scene) { m_scene = scene; }

        int getPriority() const { return m_priority; }
        void setPriority(int priority) { m_priority = priority; }

    protected:
        int m_priority; // 系统优先值，规定系统的运行顺序

        Scene* m_scene = nullptr; // 场景指针
    };
}
