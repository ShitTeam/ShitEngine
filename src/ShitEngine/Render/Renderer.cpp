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
        // 创建 SDL 渲染器
        m_renderer = std::unique_ptr<SDL_Renderer, SDLRendererDeleter>(SDL_CreateRenderer(Window::GetWindow(), nullptr));

        if (!m_renderer) {
            ST_CORE_ERROR("无法创建渲染器! SDL错误: {}", SDL_GetError());
            return false;
        }

        // 从配置读取目标帧率
        m_targetFPS = Config::GetWindowConfig().targetFPS;

        return true;
    }

    void Renderer::limitFPS() {
        if (m_targetFPS == 0) return;

        Uint64 currentTime = SDL_GetTicksNS();
        Uint64 totalTime = static_cast<Uint64>(Time::GetTotalTime() * 1000000000.0f);
        Uint64 frameTime = currentTime - totalTime; // 当前帧的时间

        Uint64 targetFrameTime = 1000000000ULL / m_targetFPS;

        if (targetFrameTime > frameTime) {
            SDL_DelayNS(targetFrameTime - frameTime);
        }
    }
}
