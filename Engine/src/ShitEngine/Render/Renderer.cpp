#include "ShitEngine/Core/pch.h"
#include "ShitEngine/Core/EngineContext.h"

#include <cstring>

#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Core/Window.h"
#include "ShitEngine/Core/Log.h"
#include "ShitEngine/Core/Config.h"
#include "ShitEngine/Core/Time.h"
#include "ShitEngine/Render/Sprite.h"
#include "ShitEngine/Resource/ResourceManager.h"

namespace Shit {
    Renderer& Renderer::GetInstance() {
        return EngineContext::current().renderer;
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
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
            SDL_RenderClear(r);
        }
    }

    bool Renderer::beginOffscreen() {
        SDL_Renderer* r = raw();
        if (!r) return false;
        // 懒创建逻辑尺寸(1280×720)目标纹理；之后渲染进该纹理，读出即逻辑坐标
        if (!m_offscreenTarget) {
            m_offscreenTarget.reset(SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888,
                SDL_TEXTUREACCESS_TARGET, m_logicalWidth, m_logicalHeight));
        }
        if (!m_offscreenTarget) return false;
        return SDL_SetRenderTarget(r, m_offscreenTarget.get());
    }

    void Renderer::endOffscreen() {
        if (SDL_Renderer* r = raw()) SDL_SetRenderTarget(r, nullptr);
    }

    void Renderer::present() {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_RenderPresent(r);
        }
    }

    bool Renderer::readPixels(void* pixels, int pitch) {
        SDL_Renderer* r = raw();
        if (!r || !pixels) return false;

        // SDL3 新 API：返回当前渲染目标的 SDL_Surface
        SDL_Surface* frame = SDL_RenderReadPixels(r, nullptr);
        if (!frame) return false;

        bool ok = false;
        // 转为 ARGB8888（与 QImage::Format_ARGB32 字节序一致），逐行拷贝到调用方缓冲
        SDL_Surface* argb = SDL_ConvertSurface(frame, SDL_PIXELFORMAT_ARGB8888);
        if (argb) {
            for (int y = 0; y < argb->h; ++y) {
                memcpy(static_cast<char*>(pixels) + static_cast<size_t>(y) * pitch,
                       static_cast<char*>(argb->pixels) + static_cast<size_t>(y) * argb->pitch,
                       static_cast<size_t>(argb->w) * 4);
            }
            ok = true;
            SDL_DestroySurface(argb);
        }
        SDL_DestroySurface(frame);
        return ok;
    }

    void Renderer::DrawSprite(const Sprite& sprite, const Vector2& position, const std::optional<Vector2>& size) {
        auto& instance = GetInstance();
        SDL_Renderer* renderer = instance.raw();
        if (!renderer) return;  // 渲染器未初始化/已销毁
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
        SDL_RenderTextureRotated(renderer, texture,
            useSrcRect ? &srcRect : nullptr, &destRect, 0.0, nullptr, flip);
    }

    void Renderer::SetDrawColor(const Color& color) {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_SetRenderDrawColor(r, color.red, color.green, color.blue, color.alpha);
        }
    }

    void Renderer::FillRect(const SDL_FRect& rect) {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_RenderFillRect(r, &rect);
        }
    }

    void Renderer::SetClipRect(const SDL_Rect* rect) {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_SetRenderClipRect(r, rect);
        }
    }

    void Renderer::ClearClipRect() {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_SetRenderClipRect(r, nullptr);
        }
    }

    void Renderer::SetViewport(const SDL_Rect* viewport) {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_SetRenderViewport(r, viewport);
        }
    }

    void Renderer::ClearViewport() {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_SetRenderViewport(r, nullptr);
        }
    }

    void Renderer::DrawTexture(SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect& dst) {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_RenderTexture(r, texture, src, &dst);
        }
    }

    void Renderer::DrawTextureRotated(SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect& dst,
        double angle, const SDL_FPoint* center, SDL_FlipMode flip) {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_RenderTextureRotated(r, texture, src, &dst, angle, center, flip);
        }
    }

    void Renderer::RenderCoordinatesToWindow(float x, float y, float* winX, float* winY) {
        if (SDL_Renderer* r = GetInstance().raw()) {
            SDL_RenderCoordinatesToWindow(r, x, y, winX, winY);
        }
    }

    SDL_Texture* Renderer::CreateTextureFromSurface(SDL_Surface* surface) {
        SDL_Renderer* renderer = GetInstance().raw();
        if (!renderer) return nullptr;  // 渲染器未初始化/已销毁

        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
        if (!tex) {
            ST_CORE_ERROR("Renderer::CreateTextureFromSurface 失败: {}", SDL_GetError());
        }
        return tex;
    }
}
