#include "ShitEngine/Render/Renderer.h"

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Config.h"
#include "ShitEngine/Core/Time.h"

namespace Shit {
    Renderer& Renderer::GetInstance() {
        static Renderer instance;
        return instance;
    }

    bool Renderer::init() {
        m_renderer = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>(SDL_CreateRenderer(Window::GetWindow(), nullptr));

        if (!m_renderer) {
            ST_CORE_ERROR("无法创建渲染器! SDL错误: {}", SDL_GetError());
            return false;
        }

        m_targetFPS = Config::GetWindowConfig().targetFPS;

        return true;
    }

    void Renderer::limitFPS() {
        if (m_targetFPS > 0) {
            Uint64 currentTime = SDL_GetTicks();
            Uint64 totalTime = static_cast<Uint64>(Time::GetTotalTime() * 1000000000.0f);
            Uint64 deltaTime = currentTime - totalTime;

            // 计算需要等待的时间
            Uint64 timeToWait = static_cast<Uint64>(1.0f / static_cast<float>(m_targetFPS) * 1000000000.0f) - deltaTime;

            if (timeToWait > 0) {
                SDL_DelayNS(timeToWait);
            }
        }
    }
}
