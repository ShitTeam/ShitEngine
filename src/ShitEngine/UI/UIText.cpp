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

		// 收集所有行
		std::vector<std::string> lines;
		std::string line;
		for (size_t i = 0; i < m_text.size(); ++i) {
			if (m_text[i] == '\n') {
				lines.push_back(line);
				line.clear();
			} else {
				line.push_back(m_text[i]);
			}
		}
		lines.push_back(line); // 最后一行（或唯一行）

		int lineHeight = TTF_GetFontHeight(font);

		int maxWidth = 0;
		for (const auto& l : lines) {
			int w = 0, h = 0;
			if (!l.empty())
				TTF_GetStringSize(font, l.c_str(), l.length(), &w, &h);
			if (w > maxWidth) maxWidth = w;
		}

		// 合并所有行为一张纹理
		int totalHeight = static_cast<int>(lines.size()) * lineHeight;
		if (totalHeight <= 0 || maxWidth <= 0) {
			m_isDirty = false;
			return;
		}

		SDL_Surface* merged = SDL_CreateSurface(maxWidth, totalHeight, SDL_PIXELFORMAT_RGBA32);
		if (!merged) {
			ST_CORE_ERROR("UIText: 无法创建多行纹理: {}", SDL_GetError());
			m_isDirty = false;
			return;
		}
		// merged surface 默认已为全零（全部为零的内存分配），即为黑色透明，无需额外填充

		SDL_Color color = toSDLColor(m_color);
		for (size_t i = 0; i < lines.size(); ++i) {
			if (lines[i].empty()) continue;
			SDL_Surface* lineSurf = TTF_RenderText_Blended(font, lines[i].c_str(), lines[i].length(), color);
			if (!lineSurf) continue;
			SDL_Rect dstRect{0, static_cast<int>(i) * lineHeight, 0, 0};
			SDL_BlitSurface(lineSurf, nullptr, merged, &dstRect);
			SDL_DestroySurface(lineSurf);
		}

		m_cachedTexture = Renderer::CreateTextureFromSurface(merged);
		SDL_DestroySurface(merged);

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