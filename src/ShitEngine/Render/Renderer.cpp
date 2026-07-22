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

        return true;
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

    void Renderer::SetDrawColor(const Color& color) {
        SDL_SetRenderDrawColor(GetInstance().m_renderer.get(),
            color.red, color.green, color.blue, color.alpha);
    }

    void Renderer::FillRect(const SDL_FRect& rect) {
        SDL_RenderFillRect(GetInstance().m_renderer.get(), &rect);
    }

    void Renderer::SetClipRect(const SDL_Rect* rect) {
        SDL_SetRenderClipRect(GetInstance().m_renderer.get(), rect);
    }

    void Renderer::ClearClipRect() {
        SDL_SetRenderClipRect(GetInstance().m_renderer.get(), nullptr);
    }

    void Renderer::SetViewport(const SDL_Rect* viewport) {
        SDL_SetRenderViewport(GetInstance().m_renderer.get(), viewport);
    }

    void Renderer::ClearViewport() {
        SDL_SetRenderViewport(GetInstance().m_renderer.get(), nullptr);
    }

    void Renderer::DrawTexture(SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect& dst) {
        SDL_RenderTexture(GetInstance().m_renderer.get(), texture, src, &dst);
    }

    void Renderer::DrawTextureRotated(SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect& dst,
        double angle, const SDL_FPoint* center, SDL_FlipMode flip) {
        SDL_RenderTextureRotated(GetInstance().m_renderer.get(), texture, src, &dst, angle, center, flip);
    }

    void Renderer::RenderCoordinatesToWindow(float x, float y, float* winX, float* winY) {
        SDL_RenderCoordinatesToWindow(GetInstance().m_renderer.get(), x, y, winX, winY);
    }

    SDL_Texture* Renderer::CreateTextureFromSurface(SDL_Surface* surface) {
        return SDL_CreateTextureFromSurface(GetInstance().m_renderer.get(), surface);
    }
}
