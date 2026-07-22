#include "ShitEngine/Core/pch.h"
#include "ShitEngine/UI/UIText.h"
#include "ShitEngine/Render/Renderer.h"
#include "ShitEngine/Resource/ResourceManager.h"
#include "ShitEngine/Core/Log.h"

#include <SDL3_ttf/SDL_ttf.h>

namespace Shit {
	UIText::UIText(const std::string& text, const std::string& fontPath, float fontSize)
		: m_text(text), m_fontPath(fontPath), m_fontSize(fontSize) {}

	UIText::~UIText() {
		if (m_cachedTexture) {
			SDL_DestroyTexture(m_cachedTexture);
			m_cachedTexture = nullptr;
		}
	}

	TTF_Font* UIText::getLoadedFont() {
		if (m_fontPath.empty()) return nullptr;
		TTF_Font* font = ResourceManager::GetFont(m_fontPath, m_fontSize);
		if (!font) {
			ST_CORE_ERROR("UIText: 无法获取字体 {} ({}): {}", m_fontPath, m_fontSize, SDL_GetError());
		}
		return font;
	}

	void UIText::rebuildTexture() {
		SDL_Renderer* renderer = Renderer::GetRenderer();
		if (!renderer) return;

		if (m_cachedTexture) {
			SDL_DestroyTexture(m_cachedTexture);
			m_cachedTexture = nullptr;
		}

		if (m_text.empty()) {
			m_isDirty = false;
			return;
		}

		TTF_Font* font = getLoadedFont();
		if (!font) {
			m_isDirty = false;
			return;
		}

		SDL_Surface* surface = TTF_RenderText_Blended(font, m_text.c_str(), m_text.length(), toSDLColor(m_color));
		if (!surface) {
			ST_CORE_ERROR("UIText: 文字渲染失败 '{}': {}", m_text, SDL_GetError());
			m_isDirty = false;
			return;
		}

		m_cachedTexture = Renderer::CreateTextureFromSurface(surface);
		SDL_DestroySurface(surface);

		if (!m_cachedTexture) {
			ST_CORE_ERROR("UIText: 文字纹理创建失败 '{}': {}", m_text, SDL_GetError());
		}
		m_isDirty = false;
	}

	void UIText::onRender(const SDL_FRect& screenRect) {
		if (m_isDirty) {
			rebuildTexture();
		}
		if (!m_cachedTexture) return;

		float textureWidth = 0.0f, textureHeight = 0.0f;
		SDL_GetTextureSize(m_cachedTexture, &textureWidth, &textureHeight);

		SDL_FRect destRect;
		destRect.y = screenRect.y + (screenRect.h - textureHeight) * 0.5f;
		destRect.h = textureHeight;

		switch (m_anchor) {
			case TextAnchor::Left:
				destRect.x = screenRect.x;
				break;
			case TextAnchor::Right:
				destRect.x = screenRect.x + screenRect.w - textureWidth;
				break;
			case TextAnchor::Center:
			default:
				destRect.x = screenRect.x + (screenRect.w - textureWidth) * 0.5f;
				break;
		}
		destRect.w = textureWidth;

		Renderer::DrawTexture(m_cachedTexture, nullptr, destRect);
	}

	void UIText::onDestroy() {
		if (m_cachedTexture) {
			SDL_DestroyTexture(m_cachedTexture);
			m_cachedTexture = nullptr;
		}
		UIRendererComponent::onDestroy();
	}
}