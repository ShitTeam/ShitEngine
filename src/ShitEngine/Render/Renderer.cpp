#include "ShitEngine/Render/Renderer.h"

#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Config.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Render/Sprite.h"
#include "ShitEngine/Resource/ResourceManager.h"

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

        // 像素模式：最近邻缩放，避免像素画模糊
        SDL_SetDefaultTextureScaleMode(m_renderer.get(), SDL_SCALEMODE_NEAREST);

        // 全局逻辑分辨率
        SDL_SetRenderLogicalPresentation(m_renderer.get(), m_logicalWidth, m_logicalHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);

        m_targetFPS = Config::GetWindowConfig().targetFPS;
        m_lastFrameTimeNS = SDL_GetTicksNS();

        return true;
    }

    void Renderer::limitFPS() {
        if (m_targetFPS == 0) return;

        Uint64 now = SDL_GetTicksNS();
        Uint64 frameElapsed = now - m_lastFrameTimeNS;
        Uint64 targetFrameTime = static_cast<Uint64>(1.0e9f / static_cast<float>(m_targetFPS));

        if (frameElapsed < targetFrameTime) {
            SDL_DelayNS(targetFrameTime - frameElapsed);
        }

        m_lastFrameTimeNS = SDL_GetTicksNS();
    }

    void Renderer::clearScreen() {
        SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 255);
        SDL_RenderClear(m_renderer.get());
    }

    void Renderer::present() {
        SDL_RenderPresent(m_renderer.get());
    }

    void Renderer::DrawSprite(const Sprite& sprite, const Vector2& position, const std::optional<Vector2>& size) {
        auto& instance = GetInstance();

        SDL_Texture* texture = ResourceManager::GetTexture(sprite.getTexturePath());
        if (!texture) {
            ST_CORE_ERROR("DrawSprite: 无法获取纹理 {}", sprite.getTexturePath());
            return;
        }

        SDL_FRect srcRect{};
        bool useSrcRect = false;
        if (sprite.getSourceRect().has_value()) {
            srcRect = sprite.getSourceRect().value();
            useSrcRect = true;
        }

        SDL_FRect destRect{ position.x, position.y, 0, 0 };
        if (size.has_value()) {
            destRect.w = size->x;
            destRect.h = size->y;
        } else if (useSrcRect) {
            destRect.w = srcRect.w;
            destRect.h = srcRect.h;
        } else {
            SDL_GetTextureSize(texture, &destRect.w, &destRect.h);
        }

        SDL_FlipMode flip = sprite.isFlipped() ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
        SDL_RenderTextureRotated(instance.m_renderer.get(), texture,
            useSrcRect ? &srcRect : nullptr, &destRect, 0.0, nullptr, flip);
    }
}
